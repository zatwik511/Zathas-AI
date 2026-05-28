#include "server.h"

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <csignal>

using json = nlohmann::json;

// Global pointer so the signal handler can flush memory on Ctrl-C
static ChatServer* g_server = nullptr;

static void on_signal(int)
{
    if (g_server) g_server->shutdown();
}

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
    : engine_(std::move(engine)), cfg_(config), memory_(MemoryType::PRIVATE, config.recent_depth)
{
    last_session_ = memory_.load_context();
}

void ChatServer::shutdown()
{
    memory_.save_session();
    svr_.stop();
}

void ChatServer::run()
{
    g_server = this;
    std::signal(SIGINT,  on_signal);
    std::signal(SIGTERM, on_signal);

    // ── GET /health ────────────────────────────────────────────────────────────
    svr_.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"status":"ok"})", "application/json");
    });

    // ── POST /api/chat ─────────────────────────────────────────────────────────
    svr_.Post("/api/chat", [this](const httplib::Request& req, httplib::Response& res) {
        json body;
        try {
            body = json::parse(req.body);
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", std::string("Invalid JSON: ") + e.what()}}.dump(),
                            "application/json");
            return;
        }

        if (!body.contains("message") || !body["message"].is_string()) {
            res.status = 400;
            res.set_content(json{{"error", "Missing or invalid 'message' field"}}.dump(),
                            "application/json");
            return;
        }

        const std::string user_msg = body["message"].get<std::string>();

        // Build history: inject last session as prior context, then current session turns
        std::vector<Message> history = last_session_;
        if (body.contains("history") && body["history"].is_array()) {
            try {
                for (const auto& m : parse_history(body["history"]))
                    history.push_back(m);
            } catch (...) {}
        }
        history.push_back({"user", user_msg});

        try {
            std::string response = engine_->generate(
                history, cfg_.max_tokens, cfg_.temperature
            );

            // Log this exchange to memory
            memory_.record(user_msg, response);

            res.set_content(json{{"response", response}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(json{{"error", std::string("Inference error: ") + e.what()}}.dump(),
                            "application/json");
        }
    });

    // ── Serve static frontend files ────────────────────────────────────────────
    svr_.set_mount_point("/", cfg_.static_dir);

    svr_.Get(".*", [&](const httplib::Request&, httplib::Response& res) {
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

    svr_.listen(cfg_.host, cfg_.port);

    // Reached when svr_.stop() is called from shutdown()
    memory_.save_session();
    memory_.finalise();                        // PUBLIC: append to summary.txt; PRIVATE: no-op
    memory_.summarise_old_sessions(*engine_);  // PRIVATE: compress old sessions; PUBLIC: no-op
    std::cout << "[server] Goodbye.\n";
}
