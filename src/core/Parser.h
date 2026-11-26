// Reads the .in task description format:
//
//   type period computing [red green blue]
//
// Numbers accept both '.' and ',' as decimal separator, independent of the
// system locale. Lines starting with '#' are comments. "aperiodic" lines
// belong to the server declared right before them.
#pragma once

#include <istream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Model.h"

namespace rtsim {

struct ParseError : std::runtime_error {
    int line;
    ParseError(int line, const std::string& msg);
};

// Parses a time value ("15,5", "15.5", "3") into ticks. Throws
// std::invalid_argument if it is not a number or is finer than one tick.
Tick parseTime(const std::string& text);

// Parses and validates the system, then assigns Rate Monotonic priorities
// (shorter period = higher priority; ties keep file order).
SystemSpec parseSystem(std::istream& in, std::vector<std::string>& warnings);

}  // namespace rtsim
