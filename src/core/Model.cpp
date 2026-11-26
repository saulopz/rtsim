#include "Model.h"

namespace rtsim {

bool isServer(TaskKind k) { return k != TaskKind::Periodic; }

const char* kindPrefix(TaskKind k) {
    switch (k) {
        case TaskKind::Periodic: return "T";
        case TaskKind::PollingServer: return "PS";
        case TaskKind::DeferrableServer: return "DS";
        case TaskKind::SporadicServer: return "SS";
    }
    return "?";
}

const char* kindName(TaskKind k) {
    switch (k) {
        case TaskKind::Periodic: return "periodic task";
        case TaskKind::PollingServer: return "polling server";
        case TaskKind::DeferrableServer: return "deferrable server";
        case TaskKind::SporadicServer: return "simple sporadic server";
    }
    return "?";
}

std::string formatTicks(Tick t) {
    std::string sign = t < 0 ? "-" : "";
    if (t < 0) t = -t;
    return sign + std::to_string(t / TICKS_PER_UNIT) + "." + std::to_string(t % TICKS_PER_UNIT);
}

}  // namespace rtsim
