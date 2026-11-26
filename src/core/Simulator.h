// Discrete-time simulation of a fixed-priority (Rate Monotonic), preemptive
// uniprocessor with periodic tasks and aperiodic servers.
//
// Server rules follow Jane W. S. Liu, "Real-Time Systems" (2000), ch. 7:
//  - Polling server: budget refilled each period; lost as soon as there is
//    no aperiodic job waiting (also at the start of the period).
//  - Deferrable server: budget refilled each period and kept while idle.
//  - Simple sporadic server: consumption rules C1, C2 and replenishment
//    rules R1, R2, R3 (fixed-priority version, section 7.3.1).
#pragma once

#include <string>
#include <vector>

#include "Model.h"

namespace rtsim {

// A contiguous interval during which one job used the processor.
struct Segment {
    Tick start = 0, end = 0;
    Color color;
    int job = -1;        // index in TaskTrace::jobs (periodic tasks)
    int aperiodic = -1;  // index in TaskTrace::aperiodics (servers)
};

struct JobRecord {
    Tick release = 0, deadline = 0;
    Tick start = -1, finish = -1;  // finish = end of the last tick executed
    bool missed = false;
    Tick response() const { return finish < 0 ? -1 : finish - release; }
};

struct AperiodicResult {
    AperiodicSpec spec;
    Tick start = -1, finish = -1;
    // Response time = completion - release (Liu, Buttazzo).
    Tick response() const { return finish < 0 ? -1 : finish - spec.release; }
    bool done() const { return finish >= 0; }
};

struct BudgetPoint {
    Tick t = 0;
    Tick value = 0;
};

struct TaskTrace {
    TaskSpec spec;
    std::vector<Segment> segments;
    std::vector<JobRecord> jobs;              // periodic tasks
    std::vector<AperiodicResult> aperiodics;  // servers
    std::vector<BudgetPoint> budget;          // servers, polyline
    std::vector<Tick> replenishments;         // servers
};

struct SimOptions {
    Tick length = 0;            // 0 = automatic (hyperperiod, extended for aperiodics)
    Tick maxLength = 50'000;    // limit for the automatic length (5000 time units)
};

struct SimResult {
    std::vector<TaskTrace> tasks;  // same order as SystemSpec::tasks (priority)
    Tick hyperperiod = 0;          // 0 if larger than the limit
    Tick length = 0;               // simulated interval [0, length)
    std::vector<std::string> warnings;
};

Tick hyperperiod(const SystemSpec& sys, Tick limit);  // 0 if it exceeds limit
SimResult simulate(const SystemSpec& sys, const SimOptions& opt = {});

}  // namespace rtsim
