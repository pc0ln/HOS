#pragma once
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <chrono>

using Clock = std::chrono::system_clock;
using SysTime = std::chrono::time_point<Clock>;
using Hours = std::chrono::hours;

// Helpers
struct DateStamp {
    int y{}, m{}, d{};
};

struct DateTimeStamp {
    int y{}, m{}, d{}, H{}, M{};
};

inline bool operator==(const DateStamp& a, const DateStamp& b) {
    return a.y==b.y && a.m==b.m && a.d==b.d;
}

// Parser for "YYYY-MM-DD"
inline bool parse_date(const std::string& s, DateStamp& out) {
    if (s.size() != 10 || s[4]!='-' || s[7]!='-') return false;
    out.y = std::stoi(s.substr(0,4));
    out.m = std::stoi(s.substr(5,2));
    out.d = std::stoi(s.substr(8,2));
    return true;
}
// Parser for "YYY-MM-DDTHH:MM"
inline bool parse_datetime(const std::string& s, DateTimeStamp& out) {
    if (s.size() != 16 || s[4]!='-' || s[7]!='-' || s[10] != 'T' || s[13] != ':') return false;
    out.y = std::stoi(s.substr(0,4));
    out.m = std::stoi(s.substr(5,2));
    out.d = std::stoi(s.substr(8,2));
    out.H = std::stoi(s.substr(11,2));
    out.M = std::stoi(s.substr(14,2));
    return true;
}

// Convert DateTimeStamp to a rough time_point
inline SysTime to_time_point(const DateTimeStamp& dt) {
    std::tm tm{};
    tm.tm_year = dt.y - 1900;
    tm.tm_mon  = dt.m - 1;
    tm.tm_mday = dt.d;
    tm.tm_hour = dt.H;
    tm.tm_min  = dt.M;
    tm.tm_sec  = 0;
    auto t = std::mktime(&tm); // local
    return Clock::from_time_t(t);
}

inline DateStamp to_date(const DateTimeStamp& dt) { 
    return {dt.y, dt.m, dt.d}; 
}


// Structs for engine
struct Preferences {
    std::unordered_set<std::string> preferred_unit;
    bool avoid_nights = false; // default
};

struct Availability {
    DateStamp date{};
    bool can_work{};
};

struct Staff {
    // Identifiers
    std::string id;
    std::string name;
    std::string role;
    std::unordered_set<std::string> skills;
    // Filters
    short max_weekly_hours = 40;
    short max_consecutive_days = 5;
    short min_rest = 12;
    Preferences prefs;
    std::vector<Availability> availability;
    // Time tracking
    Hours assigned_weekly{0};
    short consecutive_days{0};
};

struct Shifts {
    // Identifiers
    std::string id;
    std::string name; // Unit
    std::string req_role;
    short required_count = 1; // default  
    SysTime start;
    SysTime end;

    // Time tracking
    Hours duration() const { // Length of shift
        return std::chrono::duration_cast<Hours>(end - start);
    }
    DateStamp day() const { // Reconstruct of the day
        std::time_t tt = Clock::to_time_t(start);
        std::tm* tm = std::localtime(&tt);
        return {tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday};
    }
    bool is_night() const { // Is it night (Naive: Night if shift starts after 12pm)
        std::time_t tt = Clock::to_time_t(start);
        std::tm* tm = std::localtime(&tt);
        return (tm->tm_hour >= 12);
    }
};


struct Rules {
    // shorts to save some memory since doesnt exceed 32k
    short max_hours_per_week_default = 40; // default weekly hour limit
    short max_consecutive_days_default = 5; // default limit for consecutive working days
    short min_rest_hours_default = 12; // default limit for rest between shifts
    std::unordered_set<std::string> hard_constraints; // Covereage, legal limits, Needs
    std::unordered_set<std::string> soft_constraints; // preferences, fairness
};

struct InputModel {
    std::vector<Staff> staff; // List of staff for schedules
    std::vector<Shifts> shifts; // List of shifts needed/available
    Rules rules; // Rules for the engine (Like max hours, preference, coverage)
};
