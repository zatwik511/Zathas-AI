#include "memory.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

ConversationMemory::ConversationMemory(const std::string& file_path)
    : path_(file_path)
{}

void ConversationMemory::record(const std::string& user_msg,
                                const std::string& assistant_msg)
{
    current_session_.push_back({"user",      user_msg});
    current_session_.push_back({"assistant", assistant_msg});
    save_session();  // persist after every exchange so hard kills don't lose data
}

void ConversationMemory::save_session()
{
    if (current_session_.empty()) return;

    // Load all sessions, replace or append the current one (last slot)
    json all_sessions = json::array();
    {
        std::ifstream f(path_);
        if (f.is_open()) {
            try { all_sessions = json::parse(f); }
            catch (...) { all_sessions = json::array(); }
        }
    }

    // Rebuild current session as JSON
    json session = json::array();
    for (const auto& msg : current_session_) {
        session.push_back({ {"role", msg.role}, {"content", msg.content} });
    }

    // Overwrite the last slot if it's the same session (we write after every turn),
    // otherwise append a new one.
    if (!all_sessions.empty() && session_slot_ == static_cast<int>(all_sessions.size()) - 1) {
        all_sessions.back() = session;
    } else {
        all_sessions.push_back(session);
        session_slot_ = static_cast<int>(all_sessions.size()) - 1;
    }

    // Keep only the last 20 sessions
    if (all_sessions.size() > 20) {
        all_sessions.erase(all_sessions.begin(),
                           all_sessions.begin() + (static_cast<int>(all_sessions.size()) - 20));
        session_slot_ = 19;
    }

    std::ofstream f(path_);
    if (f.is_open()) {
        f << all_sessions.dump(2);
        std::cout << "[memory] Saved " << current_session_.size() / 2
                  << " exchanges to " << path_ << "\n";
    } else {
        std::cerr << "[memory] Warning: could not write to " << path_ << "\n";
    }
}

std::vector<Message> ConversationMemory::load_last_session() const
{
    std::ifstream f(path_);
    if (!f.is_open()) return {};

    json all_sessions;
    try { all_sessions = json::parse(f); }
    catch (...) { return {}; }

    if (all_sessions.empty()) return {};

    const auto& last = all_sessions.back();
    std::vector<Message> msgs;
    for (const auto& m : last) {
        msgs.push_back({ m.at("role").get<std::string>(),
                         m.at("content").get<std::string>() });
    }
    std::cout << "[memory] Loaded " << msgs.size() / 2
              << " exchanges from previous session\n";
    return msgs;
}
