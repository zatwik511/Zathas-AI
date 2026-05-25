#pragma once
#include "inference.h"
#include <string>
#include <memory>

struct ServerConfig {
    std::string host        = "0.0.0.0";
    int         port        = 8080;
    std::string static_dir  = "./frontend";  // served at GET /
    int         max_tokens  = 512;
    float       temperature = 0.7f;
};

class ChatServer {
public:
    ChatServer(std::shared_ptr<InferenceEngine> engine, const ServerConfig& config);

    // Blocks until the server stops (Ctrl-C / SIGINT).
    void run();

private:
    std::shared_ptr<InferenceEngine> engine_;
    ServerConfig cfg_;
};
