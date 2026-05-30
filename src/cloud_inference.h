#pragma once
#include "inference.h"
#include <string>

// Calls a Groq-compatible OpenAI chat/completions endpoint over HTTPS.
// Drop-in replacement for InferenceEngine on the public route.
class CloudInferenceEngine : public IInferenceEngine {
public:
    CloudInferenceEngine(std::string api_key,
                         std::string model = "llama-3.3-70b-versatile",
                         std::string host  = "api.groq.com");

    std::string generate(const ContextLayers& ctx,
                         int              max_tokens  = 512,
                         float            temperature = 0.7f,
                         const TokenCallback& on_token = {},
                         const DoneCallback&  on_done  = {}) override;

private:
    std::string api_key_;
    std::string model_;
    std::string host_;
};
