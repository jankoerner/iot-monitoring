#include <cassert>
#include <cstdlib>
#include <math.h>
#include <numeric>
#include <iostream>

#include "filter.hpp"
#include "utils.hpp"

namespace {
    bool differenceBelowThreshold(const double oldValue, const double newValue, const double threshold){
        auto absDifference = std::abs(oldValue - newValue);
        return absDifference <= threshold;
    }
}


Filter::Filter(const double threshold, const std::int64_t windowSize, AlgorithmId algoId, std::int64_t deviceId,
const std::string &ip, const std::int64_t port) :
Threshold{threshold}, MaxWindowSize{windowSize}, Window{std::vector<double>(windowSize,0)}, AlgoId{algoId}, 
DeviceId{deviceId}, Ip{ip}, Port{port}{
    assert(("The threshold is not lower than 1", Threshold < 1));
    return;
}

void Filter::sendMessage(const double value){
    Client client{Ip,Port};
    auto msg = createMsg(DeviceId, AlgoId, value);
    if (!client.sendMessage(msg)){
        std::cout << "Sent Message failed" << "\n";
    }
}

Baseline::Baseline(const std::int64_t deviceId, const std::string &ip, const std::int64_t port) : 
Filter{0, 0, AlgorithmId::BASELINE, deviceId, ip, port}{
    return;
}

bool Baseline::filter(const double value){
    return false; 
};

StaticMeanFilter::StaticMeanFilter(const double threshold, const std::int64_t windowSize, const std::int64_t deviceId,
const std::string &ip, const std::int64_t port) : 
WindowIndex{0}, WindowFull{false}, Filter{threshold, windowSize, AlgorithmId::STATICMEANFILTER, deviceId, ip, port}{
    return;
};

bool StaticMeanFilter::filter(const double value){
    const auto mean = calculateWindowMean();
    updateWindow(value);

    return differenceBelowThreshold(mean,value,Filter::Threshold);
}

double StaticMeanFilter::calculateWindowMean(){
    const double referenceValue = Window[WindowIndex];
    double currentValue;
    if(WindowIndex == 0){
        currentValue = Window[MaxWindowSize-1];
    }else{
        currentValue = Window[WindowIndex-1];
    }

    const double sum = currentValue - referenceValue;
    
    if(!WindowFull){
        if (WindowIndex == 0){
            return 0;
        }

        if(WindowIndex == (MaxWindowSize-1)){
            WindowFull = true;
        }

        return sum / static_cast<double>(WindowIndex);
    }

    return sum / static_cast<double>(MaxWindowSize);
}

void StaticMeanFilter::updateWindow(const double value){
    if(WindowIndex == 0){
        Window[WindowIndex] = Window[MaxWindowSize-1] + value;
        ++WindowIndex;
    }else if(WindowIndex == (MaxWindowSize-1)){
        Window[WindowIndex] = Window[WindowIndex-1] + value;
        WindowIndex=0;
    }else{
        Window[WindowIndex] = Window[WindowIndex-1] + value;
        ++WindowIndex;
    }

    return;
}

StaticFilter::StaticFilter(const double threshold, const std::int64_t deviceId, const std::string &ip, const std::int64_t port) :  
Filter{threshold, 0, AlgorithmId::STATICFILTER, deviceId, ip, port}{
    return;
}

bool StaticFilter::filter(const double value){
    if (!Initialized){
        PreviousSentValue = value;
        Initialized = true;
        return false;
    }

    if (differenceBelowThreshold(PreviousSentValue,value,Filter::Threshold)){
        return true;
    }
    
    PreviousSentValue = value;
    return false;
}

LMSFilter::LMSFilter(const float threshold, const std::int64_t windowSize, 
const std::int64_t successfulPredictions, const std::int64_t deviceId, const std::int64_t sampleRate, 
const std::string &ip, const std::int64_t port) : 
CurrentState{LMSFilter::State::INITIALIZATION}, Weights{std::vector<double>(windowSize,0.)},
CurrentReadingNo{0}, SuccessfulPredictions{successfulPredictions},
Filter{threshold, windowSize,AlgorithmId::LMSFILTER, deviceId, ip, port}{
    Client client{ip,port};
    auto msg = std::to_string(deviceId) + ",INIT," + std::to_string(windowSize) + "," + std::to_string(sampleRate); 
    client.sendMessage(msg);
    return;
}

LMSFilter::~LMSFilter(){
    Client client{Filter::Ip,Filter::Port};
    auto msg = std::to_string(Filter::DeviceId) + ",STOP"; 
    client.sendMessage(msg);
    return;
}


bool LMSFilter::filter(const double value){
    switch (CurrentState){
    case State::INITIALIZATION:
        initialize(value);
        return false;
    case State::NORMAL:
        return normal(value);
    case State::STANDALONE:
        return standalone(value);
    default:
        break;
    }

    return true;
}

void LMSFilter::sendMessage(const double value){
    Client client{Filter::Ip,Filter::Port};
    std::string msg = std::to_string(Filter::DeviceId) + ",DATA," + std::to_string(value) + "," +std::to_string(CurrentState);
    client.sendMessage(msg);
    return;
}

void LMSFilter::initialize(const double value){
    if(CurrentReadingNo >= Filter::MaxWindowSize - 1){
        
        double sum = std::accumulate(Filter::Window.begin(),Filter::Window.end(),0.0,
        [](const double sum, const double v){
            return sum + pow(std::abs(v),2);
        });
        double ex = (1.0 / Filter::MaxWindowSize) * sum;
        double upperBound = 1.0 / ex;

        LearningRate = upperBound / 100.0;

        std::cout << "LearningRate: " << LearningRate << "\n";

        CurrentState = State::NORMAL;
        CurrentReadingNo = 0;
    }else{
        CurrentReadingNo++;
    }

    updateWindow(value);
    return;
}

bool LMSFilter::normal(const double value){
    auto prediction = predict();
    
    if(differenceBelowThreshold(value,prediction,Filter::Threshold)){
        CurrentReadingNo++;
        if(CurrentReadingNo > SuccessfulPredictions){
            CurrentState = State::STANDALONE;
        }
    }

    updateWeights(prediction, value);
    updateWindow(value);

    return false;
}

bool LMSFilter::standalone(const double value){
    auto prediction = predict();

    if(!differenceBelowThreshold(value,prediction,Filter::Threshold)){
        CurrentReadingNo = 0;
        CurrentState = State::NORMAL;
        updateWeights(prediction,value);
        updateWindow(value);
        std::cout << "Value sent: " << value << "\n";
        return false;
    }

    std::cout << "Value sent: " << prediction << "\n";
    updateWindow(prediction);
    return true;
}

double LMSFilter::predict(){
    return std::inner_product(Filter::Window.begin(), Filter::Window.end(), Weights.begin(),0.0);
}

void LMSFilter::updateWeights(const double prediction, const double target){
    auto error = target - prediction;
    for(int i = 0; i<Filter::MaxWindowSize;++i){
        Weights[i] += LearningRate * error * Filter::Window[i];
    }
}

void LMSFilter::updateWindow(const double value){
    std::move(begin(Filter::Window) + 1, end(Filter::Window), begin(Filter::Window));
    Filter::Window[MaxWindowSize-1] = value;
}
