#pragma once
#include "inference.h"
#include <string>
#include <vector>

// Persists conversation sessions to a JSON file on disk.
// Each session is a list of {role, content} message pairs.
class ConversationMemory {
public:
    explicit ConversationMemory(const std::string& file_path);

    // Append a completed exchange (user + assistant turn) to the current session.
    void record(const std::string& user_msg, const std::string& assistant_msg);

    // Flush the current session to disk and start a new one.
    void save_session();

    // Return messages from the most recent saved session to inject as prior context.
    std::vector<Message> load_last_session() const;

private:
    std::string          path_;
    std::vector<Message> current_session_;
    int                  session_slot_ = -1;  // index of this session in the file
};
