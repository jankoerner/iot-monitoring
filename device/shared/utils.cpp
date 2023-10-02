#include <arpa/inet.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <unistd.h>


#include "utils.hpp"


std::string createMsg(const AlgorithmId algoId, const float value){
    using namespace std::chrono;
    auto utcMilliSec = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
    auto msg = std::to_string(algoId) + "," + std::to_string(utcMilliSec) + "," + std::to_string(value) + "\n";

    return msg;
}

std::string createMsg(const std::int64_t deviceId, const AlgorithmId algoId, const float value){
    return std::to_string(deviceId) + "," + createMsg(algoId,value);
}

std::optional<std::int64_t> getDeviceId(const std::string& deviceIdFilepath){
    if (! std::filesystem::exists(deviceIdFilepath))
    {
        return std::nullopt;
    }

    auto fileStream = std::ifstream{deviceIdFilepath};
    
    std::string line;
    std::getline(fileStream,line);

    fileStream.close();
    std::cout << line.size() << "\n";  

    return std::stoi(line);
}


Client::Client(const std::string& ip, const std::int64_t port) : Ip{ip}, Port{port} {
    Socket_fd = socket(AF_INET,SOCK_STREAM,0); 
    if(Socket_fd == -1){
        std::cout << "Socket could not be created" << "\n";
    }
    return;
};

Client::~Client(){
    disconnect();
}

bool Client::sendMessage(std::string& msg){
    if (!connectToServer()){
        return false;
    }

    auto bytesSent = send(Socket_fd,msg.c_str(),msg.length(),0);
    if (bytesSent == -1){
        std::cout << "Sent failed" << "\n";
        return false;
    }

    disconnect();

    return true;
}


bool Client::connectToServer(){
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(Ip.c_str());
    servaddr.sin_port = htons(Port);


    if (connect(Socket_fd, (const sockaddr*)&servaddr, sizeof(servaddr))!= 0) {
        std::cout << "Could not connect to the server!" << "\n";
        return false;
    }

    return true;
}

void Client::disconnect(){
    close(Socket_fd);
}