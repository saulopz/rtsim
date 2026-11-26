#include "Simulator.h"

#include <algorithm>
#include <deque>
#include <numeric>

namespace rtsim {

Tick hyperperiod(const SystemSpec& sys, Tick limit) {
    Tick h = 1;
    for (const auto& t : sys.tasks) {
        Tick g = std::gcd(h, t.period);
        Tick step = t.period / g;
        if (h > limit / step) return 0;  // would exceed the limit (or overflow)
        h *= step;
    }
    return h;
}

namespace {

// State of the simple sporadic server (Liu's notation).
struct SporadicState {
    bool thBusyPrev = false;  // was T_H (higher-priority subsystem) busy in the previous tick?
    bool thBusy = false;      // is T_H busy in the current tick?
    Tick begin = -1;          // BEGIN: start of the latest busy interval of T_H (-1 = never)
    Tick end = -1;            // END: end of the latest busy interval of T_H (-1 = never ended)
    Tick tr = 0;              // last replenishment time
    Tick tc = -1;             // next scheduled replenishment (-1 = none scheduled)
    bool executedSinceTr = false;
    bool replenishWhenExhausted = false;  // rule R3(a)
    bool sawIdle = false;                 // rule R3(b): system was idle after tf
};

struct Runtime {
    std::deque<int> pendingJobs;  // periodic: indexes of unfinished jobs (FIFO)
    std::vector<Tick> jobRemaining;
    std::deque<int> queue;  // server: indexes of waiting aperiodic jobs (FIFO)
    std::vector<Tick> apRemaining;
    std::size_t nextArrival = 0;
    Tick budget = 0;
    SporadicState ss;
};

void pushBudgetPoint(std::vector<BudgetPoint>& v, Tick t, Tick value) {
    if (!v.empty() && v.back().t == t) {
        if (v.back().value != value) v.push_back({t, value});  // vertical jump
        return;
    }
    if (v.size() >= 2) {
        BudgetPoint& a = v[v.size() - 2];
        BudgetPoint& b = v.back();
        // Merge collinear points to keep the polyline small.
        if (a.t != b.t && (b.value - a.value) * (t - b.t) == (value - b.value) * (b.t - a.t)) {
            b = {t, value};
            return;
        }
    }
    v.push_back({t, value});
}

void appendSegment(TaskTrace& tr, Tick t, Color c, int job, int ap) {
    if (!tr.segments.empty()) {
        Segment& s = tr.segments.back();
        if (s.end == t && s.job == job && s.aperiodic == ap) {
            s.end = t + 1;
            return;
        }
    }
    tr.segments.push_back({t, t + 1, c, job, ap});
}

}  // namespace

SimResult simulate(const SystemSpec& sys, const SimOptions& opt) {
    SimResult res;
    const std::size_t n = sys.tasks.size();
    std::vector<Runtime> rt(n);

    for (const auto& spec : sys.tasks) {
        TaskTrace tr;
        tr.spec = spec;
        for (const auto& a : spec.aperiodics) tr.aperiodics.push_back({a});
        res.tasks.push_back(tr);
    }
    for (std::size_t i = 0; i < n; ++i)
        for (const auto& a : sys.tasks[i].aperiodics) rt[i].apRemaining.push_back(a.exec);

    res.hyperperiod = hyperperiod(sys, opt.maxLength);
    Tick end = opt.length;
    const bool autoLength = (end <= 0);
    if (autoLength) {
        if (res.hyperperiod == 0) {
            end = opt.maxLength;
            res.warnings.push_back("hyperperiod is larger than " + formatTicks(opt.maxLength) +
                                   " time units; simulating only up to that (use --length to choose)");
        } else {
            end = res.hyperperiod;
            Tick lastRelease = 0;
            for (const auto& t : sys.tasks)
                for (const auto& a : t.aperiodics) lastRelease = std::max(lastRelease, a.release);
            while (end <= lastRelease && end + res.hyperperiod <= opt.maxLength) end += res.hyperperiod;
        }
    }
    const Tick extendStep = res.hyperperiod > 0 ? res.hyperperiod : end;
    int extensions = 0;

    auto ready = [&](std::size_t i) {
        if (!isServer(sys.tasks[i].kind)) return !rt[i].pendingJobs.empty();
        return !rt[i].queue.empty() && rt[i].budget > 0;
    };

    auto replenish = [&](std::size_t i, Tick t) {
        Runtime& r = rt[i];
        r.budget = sys.tasks[i].exec;
        res.tasks[i].replenishments.push_back(t);
        if (sys.tasks[i].kind == TaskKind::SporadicServer) {
            SporadicState& s = r.ss;
            s.tr = t;  // R1: tr = current time whenever the budget is replenished
            s.tc = -1;
            s.executedSinceTr = false;
            s.replenishWhenExhausted = false;
            s.sawIdle = false;
        }
    };

    for (Tick t = 0; t < end; ++t) {
        // 1. Releases and arrivals.
        for (std::size_t i = 0; i < n; ++i) {
            const TaskSpec& spec = sys.tasks[i];
            Runtime& r = rt[i];
            TaskTrace& tr = res.tasks[i];
            if (!isServer(spec.kind)) {
                if (t % spec.period == 0) {
                    for (int j : r.pendingJobs)
                        if (tr.jobs[j].deadline <= t) tr.jobs[j].missed = true;
                    tr.jobs.push_back({t, t + spec.period});
                    r.jobRemaining.push_back(spec.exec);
                    r.pendingJobs.push_back(static_cast<int>(tr.jobs.size()) - 1);
                }
            } else {
                while (r.nextArrival < tr.aperiodics.size() &&
                       tr.aperiodics[r.nextArrival].spec.release <= t) {
                    r.queue.push_back(static_cast<int>(r.nextArrival));
                    ++r.nextArrival;
                }
            }
        }

        // 2. Budget rules, in priority order (so T_H of each server is already up to date).
        for (std::size_t i = 0; i < n; ++i) {
            const TaskSpec& spec = sys.tasks[i];
            Runtime& r = rt[i];
            switch (spec.kind) {
                case TaskKind::Periodic:
                    break;
                case TaskKind::PollingServer:
                    if (t % spec.period == 0) {
                        if (r.queue.empty()) r.budget = 0;  // nothing to poll: budget is lost
                        else replenish(i, t);
                    }
                    break;
                case TaskKind::DeferrableServer:
                    if (t % spec.period == 0) replenish(i, t);
                    break;
                case TaskKind::SporadicServer: {
                    SporadicState& s = r.ss;
                    bool thBusy = false;
                    for (std::size_t j = 0; j < i; ++j) thBusy = thBusy || ready(j);
                    if (thBusy && !s.thBusyPrev) s.begin = t;
                    if (!thBusy && s.thBusyPrev) s.end = t;
                    s.thBusyPrev = thBusy;
                    s.thBusy = thBusy;

                    if (t == 0) {
                        replenish(i, t);  // R1
                    } else if (s.tc >= 0 && t >= s.tc) {
                        replenish(i, t);  // R3: scheduled replenishment (or R3(a))
                    } else if (s.tc >= 0 && s.sawIdle) {
                        // R3(b): the system was idle before tc and becomes busy now.
                        // A waiting aperiodic job with no budget does not count as
                        // "busy", since it cannot use the processor.
                        bool systemBusy = false;
                        for (std::size_t j = 0; j < n; ++j) systemBusy = systemBusy || ready(j);
                        if (systemBusy) replenish(i, t);  // tb = t < tc, so min(tc, tb) = t
                    }
                    break;
                }
            }
        }

        // Budget at the start of the tick (after replenishments).
        std::vector<Tick> budgetStart(n);
        for (std::size_t i = 0; i < n; ++i) budgetStart[i] = rt[i].budget;

        // 3. Fixed-priority dispatch: the highest-priority ready task runs.
        int run = -1;
        for (std::size_t i = 0; i < n; ++i)
            if (ready(i)) {
                run = static_cast<int>(i);
                break;
            }

        // 4. Execute one tick.
        if (run >= 0) {
            const TaskSpec& spec = sys.tasks[run];
            Runtime& r = rt[run];
            TaskTrace& tr = res.tasks[run];
            if (!isServer(spec.kind)) {
                int j = r.pendingJobs.front();
                JobRecord& job = tr.jobs[j];
                if (job.start < 0) job.start = t;
                appendSegment(tr, t, spec.color, j, -1);
                if (--r.jobRemaining[j] == 0) {
                    job.finish = t + 1;
                    r.pendingJobs.pop_front();
                }
            } else {
                int a = r.queue.front();
                AperiodicResult& ap = tr.aperiodics[a];
                if (ap.start < 0) ap.start = t;
                appendSegment(tr, t, ap.spec.color, -1, a);
                if (--r.apRemaining[a] == 0) {
                    ap.finish = t + 1;
                    r.queue.pop_front();
                }
                --r.budget;  // C1 (and the only consumption rule of PS and DS)

                if (spec.kind == TaskKind::SporadicServer && !r.ss.executedSinceTr) {
                    // R2: t is tf, the first instant after tr at which the server executes.
                    SporadicState& s = r.ss;
                    Tick te = (s.end == t) ? std::max(s.tr, s.begin) : t;
                    s.tc = te + spec.period;
                    s.replenishWhenExhausted = (s.tc < t);  // R3(a)
                    s.executedSinceTr = true;
                    s.sawIdle = false;
                }
                if (spec.kind == TaskKind::PollingServer && r.queue.empty())
                    r.budget = 0;  // polling server suspends and loses what is left
            }
        }

        // 5. Sporadic server: rule C2 and bookkeeping for R3.
        for (std::size_t i = 0; i < n; ++i) {
            if (sys.tasks[i].kind != TaskKind::SporadicServer) continue;
            Runtime& r = rt[i];
            SporadicState& s = r.ss;
            // C2: the server executed since tr and T_H is idle -> budget is consumed
            // even if the server itself is not executing.
            if (static_cast<int>(i) != run && s.executedSinceTr && !s.thBusy && r.budget > 0) --r.budget;
            if (s.replenishWhenExhausted && r.budget == 0) s.tc = t + 1;
            if (run < 0 && s.tc >= 0) s.sawIdle = true;
        }

        // 6. Budget plot.
        for (std::size_t i = 0; i < n; ++i) {
            if (!isServer(sys.tasks[i].kind)) continue;
            pushBudgetPoint(res.tasks[i].budget, t, budgetStart[i]);
            pushBudgetPoint(res.tasks[i].budget, t + 1, rt[i].budget);
        }

        // Keep going while aperiodic jobs are still waiting (automatic length only).
        if (t + 1 == end && autoLength && extensions < 8 && end + extendStep <= opt.maxLength) {
            bool pending = false;
            for (std::size_t i = 0; i < n; ++i)
                for (const auto& a : res.tasks[i].aperiodics) pending = pending || !a.done();
            if (pending) {
                end += extendStep;
                ++extensions;
            }
        }
    }

    res.length = end;
    for (auto& tr : res.tasks) {
        for (auto& j : tr.jobs)
            if (j.finish < 0 && j.deadline <= end) j.missed = true;
            else if (j.finish > j.deadline) j.missed = true;
        for (const auto& a : tr.aperiodics)
            if (!a.done())
                res.warnings.push_back(tr.spec.name + ": aperiodic job released at " +
                                       formatTicks(a.spec.release) + " did not finish by " +
                                       formatTicks(end));
    }
    return res;
}

}  // namespace rtsim
