#pragma once
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>



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
};

struct Shifts {
    // Identifiers
    std::string id;
    std::string name;
    std::string req_role;
    short required_count = 1; // default  
};

struct Preferences {
    std::unordered_set<std::string> preferred_unit;
    bool avoid_nights = false; // default
};

struct Availability {
    // Date
    bool can_work{};
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
