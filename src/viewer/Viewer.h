#pragma once

#include <string>

#include "../render/Layout.h"

namespace rtsim {

// Opens the interactive timeline viewer (raylib). If screenshotPath is not
// empty, renders a single frame into that PNG and returns without showing
// the window. Returns a process exit code.
int runViewer(const Drawing& d, const std::string& title, const std::string& screenshotPath);

}  // namespace rtsim
