#pragma once
#include <string>
#include <vector>
#include <functional>

struct Message {
    std::string role;    // "user" or "assistant"
    std::string content;
};

// Callbacks used during streaming generation.
// on_token: called for each new token string.
// on_done:  called when generation is complete.
using TokenCallback = std::function<void(const std::string& token)>;
using DoneCallback  = std::function<void()>;

class InferenceEngine {
public:
    explicit InferenceEngine(const std::string& model_path,
                             int   n_ctx       = 4096,
                             int   n_threads    = 4,
                             int   n_gpu_layers = 0);
    ~InferenceEngine();

    // Non-copyable
    InferenceEngine(const InferenceEngine&)            = delete;
    InferenceEngine& operator=(const InferenceEngine&) = delete;

    // Generate a response given a conversation history.
    // Calls on_token for each token produced, then on_done when finished.
    // Returns the full generated text.
    std::string generate(const std::vector<Message>& history,
                         int              max_tokens  = 512,
                         float            temperature = 0.7f,
                         const TokenCallback& on_token = {},
                         const DoneCallback&  on_done  = {});

    bool is_loaded() const { return ctx_ != nullptr; }

private:
    struct llama_model* model_   = nullptr;
    struct llama_context* ctx_   = nullptr;
    const struct llama_vocab* vocab_ = nullptr;

    std::string build_prompt(const std::vector<Message>& history) const;
};
