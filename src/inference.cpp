#include "inference.h"

#include <llama.h>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <cstring>

// ── Constructor / Destructor ───────────────────────────────────────────────────

InferenceEngine::InferenceEngine(const std::string& model_path,
                                 int n_ctx,
                                 int n_threads,
                                 int n_gpu_layers)
{
    llama_backend_init();

    llama_model_params mparams = llama_model_default_params();
    mparams.n_gpu_layers = n_gpu_layers;

    model_ = llama_load_model_from_file(model_path.c_str(), mparams);
    if (!model_) {
        throw std::runtime_error("Failed to load model from: " + model_path);
    }

    llama_context_params cparams = llama_context_default_params();
    cparams.n_ctx     = static_cast<uint32_t>(n_ctx);
    cparams.n_threads = static_cast<uint32_t>(n_threads);

    ctx_ = llama_new_context_with_model(model_, cparams);
    if (!ctx_) {
        llama_free_model(model_);
        model_ = nullptr;
        throw std::runtime_error("Failed to create llama context");
    }

    vocab_ = llama_model_get_vocab(model_);

    std::cout << "[inference] Model loaded: " << model_path << "\n";
}

InferenceEngine::~InferenceEngine()
{
    if (ctx_)   { llama_free(ctx_);         ctx_   = nullptr; }
    if (model_) { llama_free_model(model_); model_ = nullptr; }
    llama_backend_free();
}

// ── Prompt builder ─────────────────────────────────────────────────────────────
// Uses ChatML format — compatible with most GGUF instruction-tuned models.
std::string InferenceEngine::build_prompt(const std::vector<Message>& history) const
{
    // Llama 3 native format — <|eot_id|> is the real stop token for this model
    std::ostringstream oss;
    oss << "<|start_header_id|>system<|end_header_id|>\n\n"
        << "You are Zathas, an AI assistant. "
        << "Your own name is Zathas — not the user's name. "
        << "Be concise and helpful. Never break character.<|eot_id|>";
    for (const auto& msg : history) {
        oss << "<|start_header_id|>" << msg.role << "<|end_header_id|>\n\n"
            << msg.content << "<|eot_id|>";
    }
    oss << "<|start_header_id|>assistant<|end_header_id|>\n\n";
    return oss.str();
}

// ── Generation ─────────────────────────────────────────────────────────────────

std::string InferenceEngine::generate(const std::vector<Message>& history,
                                      int   max_tokens,
                                      float temperature,
                                      const TokenCallback& on_token,
                                      const DoneCallback&  on_done)
{
    if (!ctx_ || !model_) {
        throw std::runtime_error("Inference engine not initialized");
    }

    const std::string prompt = build_prompt(history);

    // Tokenize prompt
    const int n_prompt_tokens = static_cast<int>(prompt.size()) * 2 + 16; // upper bound
    std::vector<llama_token> tokens(n_prompt_tokens);
    const int n_tokens = llama_tokenize(
        vocab_,
        prompt.c_str(),
        static_cast<int32_t>(prompt.size()),
        tokens.data(),
        static_cast<int32_t>(tokens.size()),
        /*add_special=*/true,
        /*parse_special=*/true
    );
    if (n_tokens < 0) {
        throw std::runtime_error("Tokenization failed — prompt may be too long");
    }
    tokens.resize(n_tokens);

    // Reset KV cache and eval prompt
    llama_kv_self_clear(ctx_);

    llama_batch batch = llama_batch_get_one(tokens.data(), static_cast<int32_t>(tokens.size()));
    if (llama_decode(ctx_, batch) != 0) {
        throw std::runtime_error("llama_decode failed on prompt");
    }

    // Sampling params
    llama_sampler* sampler = llama_sampler_chain_init(llama_sampler_chain_default_params());
    llama_sampler_chain_add(sampler, llama_sampler_init_temp(temperature));
    llama_sampler_chain_add(sampler, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));

    std::string result;
    result.reserve(512);

    // Detect end tokens
    const llama_token eos_id  = llama_vocab_eos(vocab_);
    const llama_token eot_id  = llama_vocab_eot(vocab_);  // <|im_end|>

    for (int i = 0; i < max_tokens; ++i) {
        const llama_token new_token = llama_sampler_sample(sampler, ctx_, -1);
        llama_sampler_accept(sampler, new_token);

        if (new_token == eos_id || new_token == eot_id) break;

        // Convert token to string
        char buf[256];
        const int n = llama_token_to_piece(vocab_, new_token, buf, sizeof(buf) - 1, 0, true);
        if (n < 0) continue;
        buf[n] = '\0';

        const std::string piece(buf);
        result += piece;
        if (on_token) on_token(piece);

        // Feed token back into context for next step
        llama_token next_tokens[1] = { new_token };
        llama_batch next_batch = llama_batch_get_one(next_tokens, 1);
        if (llama_decode(ctx_, next_batch) != 0) break;
    }

    llama_sampler_free(sampler);

    if (on_done) on_done();
    return result;
}
