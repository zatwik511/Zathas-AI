#pragma once
#include "inference.h"
#include <string>
#include <vector>

enum class MemoryType { PRIVATE, PUBLIC };

// Persists conversation sessions as human-readable .txt files under
// ./memory/private/ or ./memory/public/, named by timestamp.
// On startup, loads the last `depth` session files as prior context.
class ConversationMemory {
public:
    ConversationMemory(MemoryType type, int depth = 5);

    // Append a completed exchange and flush to disk immediately.
    void record(const std::string& user_msg, const std::string& assistant_msg);

    // Write current session to its timestamp file (overwrites on repeat calls).
    void save_session();

    // Return messages from the last `depth` session files.
    std::vector<Message> load_last_session() const;

private:
    std::string          dir_;
    int                  depth_;
    std::vector<Message> current_session_;
    std::string          current_file_;   // set on first save, reused for the session
};
