#ifndef CODE_SHARED_UTILS_HPP
#define CODE_SHARED_UTILS_HPP

#include <string>
#include <cstdint>
#include <optional>

enum AlgorithmId {
    BASELINE = 1,
    STATICFILTER = 2,
    STATICMEANFILTER = 3,
    ADAPTIVEFILTER = 4
};

std::string createMsg(const AlgorithmId algoId, const float value);
std::string createMsg(const std::int64_t deviceId, const AlgorithmId algoId, const float value);

std::optional<std::int64_t> getDeviceId(const std::string& deviceIdFilepath);

class Client {
    public: 
    Client(const std::string& ip, const std::int64_t port);
    ~Client();
    bool sendMessage(std::string& msg);

    private:
    std::string Ip;
    std::int64_t Port;
    int Socket_fd;
    
    void disconnect();
    bool connectToServer();
};
#endif