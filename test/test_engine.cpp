#include "../src/model.hpp"
#include "../src/engine.hpp"
#include <cassert>
#include <iostream>

// Helper to build a time point easily
static SysTime make_time(int year, int month, int day, int hour, int minute) {
    DateTimeStamp dt{year, month, day, hour, minute};
    return to_time_point(dt);
}

int main() {
    // ---- Test 1: prefers non-night-avoiding nurse on night shift ----
    {
        InputModel m;

        Staff a;
        a.id = "nurse_night_ok";
        a.name = "Night OK";
        a.role = "RN";
        a.max_weekly_hours = 40;
        a.min_rest = 0;
        a.prefs.avoid_nights = false;

        Staff b;
        b.id = "nurse_avoid_nights";
        b.name = "Avoids Nights";
        b.role = "RN";
        b.max_weekly_hours = 40;
        b.min_rest = 0;
        b.prefs.avoid_nights = true;

        // Always available (no availability entries => default true)

        m.staff.push_back(a);
        m.staff.push_back(b);

        Shifts sh;
        sh.id = "night_shift";
        sh.name = "ICU";
        sh.start = make_time(2025, 4, 1, 22, 0); // 10pm
        sh.end   = make_time(2025, 4, 2, 6, 0);  // 6am
        sh.req_role = "RN";
        sh.required_count = 1;

        m.shifts.push_back(sh);

        EngineOptions opt;
        opt.fairness_on = true;
        opt.respect_preferences = true;

        auto res = build_schedule(m, opt);
        assert(res.assignments.size() == 1);
        const auto& asg = res.assignments[0];
        assert(asg.staff_ids.size() == 1);
        // Should pick the nurse who does NOT avoid nights
        assert(asg.staff_ids[0] == "nurse_night_ok");
    }

    // ---- Test 2: respect max_hours_per_week ----
    {
        InputModel m;

        Staff lowCap;
        lowCap.id = "low_cap";
        lowCap.role = "RN";
        lowCap.max_weekly_hours = 8; // can only do 8h
        lowCap.min_rest = 0;

        Staff highCap;
        highCap.id = "high_cap";
        highCap.role = "RN";
        highCap.max_weekly_hours = 40;
        highCap.min_rest = 0;

        m.staff.push_back(lowCap);
        m.staff.push_back(highCap);

        Shifts sh;
        sh.id = "long_shift";
        sh.name = "ICU";
        sh.start = make_time(2025, 4, 1, 7, 0);
        sh.end   = make_time(2025, 4, 1, 19, 0); // 12h
        sh.req_role = "RN";
        sh.required_count = 1;

        m.shifts.push_back(sh);

        EngineOptions opt;
        auto res = build_schedule(m, opt);
        assert(res.assignments.size() == 1);
        const auto& asg = res.assignments[0];
        // lowCap cannot take 12h with an 8h cap, so highCap must be chosen
        assert(asg.staff_ids.size() == 1);
        assert(asg.staff_ids[0] == "high_cap");
    }

    // ---- Test 3: required_count > number of staff ----
    {
        InputModel m;

        Staff a;
        a.id = "nurse1";
        a.role = "RN";
        a.max_weekly_hours = 40;
        a.min_rest = 0;

        m.staff.push_back(a);

        Shifts sh;
        sh.id = "short_staffed";
        sh.name = "ICU";
        sh.start = make_time(2025, 4, 1, 7, 0);
        sh.end   = make_time(2025, 4, 1, 15, 0);
        sh.req_role = "RN";
        sh.required_count = 2; // needs 2 but only 1 exists

        m.shifts.push_back(sh);

        EngineOptions opt;
        auto res = build_schedule(m, opt);
        assert(res.assignments.size() == 1);
        const auto& asg = res.assignments[0];
        assert(asg.staff_ids.size() == 1); // only 1 assigned
        assert(asg.staff_ids[0] == "nurse1");
        // Should have a warning about short coverage
        assert(!res.warnings.empty());
    }

    std::cout << "engine_tests: all tests passed.\n";
    return 0;
}
