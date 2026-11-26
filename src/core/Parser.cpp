#include "Parser.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <sstream>

namespace rtsim {

ParseError::ParseError(int l, const std::string& msg)
    : std::runtime_error("line " + std::to_string(l) + ": " + msg), line(l) {}

// Okabe-Ito palette: distinguishable with the common types of color blindness.
static const Color kDefaultPalette[] = {
    {0, 114, 178}, {230, 159, 0}, {0, 158, 115}, {204, 121, 167},
    {86, 180, 233}, {213, 94, 0}, {240, 228, 66}, {0, 0, 0},
};

Tick parseTime(const std::string& text) {
    std::string s = text;
    std::replace(s.begin(), s.end(), ',', '.');
    double value = 0;
    // std::from_chars does not depend on the C locale (unlike scanf/strtod).
    auto res = std::from_chars(s.data(), s.data() + s.size(), value);
    if (res.ec != std::errc() || res.ptr != s.data() + s.size() || !std::isfinite(value))
        throw std::invalid_argument("'" + text + "' is not a number");
    double scaled = value * TICKS_PER_UNIT;
    double rounded = std::round(scaled);
    if (std::fabs(scaled - rounded) > 1e-6)
        throw std::invalid_argument("'" + text + "' is finer than the 0.1 time resolution");
    return static_cast<Tick>(rounded);
}

static int parseColorComponent(const std::string& text, int line) {
    int v = 0;
    auto res = std::from_chars(text.data(), text.data() + text.size(), v);
    if (res.ec != std::errc() || res.ptr != text.data() + text.size() || v < 0 || v > 255)
        throw ParseError(line, "color component '" + text + "' must be an integer 0..255");
    return v;
}

static bool kindFromKeyword(const std::string& w, TaskKind& kind) {
    if (w == "periodic") kind = TaskKind::Periodic;
    else if (w == "simple_sporadic_server" || w == "sporadic_server") kind = TaskKind::SporadicServer;
    else if (w == "deferrable_server") kind = TaskKind::DeferrableServer;
    // "pooling_server" was the spelling used by the original files.
    else if (w == "polling_server" || w == "pooling_server") kind = TaskKind::PollingServer;
    else return false;
    return true;
}

SystemSpec parseSystem(std::istream& in, std::vector<std::string>& warnings) {
    SystemSpec sys;
    std::string raw;
    int lineNo = 0;
    int paletteIndex = 0;

    while (std::getline(in, raw)) {
        ++lineNo;
        if (auto hash = raw.find('#'); hash != std::string::npos) raw.erase(hash);
        std::istringstream ls(raw);
        std::vector<std::string> f;
        for (std::string w; ls >> w;) f.push_back(w);
        if (f.empty()) continue;

        if (f.size() != 3 && f.size() != 6)
            throw ParseError(lineNo, "expected 'type period computing [r g b]', got " +
                                         std::to_string(f.size()) + " fields");

        Tick a = 0, b = 0;
        try {
            a = parseTime(f[1]);
            b = parseTime(f[2]);
        } catch (const std::invalid_argument& e) {
            throw ParseError(lineNo, e.what());
        }

        Color color = kDefaultPalette[paletteIndex++ % std::size(kDefaultPalette)];
        if (f.size() == 6) {
            color.r = static_cast<std::uint8_t>(parseColorComponent(f[3], lineNo));
            color.g = static_cast<std::uint8_t>(parseColorComponent(f[4], lineNo));
            color.b = static_cast<std::uint8_t>(parseColorComponent(f[5], lineNo));
        }

        if (f[0] == "aperiodic") {
            if (sys.tasks.empty() || !isServer(sys.tasks.back().kind))
                throw ParseError(lineNo, "an 'aperiodic' line must come after a server declaration");
            if (a < 0) throw ParseError(lineNo, "release time must be >= 0");
            if (b <= 0) throw ParseError(lineNo, "execution time must be > 0");
            sys.tasks.back().aperiodics.push_back({a, b, color, lineNo});
            continue;
        }

        TaskSpec t;
        if (!kindFromKeyword(f[0], t.kind)) throw ParseError(lineNo, "unknown task type '" + f[0] + "'");
        if (a <= 0) throw ParseError(lineNo, "period must be > 0");
        if (b <= 0) throw ParseError(lineNo, "computing time must be > 0");
        if (b > a) warnings.push_back("line " + std::to_string(lineNo) + ": computing time is larger than the period");
        t.period = a;
        t.exec = b;
        t.color = color;
        t.line = lineNo;
        sys.tasks.push_back(t);
    }

    if (sys.tasks.empty()) throw ParseError(lineNo, "no tasks declared");

    std::stable_sort(sys.tasks.begin(), sys.tasks.end(),
                     [](const TaskSpec& x, const TaskSpec& y) { return x.period < y.period; });
    for (std::size_t i = 0; i < sys.tasks.size(); ++i) {
        TaskSpec& t = sys.tasks[i];
        t.priority = static_cast<int>(i) + 1;
        t.name = std::string(kindPrefix(t.kind)) + std::to_string(t.priority);
        std::stable_sort(t.aperiodics.begin(), t.aperiodics.end(),
                         [](const AperiodicSpec& x, const AperiodicSpec& y) { return x.release < y.release; });
        if (i > 0 && sys.tasks[i - 1].period == t.period)
            warnings.push_back(sys.tasks[i - 1].name + " and " + t.name +
                               " have the same period; the one declared first gets the higher priority");
    }
    return sys;
}

}  // namespace rtsim
