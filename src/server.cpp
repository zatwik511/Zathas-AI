#include "server.h"

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <iostream>
#include <stdexcept>

using json = nlohmann::json;

// ── Helper: parse message array from JSON ──────────────────────────────────────
static std::vector<Message> parse_history(const json& j)
{
    std::vector<Message> history;
    for (const auto& item : j) {
        history.push_back({
            item.at("role").get<std::string>(),
            item.at("content").get<std::string>()
        });
    }
    return history;
}

// ── ChatServer ─────────────────────────────────────────────────────────────────

ChatServer::ChatServer(std::shared_ptr<InferenceEngine> engine, const ServerConfig& config)
    : engine_(std::move(engine)), cfg_(config)
{}

void ChatServer::run()
{
    httplib::Server svr;

    // ── GET /health ────────────────────────────────────────────────────────────
    svr.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"status":"ok"})", "application/json");
    });

    // ── POST /api/chat ─────────────────────────────────────────────────────────
    svr.Post("/api/chat", [this](const httplib::Request& req, httplib::Response& res) {
        // Parse request body
        json body;
        try {
            body = json::parse(req.body);
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", std::string("Invalid JSON: ") + e.what()}}.dump(),
                            "application/json");
            return;
        }

        // Validate required fields
        if (!body.contains("message") || !body["message"].is_string()) {
            res.status = 400;
            res.set_content(json{{"error", "Missing or invalid 'message' field"}}.dump(),
                            "application/json");
            return;
        }

        // Build history: prior turns + current user message
        std::vector<Message> history;
        if (body.contains("history") && body["history"].is_array()) {
            try { history = parse_history(body["history"]); }
            catch (...) { /* ignore malformed history */ }
        }
        history.push_back({"user", body["message"].get<std::string>()});

        // Run inference
        try {
            std::string response = engine_->generate(
                history,
                cfg_.max_tokens,
                cfg_.temperature
            );

            res.set_content(
                json{{"response", response}}.dump(),
                "application/json"
            );
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(json{{"error", std::string("Inference error: ") + e.what()}}.dump(),
                            "application/json");
        }
    });

    // ── Serve static frontend files ────────────────────────────────────────────
    svr.set_mount_point("/", cfg_.static_dir);

    // Fallback: serve index.html for any unmatched GET (SPA-style)
    svr.Get(".*", [&](const httplib::Request&, httplib::Response& res) {
        const std::string index_path = cfg_.static_dir + "/index.html";
        std::ifstream f(index_path);
        if (f) {
            std::string content((std::istreambuf_iterator<char>(f)),
                                 std::istreambuf_iterator<char>());
            res.set_content(content, "text/html");
        } else {
            res.status = 404;
            res.set_content("Not found", "text/plain");
        }
    });

    std::cout << "[server] Zathas AI listening on http://"
              << cfg_.host << ":" << cfg_.port << "\n";
    std::cout << "[server] Press Ctrl-C to stop.\n";

    svr.listen(cfg_.host, cfg_.port);
}
