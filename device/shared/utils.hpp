#ifndef CODE_SHARED_UTILS_HPP
#define CODE_SHARED_UTILS_HPP

#include <string>
#include <cstdint>

enum AlgorithmId {
    BASELINE = 1
};

std::string createMsg(const AlgorithmId algoId, const float value);

class Client {
    public: 
    Client(const std::string& ip, const std::int64_t port);
    ~Client();
    bool connectToServer();
    bool sendMessage(std::string& msg);

    private:
    std::string Ip;
    std::int64_t Port;
    int socket_fd;
};

#endif