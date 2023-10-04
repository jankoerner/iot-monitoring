#ifndef CODE_SHARED_FILTER_HPP
#define CODE_SHARED_FILTER_HPP

#include <cstdint>
#include <vector>

class Filter{
    public: 
    Filter(const double threshold, const std::int64_t windowSize);
    virtual bool filter(const double value) = 0;

    protected:
    double Threshold;
    std::int64_t MaxWindowSize;
    std::vector<double> Window;

    double calculateWindowMean();
    void updateWindow(const double value);

    private:
    std::int64_t WindowIndex;
    bool WindowFull;
};

class Baseline : public Filter {
    public: 
    Baseline();
    bool filter(const double value);
};

class StaticFilter : public Filter{
    public:
    StaticFilter(const double threshold, const std::int64_t windowSize);
    bool filter(const double value);
};

class AdaptiveFilter : public Filter {
    public:
    AdaptiveFilter(const float initialThreshold, const float minThreshold, const std::int64_t windowSize);
    bool filter(const float value);

    private:
    float MinThreshold;
    
    void updateThreshold(const float value);
};

#endif