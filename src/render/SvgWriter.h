#pragma once

#include <ostream>

#include "Layout.h"

namespace rtsim {

// Vector output: opens in any browser, Inkscape or LaTeX (via svg/inkscape).
void writeSvg(std::ostream& out, const Drawing& d);

}  // namespace rtsim
