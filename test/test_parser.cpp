#include "../src/input_parser.hpp"
#include <cassert>
#include <iostream>

int main() {
    // Sample JSON similar to our structure
    const char* json_text = R"json(
{
  "staff": [
    {
      "id": "nurse1",
      "name": "Alice",
      "role": "RN",
      "skills": ["ICU"],
      "max_weekly_hours": 40,
      "max_consecutive_days": 5,
      "min_rest": 12,
      "preferences": {
        "avoid_nights": true,
        "preferred_unit": ["ICU", "ER"]
      },
      "availability": [
        { "date": "2025-04-01", "can_work": true },
        { "date": "2025-04-02", "can_work": false }
      ]
    }
  ],
  "shifts": [
    {
      "id": "shift1",
      "name": "ICU",
      "start": "2025-04-01T07:00",
      "end": "2025-04-01T19:00",
      "required_role": "RN",
      "required_count": 3
    },
    {
      "id": "shift2",
      "name": "ER",
      "start": "2025-04-01T07:00",
      "end": "2025-04-01T15:00",
      "required_role": "RN",
      "required_count": 2
    }
  ],
  "rules": {
    "max_hours_per_week_default": 40,
    "max_consecutive_days_default": 5,
    "min_rest_hours_default": 12,
    "hard_constraints": ["coverage", "legal_limits"],
    "soft_constraints": ["preferences", "fairness"]
  }
}
)json";

    // --- Test 1: basic parsing, no unit filter ---
    {
        Filters opt;
        auto model = parse_input_json(json_text, opt);

        // staff
        assert(model.staff.size() == 1);
        const auto& s = model.staff[0];
        assert(s.id == "nurse1");
        assert(s.name == "Alice");
        assert(s.role == "RN");
        assert(s.skills.count("ICU") == 1);
        assert(s.max_weekly_hours == 40);
        assert(s.max_consecutive_days == 5);
        assert(s.min_rest == 12);
        assert(s.prefs.avoid_nights == true);
        assert(s.prefs.preferred_unit.count("ICU") == 1);
        assert(s.prefs.preferred_unit.count("ER") == 1);
        assert(s.availability.size() == 2);
        assert(s.availability[0].can_work == true);

        // shifts
        assert(model.shifts.size() == 2);
        assert(model.shifts[0].id == "shift1");
        assert(model.shifts[1].id == "shift2");

        // rules
        assert(model.rules.max_hours_per_week_default == 40);
        assert(model.rules.hard_constraints.count("coverage") == 1);
        assert(model.rules.soft_constraints.count("fairness") == 1);
    }

    // --- Test 2: unit filter (only ICU) ---
    {
        Filters opt;
        opt.only_shown = "ICU";
        auto model = parse_input_json(json_text, opt);

        assert(model.staff.size() == 1);
        assert(model.shifts.size() == 1);
        assert(model.shifts[0].id == "shift1");
        assert(model.shifts[0].name == "ICU");
    }

    std::cout << "parser_tests: all tests passed.\n";
    return 0;
}
