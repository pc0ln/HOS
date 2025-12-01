#pragma once
#include <string>

struct Filters // Optional to add filter, selected is only_shown and none is all
{
    std::string only_shown;
};

// Need to make Model.
InputModel parse_input_json(const std::string& json_text, const Filters& opt = {});