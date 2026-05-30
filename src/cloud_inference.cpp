#include "cloud_inference.h"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <stdexcept>

using json = nlohmann::json;

CloudInferenceEngine::CloudInferenceEngine(std::string api_key,
                                           std::string model,
                                           std::string host)
    : api_key_(std::move(api_key))
    , model_(std::move(model))
    , host_(std::move(host))
{
    std::cout << "[cloud] Engine ready — model: " << model_ << "\n";
}

// ── Convert ContextLayers to OpenAI messages array ────────────────────────────

static json build_messages(const ContextLayers& ctx)
{
    json msgs = json::array();

    // System turn — merge system prompt + public summary
    std::string sys = ctx.system_prompt;
    if (!ctx.public_summary.empty())
        sys += "\n\n" + ctx.public_summary;
    msgs.push_back({{"role", "system"}, {"content", sys}});

    // Private history (already-completed sessions)
    for (const auto& m : ctx.private_history)
        msgs.push_back({{"role", m.role}, {"content", m.content}});

    // Document injection
    if (!ctx.document.empty()) {
        const std::string doc = ctx.document.size() > 8000
            ? ctx.document.substr(0, 8000) + "\n[document truncated]"
            : ctx.document;
        msgs.push_back({{"role", "user"}, {"content", "<document>\n" + doc + "\n</document>"}});
        msgs.push_back({{"role", "assistant"}, {"content", "Understood. I have read the uploaded document."}});
    }

    // Current session
    for (const auto& m : ctx.current_session)
        msgs.push_back({{"role", m.role}, {"content", m.content}});

    return msgs;
}

// ── SSE chunk parser ──────────────────────────────────────────────────────────

struct SSEParser {
    std::string              buf;
    std::string              full_response;
    const TokenCallback&     on_token;
    bool                     done = false;

    explicit SSEParser(const TokenCallback& cb) : on_token(cb) {}

    // Feed raw bytes; returns false to abort the connection.
    bool feed(const char* data, size_t len) {
        buf.append(data, len);
        size_t pos;
        while ((pos = buf.find("\n\n")) != std::string::npos) {
            const std::string event = buf.substr(0, pos);
            buf = buf.substr(pos + 2);

            if (event.size() < 6 || event.substr(0, 6) != "data: ") continue;
            const std::string payload = event.substr(6);

            if (payload == "[DONE]") { done = true; continue; }

            try {
                const auto j = json::parse(payload);
                const auto& choices = j.at("choices");
                if (!choices.empty()) {
                    const auto& delta = choices[0].at("delta");
                    if (delta.contains("content") && delta["content"].is_string()) {
                        const std::string tok = delta["content"].get<std::string>();
                        if (!tok.empty()) {
                            full_response += tok;
                            if (on_token) on_token(tok);
                        }
                    }
                }
            } catch (...) { /* skip malformed chunk */ }
        }
        return true;
    }
};

// ── Main generate ─────────────────────────────────────────────────────────────

std::string CloudInferenceEngine::generate(
    const ContextLayers& ctx,
    int   max_tokens,
    float temperature,
    const TokenCallback& on_token,
    const DoneCallback&  on_done)
{
    const json body = {
        {"model",       model_},
        {"messages",    build_messages(ctx)},
        {"stream",      true},
        {"max_tokens",  max_tokens},
        {"temperature", temperature}
    };

    httplib::SSLClient cli(host_);
    cli.set_connection_timeout(15);
    cli.set_read_timeout(120);

    SSEParser parser(on_token);

    // Use low-level send() so we can attach a streaming ContentReceiver to a POST.
    httplib::Request req;
    req.method = "POST";
    req.path   = "/openai/v1/chat/completions";
    req.set_header("Authorization", "Bearer " + api_key_);
    req.set_header("Content-Type",  "application/json");
    req.body = body.dump();
    req.content_receiver = [&](const char* data, size_t len,
                                uint64_t /*offset*/, uint64_t /*total*/) {
        return parser.feed(data, len);
    };

    httplib::Response resp;
    httplib::Error    err = httplib::Error::Success;
    if (!cli.send(req, resp, err)) {
        throw std::runtime_error("Cloud API connection failed: " +
                                 httplib::to_string(err));
    }
    if (resp.status != 200) {
        throw std::runtime_error("Cloud API returned HTTP " +
                                 std::to_string(resp.status) + ": " + resp.body);
    }

    if (on_done) on_done();
    return parser.full_response;
}
