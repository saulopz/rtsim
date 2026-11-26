// RTSim - real-time scheduling simulator
// Data model shared by the parser, the simulator and the renderers.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace rtsim {

// Time is discrete. One tick = 0.1 time units, as in the original RTSim,
// but stored as an integer so that no floating point drift can happen.
using Tick = std::int64_t;
constexpr Tick TICKS_PER_UNIT = 10;

struct Color {
    std::uint8_t r = 0, g = 0, b = 0;
};

enum class TaskKind { Periodic, PollingServer, DeferrableServer, SporadicServer };

struct AperiodicSpec {
    Tick release = 0;
    Tick exec = 0;
    Color color;
    int line = 0;  // line in the input file (for messages)
};

struct TaskSpec {
    TaskKind kind = TaskKind::Periodic;
    Tick period = 0;  // T (or ps for servers)
    Tick exec = 0;    // C (or es, the server budget)
    Color color;
    std::vector<AperiodicSpec> aperiodics;  // only for servers
    int line = 0;
    int priority = 0;  // 1 = highest (Rate Monotonic order)
    std::string name;  // e.g. "T1", "SS4"
};

struct SystemSpec {
    std::vector<TaskSpec> tasks;  // sorted by priority after parsing
};

bool isServer(TaskKind k);
const char* kindPrefix(TaskKind k);  // "T", "PS", "DS", "SS"
const char* kindName(TaskKind k);    // "periodic", "polling server", ...

// 155 -> "15.5"
std::string formatTicks(Tick t);

}  // namespace rtsim
