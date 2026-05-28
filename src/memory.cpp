#include "memory.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <ctime>

namespace fs = std::filesystem;

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

ConversationMemory::ConversationMemory(MemoryType type, int depth)
    : depth_(depth)
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

    // Each exchange: [User]: / [Zathas]: blocks terminated by ---
    for (size_t i = 0; i + 1 < current_session_.size(); i += 2) {
        f << "[User]: "   << current_session_[i].content     << "\n"
          << "[Zathas]: " << current_session_[i + 1].content << "\n"
          << "---\n";
    }

    std::cout << "[memory] Saved " << current_session_.size() / 2
              << " exchanges to " << current_file_ << "\n";
}

std::vector<Message> ConversationMemory::load_last_session() const
{
    std::vector<fs::path> files;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(dir_, ec))
        if (entry.path().extension() == ".txt")
            files.push_back(entry.path());

    if (files.empty()) return {};

    std::sort(files.begin(), files.end());

    int start = std::max(0, static_cast<int>(files.size()) - depth_);

    std::vector<Message> msgs;
    int total_exchanges = 0;

    for (int fi = start; fi < static_cast<int>(files.size()); ++fi) {
        std::ifstream f(files[fi]);
        if (!f.is_open()) continue;

        enum class St { NONE, USER, ZATHAS };
        St state = St::NONE;
        std::string user_buf, zathas_buf;

        auto flush_exchange = [&]() {
            if (!user_buf.empty() && !zathas_buf.empty()) {
                msgs.push_back({"user",      user_buf});
                msgs.push_back({"assistant", zathas_buf});
                ++total_exchanges;
            }
            user_buf.clear();
            zathas_buf.clear();
            state = St::NONE;
        };

        std::string line;
        while (std::getline(f, line)) {
            if (line == "---") {
                flush_exchange();
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
        flush_exchange();  // handle file with no trailing ---
    }

    int n = static_cast<int>(files.size()) - start;
    std::cout << "[memory] Loaded " << total_exchanges
              << " exchanges from " << n << " session(s)\n";
    return msgs;
}
