#pragma once

#include <ostream>

#include "Analysis.h"
#include "Simulator.h"

namespace rtsim {

void writeReport(std::ostream& out, const SystemSpec& sys, const Analysis& an, const SimResult& res);

}  // namespace rtsim
