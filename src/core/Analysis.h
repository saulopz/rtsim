// Offline schedulability analysis for Rate Monotonic with implicit deadlines.
#pragma once

#include <string>
#include <vector>

#include "Model.h"

namespace rtsim {

struct TaskAnalysis {
    Tick response = -1;  // worst-case response time from RTA (-1 = exceeds the deadline)
    bool schedulable = false;
};

struct Analysis {
    double utilization = 0;
    double llBound = 0;      // Liu & Layland: n (2^(1/n) - 1)
    bool llPasses = false;   // U <= bound -> schedulable (sufficient test only)
    std::vector<TaskAnalysis> tasks;
    bool rtaPasses = false;  // exact for this model
};

// Response Time Analysis (Joseph & Pandya / Audsley et al.):
//   R = C_i + sum_{j in hp(i)} ceil((R + J_j) / T_j) C_j
// Polling and sporadic servers interfere like a periodic task (ps, es).
// A deferrable server may run back-to-back, which is modeled as release
// jitter J = ps - es (Strosnider, Lehoczky & Sha, 1995).
Analysis analyze(const SystemSpec& sys);

}  // namespace rtsim
