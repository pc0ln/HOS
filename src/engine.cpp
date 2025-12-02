#include "engine.hpp"
#include <algorithm>
#include <optional>
#include <sstream>

// Helper: check availability by date
static bool available_on(const Staff& s, const DateStamp& day) {
    if (s.availability.empty()) return true;
    for (const auto& a : s.availability) {
        if (a.date.y == day.y && a.date.m == day.m && a.date.d == day.d) {
            return a.can_work;
        }
    }
    return true; // if no explicit entry, assume can work
}

struct WorkerState {
    const Staff* staff;
    Hours assigned_hours{0};
    std::optional<SysTime> last_end{};
};

// Very naive rest check: require at least min_rest_hours between end and next start
static bool has_rest(const WorkerState& ws, const Staff& s, const Shifts& sh) {
    if (!ws.last_end) return true;
    auto rest = std::chrono::duration_cast<Hours>(sh.start - *ws.last_end);
    return rest.count() >= s.min_rest;
}

// Role must match exactly
static bool role_ok(const Staff& s, const Shifts& sh) {
    return s.role == sh.req_role;
}

// Hours must not exceed max_hours_per_week
static bool legal_hours_ok(const WorkerState& ws, const Staff& s, const Shifts& sh) {
    auto cap = Hours{s.max_weekly_hours};
    return (ws.assigned_hours + sh.duration()) <= cap;
}

// Preference penalty: lower is better
static int preference_penalty(const Staff& s, const Shifts& sh) {
    int p = 0;
    if (s.prefs.avoid_nights && sh.is_night()) p += 5;
    if (!s.prefs.preferred_unit.empty() &&
        s.prefs.preferred_unit.count(sh.name) == 0) {
        p += 1;
    }
    return p;
}

ScheduleResult build_schedule(const InputModel& input, const EngineOptions& opt) {
    ScheduleResult result;

    // Copy shifts and sort by start time (earlier first)
    std::vector<Shifts> shifts = input.shifts;
    std::sort(shifts.begin(), shifts.end(),
              [](const Shifts& a, const Shifts& b) { return a.start < b.start; });

    // Create worker state (tracking hours + last shift end)
    std::vector<WorkerState> workers;
    workers.reserve(input.staff.size());
    for (const auto& s : input.staff) {
        WorkerState ws;
        ws.staff = &s;
        workers.push_back(ws);
    }

    for (const auto& sh : shifts) {
        Assignment asg;
        asg.shift_id = sh.id;

        // Build candidate list
        std::vector<WorkerState*> candidates;
        candidates.reserve(workers.size());

        for (auto& ws : workers) {
            const Staff* s = ws.staff;
            if (!role_ok(*s, sh)) continue;
            if (!available_on(*s, sh.day())) continue;
            if (!legal_hours_ok(ws, *s, sh)) continue;
            if (!has_rest(ws, *s, sh)) continue;
            candidates.push_back(&ws);
        }

        if (candidates.empty()) {
            std::ostringstream oss;
            oss << "No eligible staff for shift " << sh.id << " (" << sh.name << ")";
            result.warnings.push_back(oss.str());
            result.assignments.push_back(std::move(asg));
            continue;
        }

        // Sort candidates: fairness (fewest hours) then preferences
        std::sort(candidates.begin(), candidates.end(),
            [&](const WorkerState* a, const WorkerState* b) {
                if (opt.fairness_on && a->assigned_hours != b->assigned_hours) {
                    return a->assigned_hours < b->assigned_hours;
                }
                if (opt.respect_preferences) {
                    int pa = preference_penalty(*a->staff, sh);
                    int pb = preference_penalty(*b->staff, sh);
                    if (pa != pb) return pa < pb;
                }
                return a->staff->id < b->staff->id; // tie-breaker
            });

        int need = sh.required_count;
        for (auto* ws : candidates) {
            if (need == 0) break;
            asg.staff_ids.push_back(ws->staff->id);
            ws->assigned_hours += sh.duration();
            ws->last_end = sh.end;
            --need;
        }

        if (need > 0) {
            std::ostringstream oss;
            oss << "Coverage short by " << need << " for shift " << sh.id;
            result.warnings.push_back(oss.str());
        }

        result.assignments.push_back(std::move(asg));
    }

    return result;
}
