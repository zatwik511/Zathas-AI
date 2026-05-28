#include "analytics.h"
#include <sqlite3.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <iostream>
#include <mutex>

using json = nlohmann::json;

namespace {
    std::mutex g_mu;
}

AnalyticsDB::AnalyticsDB()
{
    std::filesystem::create_directories("./data");

    if (sqlite3_open("./data/analytics.db", &db_) != SQLITE_OK) {
        std::cerr << "[analytics] Failed to open DB: " << sqlite3_errmsg(db_) << "\n";
        sqlite3_close(db_);
        db_ = nullptr;
        return;
    }
    init_schema();
    std::cout << "[analytics] Database ready at ./data/analytics.db\n";
}

AnalyticsDB::~AnalyticsDB()
{
    if (db_) sqlite3_close(db_);
}

void AnalyticsDB::init_schema()
{
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS requests (
            id       INTEGER PRIMARY KEY AUTOINCREMENT,
            ts       INTEGER NOT NULL,
            route    TEXT    NOT NULL,
            msg_len  INTEGER NOT NULL,
            resp_len INTEGER NOT NULL,
            gen_ms   INTEGER NOT NULL,
            has_doc  INTEGER NOT NULL
        );
        CREATE INDEX IF NOT EXISTS idx_ts    ON requests(ts);
        CREATE INDEX IF NOT EXISTS idx_route ON requests(route);
    )";
    char* err = nullptr;
    if (sqlite3_exec(db_, sql, nullptr, nullptr, &err) != SQLITE_OK) {
        std::cerr << "[analytics] Schema error: " << err << "\n";
        sqlite3_free(err);
    }
}

void AnalyticsDB::log_request(const std::string& route,
                              int msg_len, int resp_len,
                              long long gen_ms, bool has_doc)
{
    if (!db_) return;
    std::lock_guard<std::mutex> lk(g_mu);

    const char* sql =
        "INSERT INTO requests (ts, route, msg_len, resp_len, gen_ms, has_doc) "
        "VALUES (strftime('%s','now'), ?, ?, ?, ?, ?)";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return;
    sqlite3_bind_text(stmt, 1, route.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int (stmt, 2, msg_len);
    sqlite3_bind_int (stmt, 3, resp_len);
    sqlite3_bind_int64(stmt, 4, gen_ms);
    sqlite3_bind_int (stmt, 5, has_doc ? 1 : 0);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::string AnalyticsDB::get_stats() const
{
    if (!db_) return json{{"error", "database unavailable"}}.dump();
    std::lock_guard<std::mutex> lk(g_mu);

    json result;

    // ── Totals ─────────────────────────────────────────────────────────────────
    {
        sqlite3_stmt* s = nullptr;
        sqlite3_prepare_v2(db_,
            "SELECT COUNT(*), "
            "       SUM(CASE WHEN route='public' THEN 1 ELSE 0 END), "
            "       SUM(CASE WHEN route='prime'  THEN 1 ELSE 0 END), "
            "       AVG(gen_ms), "
            "       AVG(msg_len) "
            "FROM requests",
            -1, &s, nullptr);
        if (sqlite3_step(s) == SQLITE_ROW) {
            result["total"]        = sqlite3_column_int(s, 0);
            result["total_public"] = sqlite3_column_int(s, 1);
            result["total_prime"]  = sqlite3_column_int(s, 2);
            result["avg_gen_ms"]   = sqlite3_column_type(s, 3) != SQLITE_NULL
                                     ? sqlite3_column_double(s, 3) : 0.0;
            result["avg_msg_len"]  = sqlite3_column_type(s, 4) != SQLITE_NULL
                                     ? sqlite3_column_double(s, 4) : 0.0;
        }
        sqlite3_finalize(s);
    }

    // ── Per-day (last 30 days) ─────────────────────────────────────────────────
    {
        sqlite3_stmt* s = nullptr;
        sqlite3_prepare_v2(db_,
            "SELECT date(ts,'unixepoch') AS day, "
            "       SUM(CASE WHEN route='public' THEN 1 ELSE 0 END) AS pub, "
            "       SUM(CASE WHEN route='prime'  THEN 1 ELSE 0 END) AS prv "
            "FROM requests "
            "WHERE ts >= strftime('%s','now','-30 days') "
            "GROUP BY day ORDER BY day",
            -1, &s, nullptr);
        json days = json::array();
        while (sqlite3_step(s) == SQLITE_ROW) {
            days.push_back({
                {"day",    std::string(reinterpret_cast<const char*>(sqlite3_column_text(s, 0)))},
                {"public", sqlite3_column_int(s, 1)},
                {"prime",  sqlite3_column_int(s, 2)}
            });
        }
        result["per_day"] = std::move(days);
        sqlite3_finalize(s);
    }

    // ── Per-hour (UTC, all time) ───────────────────────────────────────────────
    {
        sqlite3_stmt* s = nullptr;
        sqlite3_prepare_v2(db_,
            "SELECT CAST(strftime('%H', ts, 'unixepoch') AS INTEGER) AS hr, COUNT(*) "
            "FROM requests GROUP BY hr ORDER BY hr",
            -1, &s, nullptr);

        json hours = json::array();
        for (int i = 0; i < 24; ++i) hours.push_back({{"hour", i}, {"count", 0}});

        while (sqlite3_step(s) == SQLITE_ROW) {
            int hr = sqlite3_column_int(s, 0);
            if (hr >= 0 && hr < 24)
                hours[hr]["count"] = sqlite3_column_int(s, 1);
        }
        result["per_hour"] = std::move(hours);
        sqlite3_finalize(s);
    }

    // ── Peak hour ─────────────────────────────────────────────────────────────
    {
        int peak_hr = 0, peak_count = 0;
        for (const auto& h : result["per_hour"]) {
            int c = h["count"].get<int>();
            if (c > peak_count) { peak_count = c; peak_hr = h["hour"].get<int>(); }
        }
        result["peak_hour"] = peak_hr;
    }

    return result.dump();
}
