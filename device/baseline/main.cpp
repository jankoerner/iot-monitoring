#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "../shared/utils.hpp"
#include "../shared/filter.hpp"

namespace{
    std::string DataFilepath;
    std::ifstream File;
    std::int64_t SamplePeriod = 1000;
    std::int64_t SampleDuration = 1;
    std::int64_t DeviceId;
}

float readDataPoint(){    
    std::string line;
    std::getline(File,line);

    return std::stof(line);
}

void work(std::unique_ptr<Filter>& usedFilter, const std::string& ip,const int port,const float value){
    if (usedFilter->filter(value)){
        return; //if the value is filtered, then there is no work to do here
    }

    Client client{ip,port};
    auto msg = createMsg(AlgorithmId::BASELINE, value);
    if (!client.sendMessage(msg)){
        std::cout << "Sent Message failed" << "\n";
    }
};


int main(int argc, char *argv[]){
    if (argc < 7){
        std::cout << "You need to specify the filepath, the sample period (ms),the sample duration (min), the server ip, the server port, Filepath to the Id-File and the selected algorithm";
        return 1;
    }
    
    DataFilepath = argv[1];
    SamplePeriod = std::stoi(argv[2]);
    SampleDuration = std::stoi(argv[3]);
    auto ip = argv[4];
    auto port = std::stoi(argv[5]);
    auto deviceIdFilepath = argv[6];

    auto algorithmId = std::stoi(argv[7]);

    auto deviceId = getDeviceId(deviceIdFilepath);

    if (!deviceId.has_value()){
        std::cout << "Specified Id-File does not exists" << "\n";
        return 1;
    }

    DeviceId = deviceId.value(); 
    File = std::ifstream{DataFilepath};    
    auto totalIterations = SampleDuration * ((SamplePeriod / 1000) * 60);

    std::unique_ptr<Filter> usedFilter;

    switch (algorithmId){
    case AlgorithmId::STATICFILTER:
        usedFilter = std::make_unique<StaticFilter>(0.05, 2);
        break;
    case AlgorithmId::BASELINE :
    default:
        usedFilter = std::make_unique<Baseline>();
        break;
    }

    auto endTime = std::chrono::system_clock::now() + std::chrono::minutes(SampleDuration);

    for(;;){
        auto nextIterationStart = std::chrono::system_clock::now() + std::chrono::milliseconds(SamplePeriod);
        const auto value = readDataPoint();
        work(usedFilter,ip,port,value);

        if(nextIterationStart > endTime){
            return 0;
        }

        std::this_thread::sleep_until(nextIterationStart);
    }
    
    return 0;
}