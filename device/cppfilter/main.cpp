#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "utils.hpp"
#include "filter.hpp"

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

void work(std::unique_ptr<Filter>& usedFilter, const std::string& ip,const int port, const float value){
    if (usedFilter->filter(value)){
        return; //if the value is filtered, then there is no work to do here
    }
    usedFilter->sendMessage(value);
    return;
}

int main(int argc, char *argv[]){
    if (argc < 8){
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

    auto sampleAllData = false;

    if(argc == 9){
        //To determine if we want nice data or monitor the system
        //Graph is fucked up if we don't send data every time, so this is a small hack, to create nice looking graph
        sampleAllData = static_cast<bool>(std::stoi(argv[8]));
    }

    auto deviceId = getDeviceId(deviceIdFilepath);

    if (!deviceId.has_value()){
        std::cout << "Specified Id-File does not exists" << "\n";
        return 1;
    }

    DeviceId = deviceId.value(); 

    std::cout << "Device Id: " << DeviceId << "\n"; 

    File = std::ifstream{DataFilepath};    

    std::unique_ptr<Filter> usedFilter;

    switch (algorithmId){
    case::AlgorithmId::LMSFILTER:
        usedFilter = std::make_unique<LMSFilter>(0.5, 5, 1, DeviceId, SamplePeriod, ip, port);
        break;
    case AlgorithmId::STATICFILTER:
        usedFilter = std::make_unique<StaticFilter>(0.5, DeviceId, ip, port, sampleAllData);
        break;
    case AlgorithmId::STATICMEANFILTER:
        usedFilter = std::make_unique<StaticMeanFilter>(0.5, 5, DeviceId, ip, port, sampleAllData);
        break;
    case AlgorithmId::BASELINE :
    default:
        usedFilter = std::make_unique<Baseline>(DeviceId, ip, port);
        break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    auto endTime = std::chrono::system_clock::now() + std::chrono::minutes(SampleDuration);

    for(;;){
        auto nextIterationStart = std::chrono::system_clock::now() + std::chrono::milliseconds(SamplePeriod);
        const auto value = readDataPoint();

        std::cout << "Data point:" << value << "\n";

        work(usedFilter,ip,port,value);

        if(nextIterationStart > endTime){
            return 0;
        }

        std::this_thread::sleep_until(nextIterationStart);
    }
    
    return 0;
}