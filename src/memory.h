#pragma once
#include "inference.h"
#include <string>
#include <vector>

enum class MemoryType { PRIVATE, PUBLIC };

// Two-layer persistent memory:
//   Layer 1 — verbatim: the last `recent_depth` session .txt files
//   Layer 2 — compressed: .summary files for all older sessions
// Sessions are saved as timestamped .txt files under ./memory/private/ or ./memory/public/.
class ConversationMemory {
public:
    // engine: if non-null and type==PUBLIC, finalise() appends a summary to summary.txt.
    //         if non-null and type==PRIVATE, summarise_old_sessions() uses it.
    ConversationMemory(MemoryType type, int recent_depth = 10, InferenceEngine* engine = nullptr);

    // Append a completed exchange and flush to disk immediately.
    void record(const std::string& user_msg, const std::string& assistant_msg);

    // Write current session to its timestamp file (overwrites on repeat calls).
    void save_session();

    // PUBLIC + engine provided: summarise the current session and append to summary.txt.
    // PRIVATE or no engine: no-op. Call at shutdown after save_session().
    void finalise();

    // Build two-layer context: summaries of old sessions + verbatim recent sessions.
    std::vector<Message> load_context() const;

    // PRIVATE only: summarise sessions older than recent_depth.
    // Writes a .summary file alongside each original .txt. Skips already-summarised files.
    void summarise_old_sessions(InferenceEngine& engine, int max_tokens = 300);

private:
    std::string          dir_;
    MemoryType           type_;
    int                  recent_depth_;
    InferenceEngine*     engine_;        // non-owning; may be null
    std::vector<Message> current_session_;
    std::string          current_file_;
};
