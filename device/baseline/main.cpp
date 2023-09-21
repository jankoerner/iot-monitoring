#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#include "../shared/utils.hpp"

namespace{
    std::string Filepath;
    std::ifstream File;
    std::int64_t SamplePeriod = 1000;
    std::int64_t SampleDuration = 1;
}

float readDataPoint(){    
    std::string line;
    std::getline(File,line);

    return std::stof(line);
}

int main(int argc, char *argv[]){
    if (argc < 6){
        std::cout << "You need to specify the filepath, the sample period (ms),the sample duration (min), the server ip and the server port";
        return 1;
    }
    
    Filepath = argv[1];
    SamplePeriod = std::stoi(argv[2]);
    SampleDuration = std::stoi(argv[3]);
    auto ip = argv[4];
    auto port = std::stoi(argv[5]);

    Client client{ip,port};

    if (! client.connectToServer()){
        return -1;
    }

    File = std::ifstream{Filepath};

    auto totalIterations = SampleDuration * ((SamplePeriod / 1000) * 60);

    for(int i = 0; i < totalIterations; ++i){
        const auto value = readDataPoint();
        auto msg = createMsg(AlgorithmId::BASELINE, value);
        if (!client.sendMessage(msg)){
            std::cout << "Sent Message failed" << "\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(SamplePeriod));
    }
    
    return 0;
}