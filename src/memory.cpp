#include "memory.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <ctime>

namespace fs = std::filesystem;

// ── File helpers ──────────────────────────────────────────────────────────────

static std::string make_timestamp()
{
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[20];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d_%H-%M", &tm);
    return buf;
}

// Returns all .txt session files in dir, sorted by name (= chronological order).
static std::vector<fs::path> collect_session_files(const std::string& dir)
{
    std::vector<fs::path> files;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(dir, ec))
        if (entry.path().extension() == ".txt")
            files.push_back(entry.path());
    std::sort(files.begin(), files.end());
    return files;
}

static std::string read_file_text(const fs::path& p)
{
    std::ifstream f(p);
    if (!f.is_open()) return {};
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// Parse a .txt session file into Message pairs using the [User]: / [Zathas]: / --- format.
static std::vector<Message> parse_session_file(const fs::path& p)
{
    std::ifstream f(p);
    if (!f.is_open()) return {};

    enum class St { NONE, USER, ZATHAS };
    St state = St::NONE;
    std::string user_buf, zathas_buf;
    std::vector<Message> msgs;

    auto flush = [&]() {
        if (!user_buf.empty() && !zathas_buf.empty()) {
            msgs.push_back({"user",      user_buf});
            msgs.push_back({"assistant", zathas_buf});
        }
        user_buf.clear();
        zathas_buf.clear();
        state = St::NONE;
    };

    std::string line;
    while (std::getline(f, line)) {
        if (line == "---") {
            flush();
        } else if (line.rfind("[User]: ", 0) == 0) {
            user_buf = line.substr(8);
            state    = St::USER;
        } else if (line.rfind("[Zathas]: ", 0) == 0) {
            zathas_buf = line.substr(10);
            state      = St::ZATHAS;
        } else {
            if      (state == St::USER)   user_buf   += "\n" + line;
            else if (state == St::ZATHAS) zathas_buf += "\n" + line;
        }
    }
    flush();
    return msgs;
}

// Split summary.txt content into individual entries (each starts with "=== ... ===").
static std::vector<std::string> parse_summary_entries(const std::string& text)
{
    std::vector<std::string> entries;
    std::istringstream ss(text);
    std::string line, current;
    while (std::getline(ss, line)) {
        if (line.size() > 4 && line.substr(0, 4) == "=== ") {
            if (!current.empty()) entries.push_back(current);
            current = line + "\n";
        } else {
            current += line + "\n";
        }
    }
    if (!current.empty()) entries.push_back(current);
    return entries;
}

// ── ConversationMemory ────────────────────────────────────────────────────────

ConversationMemory::ConversationMemory(MemoryType type, int recent_depth, InferenceEngine* engine)
    : type_(type), recent_depth_(recent_depth), engine_(engine)
{
    dir_ = (type == MemoryType::PRIVATE) ? "./memory/private/" : "./memory/public/";
    fs::create_directories(dir_);
}

void ConversationMemory::record(const std::string& user_msg,
                                const std::string& assistant_msg)
{
    current_session_.push_back({"user",      user_msg});
    current_session_.push_back({"assistant", assistant_msg});
    save_session();
}

void ConversationMemory::save_session()
{
    if (current_session_.empty()) return;

    if (current_file_.empty())
        current_file_ = dir_ + make_timestamp() + ".txt";

    std::ofstream f(current_file_);
    if (!f.is_open()) {
        std::cerr << "[memory] Warning: could not write to " << current_file_ << "\n";
        return;
    }

    for (size_t i = 0; i + 1 < current_session_.size(); i += 2) {
        f << "[User]: "   << current_session_[i].content     << "\n"
          << "[Zathas]: " << current_session_[i + 1].content << "\n"
          << "---\n";
    }

    std::cout << "[memory] Saved " << current_session_.size() / 2
              << " exchanges to " << current_file_ << "\n";
}

void ConversationMemory::finalise()
{
    if (type_ != MemoryType::PUBLIC || engine_ == nullptr || current_session_.empty()) return;

    std::string session_text = current_file_.empty() ? "" : read_file_text(current_file_);
    if (session_text.empty()) return;

    std::cout << "[memory] Summarising public session for summary.txt...\n";

    static const char* kPrompt =
        "You are a summarisation assistant. Below is a conversation between a user and "
        "an AI assistant. Summarise the key topics discussed, questions asked, and any "
        "notable patterns in how the user communicated. Be concise. Output plain text only.";

    std::vector<Message> prompt = {{"user", std::string(kPrompt) + "\n\n" + session_text}};

    std::string summary;
    try {
        summary = engine_->generate(prompt, 300, 0.3f);
    } catch (const std::exception& e) {
        std::cerr << "[memory] Public summarisation failed: " << e.what() << "\n";
        return;
    }

    fs::path summary_path = fs::path(dir_) / "summary.txt";

    // Load existing entries, cap at 49 to make room for the new one
    static const int kMaxEntries = 50;
    std::string existing = fs::exists(summary_path) ? read_file_text(summary_path) : "";
    auto entries = parse_summary_entries(existing);
    if (static_cast<int>(entries.size()) >= kMaxEntries)
        entries.erase(entries.begin(), entries.begin() + (static_cast<int>(entries.size()) - kMaxEntries + 1));

    entries.push_back("=== " + make_timestamp() + " ===\n" + summary + "\n\n");

    std::ofstream f(summary_path);
    if (!f.is_open()) {
        std::cerr << "[memory] Warning: could not write " << summary_path.string() << "\n";
        return;
    }
    for (const auto& e : entries) f << e;
    std::cout << "[memory] Appended public summary (" << entries.size()
              << "/" << kMaxEntries << " entries)\n";
}

std::vector<Message> ConversationMemory::load_context() const
{
    auto files    = collect_session_files(dir_);
    int  total    = static_cast<int>(files.size());
    int  old_count = std::max(0, total - recent_depth_);

    std::vector<Message> result;

    // Layer 1 — compressed summaries of older sessions
    std::string summary_block;
    int summaries_loaded = 0;
    for (int i = 0; i < old_count; ++i) {
        fs::path sp = fs::path(files[i]).string() + ".summary";
        if (!fs::exists(sp)) continue;
        std::string text = read_file_text(sp);
        if (text.empty()) continue;
        summary_block += "[" + files[i].stem().string() + "]: " + text + "\n";
        ++summaries_loaded;
    }

    if (!summary_block.empty()) {
        result.push_back({"user",
            "The following are compressed summaries of past conversations for your memory:\n\n"
            + summary_block});
        result.push_back({"assistant",
            "Understood. I have reviewed my memories of past conversations."});
        std::cout << "[memory] Loaded " << summaries_loaded << " summary file(s)\n";
    }

    // Layer 2 — verbatim recent sessions
    int total_exchanges = 0;
    for (int i = old_count; i < total; ++i) {
        auto msgs = parse_session_file(files[i]);
        total_exchanges += static_cast<int>(msgs.size()) / 2;
        for (auto& m : msgs) result.push_back(std::move(m));
    }

    int n_recent = total - old_count;
    std::cout << "[memory] Loaded " << total_exchanges
              << " exchanges verbatim from " << n_recent << " recent session(s)\n";

    return result;
}

void ConversationMemory::summarise_old_sessions(InferenceEngine& engine, int max_tokens)
{
    if (type_ != MemoryType::PRIVATE) return;

    auto files     = collect_session_files(dir_);
    int  total     = static_cast<int>(files.size());
    int  old_count = std::max(0, total - recent_depth_);

    int generated = 0;
    for (int i = 0; i < old_count; ++i) {
        fs::path sp = fs::path(files[i]).string() + ".summary";
        if (fs::exists(sp)) continue;  // already summarised

        std::string session_text = read_file_text(files[i]);
        if (session_text.empty()) continue;

        std::cout << "[memory] Summarising " << files[i].filename().string() << "...\n";

        std::vector<Message> prompt = {{
            "user",
            "Summarise this conversation in 3-5 sentences. Capture the key topics, "
            "decisions made, personal details revealed, and anything that would help "
            "an AI assistant remember this person better. Be dense and specific.\n\n"
            + session_text
        }};

        try {
            std::string summary = engine.generate(prompt, max_tokens, 0.3f);

            std::ofstream sf(sp);
            if (sf.is_open()) {
                sf << summary;
                ++generated;
                std::cout << "[memory] Saved " << sp.filename().string() << "\n";
            } else {
                std::cerr << "[memory] Warning: could not write " << sp.string() << "\n";
            }
        } catch (const std::exception& e) {
            std::cerr << "[memory] Summarisation failed for "
                      << files[i].filename().string() << ": " << e.what() << "\n";
        }
    }

    if (generated > 0)
        std::cout << "[memory] Generated " << generated << " new summary file(s)\n";
}
