#pragma once
#include "inference.h"
#include "memory.h"
#include "docstore.h"
#include "analytics.h"
#include <httplib.h>
#include <string>
#include <memory>

struct ServerConfig {
    std::string host         = "0.0.0.0";
    int         port         = 8080;
    std::string static_dir   = "./frontend";
    int         recent_depth = 10;
    int         max_tokens   = 512;
    float       temperature  = 0.7f;
    std::string prime_token;   // read from PRIME_TOKEN in .env; empty = no auth required
};

class ChatServer {
public:
    // private_engine: local llama.cpp model (may be nullptr on cloud-only deploys).
    // public_engine:  used for /api/chat — may be cloud or the same local engine.
    ChatServer(std::shared_ptr<InferenceEngine>  private_engine,
               std::shared_ptr<IInferenceEngine> public_engine,
               const ServerConfig& config);

    // Blocks until the server stops (Ctrl-C / SIGINT).
    void run();

    // Flush both memory instances and stop — called from signal handler.
    void shutdown();

private:
    std::shared_ptr<InferenceEngine>  private_engine_;  // local; nullptr = prime disabled
    std::shared_ptr<IInferenceEngine> public_engine_;   // cloud or local
    ServerConfig                      cfg_;
    ConversationMemory                private_memory_;
    ConversationMemory                public_memory_;
    std::vector<Message>              private_history_;
    std::string                       public_summary_;
    DocStore                          doc_store_;
    AnalyticsDB                       analytics_;
    httplib::Server                   svr_;
};
