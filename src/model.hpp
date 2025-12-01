#pragma once
#include <vector>

struct InputModel {
    std::vector<Staff> staff; // List of staff for schedules
    std::vector<Shifts> shifts; // List of shifts needed/available
    Rules rules; // Rules for the engine (Like max hours, preference, coverage)
};
