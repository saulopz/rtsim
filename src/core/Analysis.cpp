#include "Analysis.h"

#include <cmath>

namespace rtsim {

static Tick ceilDiv(Tick a, Tick b) { return (a + b - 1) / b; }

Analysis analyze(const SystemSpec& sys) {
    Analysis an;
    const std::size_t n = sys.tasks.size();
    for (const auto& t : sys.tasks) an.utilization += static_cast<double>(t.exec) / static_cast<double>(t.period);
    an.llBound = n * (std::pow(2.0, 1.0 / static_cast<double>(n)) - 1.0);
    an.llPasses = an.utilization <= an.llBound + 1e-12;

    an.rtaPasses = true;
    for (std::size_t i = 0; i < n; ++i) {
        const TaskSpec& ti = sys.tasks[i];
        Tick r = ti.exec;
        TaskAnalysis ta;
        for (;;) {
            Tick next = ti.exec;
            for (std::size_t j = 0; j < i; ++j) {
                const TaskSpec& tj = sys.tasks[j];
                Tick jitter = (tj.kind == TaskKind::DeferrableServer) ? tj.period - tj.exec : 0;
                next += ceilDiv(r + jitter, tj.period) * tj.exec;
            }
            if (next > ti.period) break;  // deadline (= period) exceeded
            if (next == r) {
                ta.response = r;
                ta.schedulable = true;
                break;
            }
            r = next;
        }
        an.rtaPasses = an.rtaPasses && ta.schedulable;
        an.tasks.push_back(ta);
    }
    return an;
}

}  // namespace rtsim
