#include <cassert>
#include <cstdlib>
#include <numeric>
#include <iostream>

#include "filter.hpp"

Filter::Filter(const double threshold, const std::int64_t windowSize) :
Threshold{threshold}, MaxWindowSize{windowSize}, Window{std::vector<double>(windowSize,0)}, 
WindowIndex{0}, WindowFull{false}{
    assert(("The threshold is lower than 1", Threshold < 1));
    return;
}

double Filter::calculateWindowMean(){
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

void Filter::updateWindow(const double value){
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

Baseline::Baseline() : Filter{0,0}{
    return;
}

bool Baseline::filter(const double value){
    return false; //There is no filter in the baseline implementation
};

StaticFilter::StaticFilter(const double threshold, const std::int64_t windowSize) : 
Filter{threshold,windowSize}{
    return;
};

bool StaticFilter::filter(const double value){
    const auto mean = Filter::calculateWindowMean();
    updateWindow(value);

    auto absDifference = std::abs(mean - value);
    
    if (absDifference / mean <= Threshold ){
        return true;
    }

    return false;
}

AdaptiveFilter::AdaptiveFilter(float initialThreshold, const float minThreshold, const std::int64_t windowSize) : 
MinThreshold{minThreshold}, Filter{initialThreshold, windowSize} {
    return;
}

bool AdaptiveFilter::filter(const float value){
    return false;
}

void AdaptiveFilter::updateThreshold(const float value){
    Threshold = Threshold;
    return;
}