#pragma once

#include "model.hpp"
#include <string>
#include <vector>

struct EngineOptions {
    bool fairness_on        = true;  // prefer staff with fewer hours
    bool respect_preferences = true; // avoid nights / non-preferred units when possible
};

struct Assignment {
    std::string shift_id;
    std::vector<std::string> staff_ids;
};

struct ScheduleResult {
    std::vector<Assignment> assignments;
    std::vector<std::string> warnings;
};

// Build a schedule from parsed input model
ScheduleResult build_schedule(const InputModel& model, const EngineOptions& opt = EngineOptions{});
