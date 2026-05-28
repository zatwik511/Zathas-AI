#pragma once
#include <string>

struct sqlite3;

class AnalyticsDB {
public:
    AnalyticsDB();
    ~AnalyticsDB();

    void log_request(const std::string& route,
                     int msg_len, int resp_len,
                     long long gen_ms, bool has_doc);

    std::string get_stats() const;  // returns JSON

private:
    sqlite3* db_ = nullptr;
    void init_schema();
};
