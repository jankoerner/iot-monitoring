#ifndef CODE_SHARED_FILTER_HPP
#define CODE_SHARED_FILTER_HPP

#include <cstdint>
#include <vector>

enum AlgorithmId : int;

class Filter{
    public: 
    Filter(const double threshold, const std::int64_t windowSize, AlgorithmId algoId, 
    const std::int64_t deviceId, const std::string &ip, const std::int64_t port);
    virtual ~Filter() = default;
    virtual void sendMessage(const double value);
    virtual bool filter(const double value) = 0;

    protected:
    double Threshold;
    std::int64_t MaxWindowSize;
    std::vector<double> Window;
    AlgorithmId AlgoId;
    std::int64_t DeviceId;
    std::string Ip;
    std::int64_t Port;
};

class Baseline : public Filter {
    public: 
    Baseline(const std::int64_t deviceId, const std::string &ip, const std::int64_t port);
    virtual ~Baseline() = default;
    bool filter(const double value) override;
};

class StaticMeanFilter : public Filter {
    public:
    StaticMeanFilter(const double threshold, const std::int64_t windowSize, 
    const std::int64_t deviceId,const std::string &ip, const std::int64_t port,
    const bool sampleAllData);
    virtual ~StaticMeanFilter() = default;
    bool filter(const double value) override;
    void sendMessage(const double value) override;

    private:
    double calculateWindowMean();
    void updateWindow(const double value);
    std::int64_t WindowIndex;
    bool WindowFull;
    double Mean;
    bool SampleAllData;
};

class StaticFilter : public Filter {
    public:
    StaticFilter(const double threshold, const std::int64_t deviceId, 
    const std::string &ip, const std::int64_t port, const bool sampleAllData);
    virtual ~StaticFilter() = default;
    bool filter(const double value) override;
    void sendMessage(const double value) override;

    private:
    bool Initialized;
    double PreviousSentValue;
    bool SampleAllData;
};

class LMSFilter : public Filter {
    public:
    LMSFilter(const float threshold, const std::int64_t windowSize, const std::int64_t successfulPredictions, 
    const std::int64_t deviceId, const std::int64_t sampleRate, const std::string &ip, const std::int64_t port);
    virtual ~LMSFilter();
    bool filter(const double value) override;
    void sendMessage(const double value) override;

    private:
    enum State {INITIALIZATION,NORMAL,STANDALONE};
    State CurrentState;
    std::vector<double> Weights;
    double LearningRate;
    std::int64_t CurrentReadingNo; //Used to count the total readings during initialization, then used to count number of successful predictions
    std::int64_t SuccessfulPredictions;

    void initialize(const double value);
    bool normal(const double value);
    bool standalone(const double value);

    double predict();
    void updateWeights(const double prediction, const double target);
    void updateWindow(const double value);
};

#endif