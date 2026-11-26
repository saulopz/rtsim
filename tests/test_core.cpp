// Minimal self-contained tests for the simulation core (no framework needed).
#include <cstdio>
#include <sstream>
#include <string>

#include "core/Analysis.h"
#include "core/Parser.h"
#include "core/Simulator.h"

using namespace rtsim;

static int failures = 0;
#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);   \
            ++failures;                                                   \
        }                                                                 \
    } while (0)

static SystemSpec parse(const std::string& text) {
    std::istringstream in(text);
    std::vector<std::string> w;
    return parseSystem(in, w);
}

static const TaskTrace& find(const SimResult& r, const char* name) {
    for (const auto& t : r.tasks)
        if (t.spec.name == name) return t;
    std::printf("no task %s\n", name);
    std::abort();
}

static void testNumbers() {
    CHECK(parseTime("0,5") == 5);
    CHECK(parseTime("0.5") == 5);
    CHECK(parseTime("15,5") == 155);
    CHECK(parseTime("3") == 30);
    bool threw = false;
    try { parseTime("0.05"); } catch (const std::invalid_argument&) { threw = true; }
    CHECK(threw);
    threw = false;
    try { parseTime("1,5x"); } catch (const std::invalid_argument&) { threw = true; }
    CHECK(threw);
}

static void testParserMixedSeparators() {
    SystemSpec s = parse(
        "periodic 3 0,5 0 0 0\n"
        "simple_sporadic_server 5 1.5 100 100 255\n"
        "aperiodic 15,5 2,0 0 0 255\n");
    CHECK(s.tasks.size() == 2);
    CHECK(s.tasks[0].exec == 5);
    CHECK(s.tasks[1].exec == 15);
    CHECK(s.tasks[1].aperiodics.size() == 1);
    CHECK(s.tasks[1].aperiodics[0].release == 155);  // not truncated to 15
    bool threw = false;
    try { parse("periodic 3 1\naperiodic 1 1\n"); } catch (const ParseError&) { threw = true; }
    CHECK(threw);  // aperiodic after a periodic task is an error now
}

// T1=(3,1), T2=(4,1), T3=(10,2): hand-computed RTA gives R3 = 6.
static void testRtaAndSimulationAgree() {
    SystemSpec s = parse("periodic 10 2\nperiodic 3 1\nperiodic 4 1\n");
    CHECK(s.tasks[0].period == 30 && s.tasks[2].period == 100);  // RM order
    Analysis an = analyze(s);
    CHECK(an.tasks[0].response == 10);
    CHECK(an.tasks[1].response == 20);
    CHECK(an.tasks[2].response == 60);
    CHECK(an.rtaPasses);
    SimResult r = simulate(s);
    CHECK(r.length == 600);  // hyperperiod 60
    CHECK(r.tasks[2].jobs[0].finish == 60);
    for (const auto& t : r.tasks)
        for (const auto& j : t.jobs) CHECK(!j.missed);
}

// e72.in: U = 1/3 + 1.5/4 + 6/20 > 1, so T3 must miss.
static void testOverload() {
    SystemSpec s = parse("periodic 3 1,0\nperiodic 4 1,5\nperiodic 20 6\n");
    Analysis an = analyze(s);
    CHECK(an.utilization > 1.0);
    CHECK(!an.tasks[2].schedulable);
    CHECK(!an.rtaPasses);
    SimResult r = simulate(s);
    int misses = 0;
    for (const auto& j : r.tasks[2].jobs) misses += j.missed;
    CHECK(misses > 0);
}

// Same aperiodic job served by a polling and by a deferrable server.
static void testPollingVsDeferrable() {
    SystemSpec ps = parse("polling_server 5 1\naperiodic 1 1\nperiodic 10 3\n");
    SimResult rp = simulate(ps);
    const auto& apP = find(rp, "PS1").aperiodics[0];
    CHECK(apP.start == 50);      // waits for the next poll
    CHECK(apP.response() == 50);  // finishes at 6, released at 1

    SystemSpec ds = parse("deferrable_server 5 1\naperiodic 1 1\nperiodic 10 3\n");
    SimResult rd = simulate(ds);
    const auto& apD = find(rd, "DS1").aperiodics[0];
    CHECK(apD.start == 10);  // budget preserved, served immediately
    CHECK(apD.response() == 10);
}

// Server with highest priority: no T_H, so te = tf and tc = tf + ps.
static void testSporadicBasic() {
    SystemSpec s = parse("sporadic_server 5 1\naperiodic 2 1\naperiodic 4 1\nperiodic 10 4\n");
    SimResult r = simulate(s);
    const auto& ss = find(r, "SS1");
    CHECK(ss.aperiodics[0].start == 20 && ss.aperiodics[0].finish == 30);
    // budget exhausted at 3, next replenishment at tf + ps = 7
    CHECK(ss.aperiodics[1].start == 70);
    CHECK(ss.aperiodics[1].response() == 40);
    bool replenishedAt7 = false;
    for (Tick t : ss.replenishments) replenishedAt7 = replenishedAt7 || t == 70;
    CHECK(replenishedAt7);
}

// Liu's example (janeliu.in). At t=3 A1 arrives but T1 is ready, so T_H is
// busy from BEGIN=3 to END=3.5; the server starts at tf=3.5 = END, so
// te = max(tr, BEGIN) = 3 and the next replenishment is at 8.
static void testSporadicLiuExample() {
    SystemSpec s = parse(
        "periodic 3 0,5\nperiodic 4 1,0\nperiodic 19 4,5\n"
        "simple_sporadic_server 5 1.5\n"
        "aperiodic 3 1,0\naperiodic 7 2,0\naperiodic 15,5 2,0\n");
    SimResult r = simulate(s);
    const auto& ss = find(r, "SS3");
    CHECK(ss.aperiodics[0].start == 35);
    bool at8 = false;
    for (Tick t : ss.replenishments) at8 = at8 || t == 80;
    CHECK(at8);
}

int main() {
    testNumbers();
    testParserMixedSeparators();
    testRtaAndSimulationAgree();
    testOverload();
    testPollingVsDeferrable();
    testSporadicBasic();
    testSporadicLiuExample();
    if (failures == 0) std::printf("all tests passed\n");
    return failures == 0 ? 0 : 1;
}
