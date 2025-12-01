#include "input_parser.hpp"
#include "../extern/json.hpp" // Using json header from nlohmann/json

using nlohmann::json;

// Helper to set from json
static std::unordered_set<std::string> to_set(const json& arr) {
    std::unordered_set<std::string> s;
    if (!arr.is_array()) return s;
    for (auto& v : arr) s.insert(v.get<std::string>());
    return s;
}

InputModel parse_input_json(const std::string& text, const Filters& opt) {
    json j = json::parse(text);

    InputModel i_model;

    // Rules
    if (j.contains("rules")) {
        auto r = j["rules"];
        i_model.rules.max_hours_per_week_default = r.value("max_hours_per_week_default", 40);
        i_model.rules.max_consecutive_days_default = r.value("max_consecutive_days_default", 5);
        i_model.rules.min_rest_hours_default = r.value("min_rest_hours_default", 12);
        if (r.contains("hard_constraints")) {
            i_model.rules.hard_constraints = to_set(r["hard_constraints"]);
        }
        if (r.contains("soft_constraints")) {
            i_model.rules.soft_constraints = to_set(r["soft_constraints"]);
        }
    }
    // Staff
    for (auto& s : j.at("staff")) {
        Staff stf;
        stf.id = s.at("id").get<std::string>();
        stf.name = s.value("name", stf.id);
        stf.role = s.value("role", "");
        if (s.contains("skills")) {
            for (auto& sk : s["skills"]) {
                stf.skills.insert(sk.get<std::string>());
            }
        }

        stf.max_weekly_hours = s.value("max_weekly_hours", i_model.rules.max_hours_per_week_default);
        stf.max_consecutive_days = s.value("max_consecutive_days", i_model.rules.max_consecutive_days_default);
        stf.min_rest = s.value("min_rest", i_model.rules.min_rest_hours_default);

        if (s.contains("preferences")) {
            auto p = s["preferences"];
            stf.prefs.avoid_nights = p.value("avoid_nights", false);
            if (p.contains("preferred_unit")) {
                for (auto& u : p["preferred_unit"]) {
                    stf.prefs.preferred_unit.insert(u.get<std::string>());
                }
            }
        }

        if (s.contains("availability")) {
            for (auto& a : s["availability"]) {
                Availability av;
                DateStamp ds;
                if (!parse_date(a.at("date").get<std::string>(), ds)) {
                    throw std::runtime_error("Bad availability date.");
                }
                av.date = ds;
                av.can_work = a.value("can_work", true);
                stf.availability.push_back(av);
            }
        }

        i_model.staff.push_back(std::move(stf));

        // Shifts
        for (auto& sh : j.at("shifts")) {
            Shifts shf;
            shf.id   = sh.at("id").get<std::string>();
            shf.name = sh.value("name", "");
            if (!opt.only_shown.empty() && shf.name != opt.only_shown) {
                continue; // Applying filter
            }

            {
                DateTimeStamp dt{};
                if (!parse_datetime(sh.at("start").get<std::string>(), dt)) throw std::runtime_error("Bad shift start");
                shf.start = to_time_point(dt);
            }
            {
                DateTimeStamp dt{};
                if (!parse_datetime(sh.at("end").get<std::string>(), dt)) throw std::runtime_error("Bad shift end");
                shf.end = to_time_point(dt);
            }

            shf.req_role  = sh.value("req_role", "");
            shf.required_count = sh.value("required_count", 1);
            i_model.shifts.push_back(std::move(shf)); 
        }

    }

    return i_model;

}
