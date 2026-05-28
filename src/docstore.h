#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <sstream>
#include <iomanip>

class DocStore {
public:
    std::string store(const std::string& content) {
        const std::string id = make_id();
        std::lock_guard<std::mutex> lk(mu_);
        docs_[id] = content;
        return id;
    }

    std::string get(const std::string& id) const {
        std::lock_guard<std::mutex> lk(mu_);
        const auto it = docs_.find(id);
        return it != docs_.end() ? it->second : "";
    }

private:
    mutable std::mutex mu_;
    std::unordered_map<std::string, std::string> docs_;

    static std::string make_id() {
        using namespace std::chrono;
        const auto ns = duration_cast<nanoseconds>(
            system_clock::now().time_since_epoch()).count();
        std::ostringstream ss;
        ss << std::hex << ns;
        return ss.str();
    }
};
