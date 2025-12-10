#include "model.hpp"
#include "input_parser.hpp"
#include "engine.hpp"

#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>

// Helper Functions

// Usage to help run program
static void print_usage() {
    std::cout << "Usage:\n"
              << "  scheduler <input.json> [--unit UNIT_NAME] [--csv OUTPUT.csv]\n"
              << "\nIf --csv is not provided, the program automatically creates:\n"
              << "  schedule.csv\n"
              << "or, if --unit is given:\n"
              << "  schedule_<unit>.csv\n\n";
}

// Format time into string for CSV
static std::string format_time(const SysTime& t) {
    std::time_t tt = Clock::to_time_t(t);
    std::tm* tm = std::localtime(&tt);
    char buf[32];
    if (std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", tm)) {
        return std::string(buf);
    }
    return "";
}

// Join staff 
static std::string join_staff_ids(const std::vector<std::string>& ids) {
    std::string out;
    for (size_t i = 0; i < ids.size(); ++i) {
        out += ids[i];
        if (i + 1 < ids.size()) out += ";";
    }
    return out;
}

// Compute total staff hours
static std::unordered_map<std::string, Hours> compute_staff_hours(const InputModel& model, const ScheduleResult& result) {
    std::unordered_map<std::string, Hours> totals;

    // Map shift_id -> Shifts*
    std::unordered_map<std::string, const Shifts*> shift_by_id;
    for (const auto& sh : model.shifts) {
        shift_by_id[sh.id] = &sh;
    }

    // Sum the hours
    for (const auto& asg : result.assignments) {
        auto iter = shift_by_id.find(asg.shift_id);
        if (iter == shift_by_id.end()) continue;
        const Shifts* sh = iter->second;
        Hours dur = sh->duration();
        for (const auto& sid : asg.staff_ids) {
            totals[sid] += dur;
        }
    }

    return totals;
}

// Rename flag
static std::string base_name_from_csv(const std::string& csv_path) {
    // Strip everything after last '.'
    auto pos = csv_path.find_last_of('.');
    if (pos == std::string::npos) return csv_path;
    return csv_path.substr(0, pos);
}

// Schedule CSV Writer
static bool write_schedule_csv(const InputModel& model, const ScheduleResult& result, const std::string& csv_path) {
    // Output file verification
    std::ofstream out(csv_path);
    if (!out) {
        std::cerr << "Error: Cannot open CSV file for writing: " << csv_path << "\n";
        return false;
    }

    // Map shift_id -> assignment pointer
    std::unordered_map<std::string, const Assignment*> asg_by_shift;
    for (const auto& a : result.assignments) {
        asg_by_shift[a.shift_id] = &a;
    }

    // Build a list of shift indices and sort by start time
    std::vector<const Shifts*> ordered_shifts;
    ordered_shifts.reserve(model.shifts.size());
    for (const auto& sh : model.shifts) ordered_shifts.push_back(&sh);
    std::sort(ordered_shifts.begin(), ordered_shifts.end(),
              [](const Shifts* a, const Shifts* b) { return a->start < b->start; });

    // CSV Headers
    out << "shift_id,unit,start,end,required_role,required_count,"
           "assigned_count,assigned_staff_ids,coverage_ok,missing_count\n";

    // Extract data from assignments
    for (const auto* sh : ordered_shifts) {
        const Assignment* asg = nullptr;
        auto it = asg_by_shift.find(sh->id);
        if (it != asg_by_shift.end()) asg = it->second;

        int assigned_count = asg ? static_cast<int>(asg->staff_ids.size()) : 0;
        int required = sh->required_count;
        bool coverage_ok = (assigned_count >= required);
        int missing = (required > assigned_count) ? (required - assigned_count) : 0;

        std::string staff_joined = asg ? join_staff_ids(asg->staff_ids) : "";

        // Format output to cells
        out << sh->id << ","
            << sh->name << ","
            << format_time(sh->start) << ","
            << format_time(sh->end) << ","
            << sh->req_role << ","
            << required << ","
            << assigned_count << ","
            << staff_joined << ","
            << (coverage_ok ? "Yes" : "No") << ","
            << missing << "\n";
    }

    return true;
}

// Staff summary CSV (hours per staff) writer
static bool write_staff_summary_csv(const InputModel& model, const std::unordered_map<std::string, Hours>& totals, const std::string& csv_path) {
    std::ofstream out(csv_path);
    if (!out) {
        std::cerr << "Error: Cannot open CSV file for writing: " << csv_path << "\n";
        return false;
    }

    // Headers
    out << "staff_id,name,role,total_hours\n";

    // Loop through staff to get data
    for (const auto& s : model.staff) {
        auto it = totals.find(s.id);
        Hours h = Hours{0};
        if (it != totals.end()) h = it->second;
        auto h_int = std::chrono::duration_cast<Hours>(h).count();

        // Format data
        out << s.id << ","
            << s.name << ","
            << s.role << ","
            << h_int << "\n";
    }

    return true;
}

// Warnings CSV writer
static bool write_warnings_csv(const ScheduleResult& result, const std::string& csv_path)
{
    std::ofstream out(csv_path);
    if (!out) {
        std::cerr << "Error: Cannot open CSV file for writing: " << csv_path << "\n";
        return false;
    }

    // Adds warnings to CSV
    out << "index,warning\n";
    for (size_t i = 0; i < result.warnings.size(); ++i) {
        out << i << "," << result.warnings[i] << "\n";
    }

    return true;
}

// Main function

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    std::string input_path = argv[1];
    std::string unit_filter;
    std::string csv_output_path;  // optional: rename file

    // Parse flags
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        // Filter flag
        if (arg == "--unit" && i + 1 < argc) {
            unit_filter = argv[++i];
        }
        // Rename flag
        else if (arg == "--csv" && i + 1 < argc) {
            csv_output_path = argv[++i];
        }
        else {
            std::cerr << "Unknown or incomplete option: " << arg << "\n";
            print_usage();
            return 1;
        }
    }

    // Open JSON file
    std::ifstream in(input_path);
    if (!in) {
        std::cerr << "Error: Cannot open input file: " << input_path << "\n";
        return 2;
    }

    std::ostringstream buffer;
    buffer << in.rdbuf();
    std::string json_text = buffer.str();

    // Parse JSON into InputModel
    Filters f;
    f.only_shown = unit_filter;

    InputModel model;

    try {
        model = parse_input_json(json_text, f);
    } catch (const std::exception& e) {
        std::cerr << "Error: Failed to parse JSON: " << e.what() << "\n";
        return 3;
    }

    //  Build schedule
    EngineOptions opts;
    auto result = build_schedule(model, opts);

    // Print CLI output
    std::cout << "=== Hospital Scheduler ===\n"
              << "Input: " << input_path << "\n";

    if (!unit_filter.empty()) {
        std::cout << "Unit filter: " << unit_filter << "\n";
    }

    std::cout << "--------------------------\n\n";

    for (const auto& a : result.assignments) {
        std::cout << "Shift: " << a.shift_id << "\n";
        if (a.staff_ids.empty()) {
            std::cout << "  Assigned: (none)\n";
        } else {
            std::cout << "  Assigned: ";
            for (size_t i = 0; i < a.staff_ids.size(); ++i) {
                std::cout << a.staff_ids[i];
                if (i + 1 < a.staff_ids.size()) std::cout << ", ";
            }
            std::cout << "\n";
        }
        std::cout << "\n";
    }

    if (!result.warnings.empty()) {
        std::cout << "=== WARNINGS ===\n";
        for (const auto& w : result.warnings) {
            std::cout << " - " << w << "\n";
        }
        std::cout << "\n";
    }

    // CSV name
    if (csv_output_path.empty()) {
        if (!unit_filter.empty()) {
            csv_output_path = "schedule_" + unit_filter + ".csv";
        } else {
            csv_output_path = "schedule.csv";
        }
    }

    std::string base = base_name_from_csv(csv_output_path);
    std::string staff_csv    = base + "_staff.csv";
    std::string warnings_csv = base + "_warnings.csv";

    // Write main schedule CSV
    if (!write_schedule_csv(model, result, csv_output_path)) {
        std::cerr << "Failed to write schedule CSV.\n";
        return 4;
    }
    std::cout << "Schedule CSV written to: " << csv_output_path << "\n";

    // Write staff summary CSV
    auto staff_hours = compute_staff_hours(model, result);
    if (!write_staff_summary_csv(model, staff_hours, staff_csv)) {
        std::cerr << "Failed to write staff summary CSV.\n";
        return 5;
    }
    std::cout << "Staff summary CSV written to: " << staff_csv << "\n";

    // Write warnings CSV
    if (!write_warnings_csv(result, warnings_csv)) {
        std::cerr << "Failed to write warnings CSV.\n";
        return 6;
    }
    std::cout << "Warnings CSV written to: " << warnings_csv << "\n";

    std::cout << "Done.\n";
    return 0;
}
