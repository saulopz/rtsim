#include "Report.h"

#include <algorithm>
#include <cstdio>

namespace rtsim {

static std::string fmt(double v) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.3f", v);
    return buf;
}

void writeReport(std::ostream& out, const SystemSpec& sys, const Analysis& an, const SimResult& res) {
    out << "RTSim - Rate Monotonic, preemptive, time resolution 0.1\n\n";

    out << "Tasks (priority order)\n";
    for (std::size_t i = 0; i < sys.tasks.size(); ++i) {
        const TaskSpec& t = sys.tasks[i];
        out << "  " << t.name << " (" << formatTicks(t.period) << ", " << formatTicks(t.exec) << ")  "
            << kindName(t.kind) << "\n";
    }

    out << "\nSchedulability analysis\n";
    out << "  Utilization U = " << fmt(an.utilization) << "\n";
    out << "  Liu & Layland bound n(2^(1/n)-1) = " << fmt(an.llBound) << "  ->  "
        << (an.llPasses ? "U <= bound: schedulable" : "U > bound: inconclusive (the test is only sufficient)")
        << "\n";
    out << "  Response Time Analysis (exact for this model):\n";
    for (std::size_t i = 0; i < sys.tasks.size(); ++i) {
        const TaskSpec& t = sys.tasks[i];
        out << "    " << t.name << ": ";
        if (an.tasks[i].schedulable)
            out << "R = " << formatTicks(an.tasks[i].response) << " <= D = " << formatTicks(t.period) << "\n";
        else
            out << "R > D = " << formatTicks(t.period) << "  NOT schedulable\n";
    }
    out << "  Verdict: " << (an.rtaPasses ? "schedulable" : "NOT schedulable") << "\n";

    out << "\nSimulation over [0, " << formatTicks(res.length) << ")";
    if (res.hyperperiod > 0) out << ", hyperperiod " << formatTicks(res.hyperperiod);
    out << "\n";
    for (const auto& tr : res.tasks) {
        out << "  " << tr.spec.name << " (" << formatTicks(tr.spec.period) << ", " << formatTicks(tr.spec.exec)
            << ")\n";
        if (!isServer(tr.spec.kind)) {
            Tick worst = -1;
            int misses = 0;
            for (const auto& j : tr.jobs) {
                worst = std::max(worst, j.response());
                misses += j.missed ? 1 : 0;
            }
            out << "    jobs released: " << tr.jobs.size() << ", deadline misses: " << misses
                << ", worst observed response: " << (worst < 0 ? std::string("-") : formatTicks(worst)) << "\n";
        } else {
            for (std::size_t k = 0; k < tr.aperiodics.size(); ++k) {
                const auto& a = tr.aperiodics[k];
                out << "    A" << (k + 1) << " (r=" << formatTicks(a.spec.release) << ", e=" << formatTicks(a.spec.exec)
                    << ")";
                if (a.done())
                    out << "  start " << formatTicks(a.start) << ", finish " << formatTicks(a.finish)
                        << ", response time " << formatTicks(a.response()) << "\n";
                else
                    out << "  did not finish\n";
            }
            if (tr.spec.kind == TaskKind::DeferrableServer) {
                out << "    budget replenished every " << formatTicks(tr.spec.period) << "\n";
            } else {
                // List the replenishments up to the last aperiodic completion
                // (after that the server is just idle).
                Tick lastActivity = 0;
                for (const auto& a : tr.aperiodics) lastActivity = std::max(lastActivity, a.done() ? a.finish : res.length);
                out << "    budget replenished at:";
                int shown = 0;
                for (Tick t : tr.replenishments) {
                    if (t > lastActivity + tr.spec.period || shown == 40) {
                        out << " ...";
                        break;
                    }
                    out << " " << formatTicks(t);
                    ++shown;
                }
                out << "\n";
            }
        }
    }
    if (!res.warnings.empty()) {
        out << "\nWarnings\n";
        for (const auto& w : res.warnings) out << "  " << w << "\n";
    }
}

}  // namespace rtsim
