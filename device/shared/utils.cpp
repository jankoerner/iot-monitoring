#include <arpa/inet.h>
#include <chrono>
#include <iostream>
#include <string>
#include <unistd.h>

#include "utils.hpp"


std::string createMsg(const AlgorithmId algoId, const float value){
    using namespace std::chrono;
    auto utcMilliSec = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
    auto msg = std::to_string(algoId) + "," + std::to_string(utcMilliSec) + "," + std::to_string(value) + "\n";

    return msg;
}

Client::Client(const std::string& ip, const std::int64_t port) : Ip{ip}, Port{port} {
    socket_fd = socket(AF_INET,SOCK_STREAM,0); 
    if(socket_fd == -1){
        std::cout << "Socket could not be created" << "\n";
    }
    return;
};

Client::~Client(){
    close(socket_fd);
}

bool Client::connectToServer(){

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(Ip.c_str());
    servaddr.sin_port = htons(Port);


    if (connect(socket_fd, (const sockaddr*)&servaddr, sizeof(servaddr))!= 0) {
        std::cout << "Could not connect to the server!" << "\n";
        return false;
    }

    return true;
}

bool Client::sendMessage(std::string& msg){
    auto bytesSent = send(socket_fd,msg.c_str(),msg.length(),0);
    if (bytesSent == -1){
        std::cout << "Sent failed" << "\n";
        return false;
    }

    return true;
}