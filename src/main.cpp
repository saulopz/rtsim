/***********************************************************************
 * FEDERAL UNIVERSITY OF SANTA CATARINA - UFSC
 * POSTGRADUATE PROGRAM IN AUTOMATION AND SYSTEMS ENGINEERING - PPGEAS
 * CLASS: DAS6663 - REAL TIME SYSTEMS
 * AUTHOR: SAULO POPOV ZAMBIASI
 * PROGRAM: RTSIM (2006; modernized 2026: raylib, integer time, SVG output)
 * *********************************************************************/

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

#include "core/Analysis.h"
#include "core/Parser.h"
#include "core/Report.h"
#include "core/Simulator.h"
#include "render/Layout.h"
#include "render/SvgWriter.h"

#ifdef RTSIM_WITH_GUI
#include "viewer/Viewer.h"
#endif

using namespace rtsim;

static void usage() {
    std::printf(
        "Usage: rtsim [options] <file.in>\n"
        "\n"
        "Writes <file>.out (text report) and <file>.svg (timeline), then opens\n"
        "the interactive viewer.\n"
        "\n"
        "Options:\n"
        "  --no-gui            only write the output files\n"
        "  --out FILE          text report path (default <file>.out)\n"
        "  --svg FILE          SVG path (default <file>.svg)\n"
        "  --length UNITS      simulate [0, UNITS) instead of the hyperperiod\n"
        "  --screenshot FILE   render the viewer once to a PNG and exit\n"
        "  -h, --help          this help\n");
}

int main(int argc, char* argv[]) {
    std::string input, outPath, svgPath, screenshot;
    bool gui = true;
    SimOptions simOpt;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "rtsim: %s needs a value\n", a.c_str());
                std::exit(2);
            }
            return argv[++i];
        };
        if (a == "-h" || a == "--help") {
            usage();
            return 0;
        } else if (a == "--no-gui") gui = false;
        else if (a == "--out") outPath = next();
        else if (a == "--svg") svgPath = next();
        else if (a == "--screenshot") screenshot = next();
        else if (a == "--length") {
            std::string v = next();
            try {
                simOpt.length = parseTime(v);
            } catch (const std::exception& e) {
                std::fprintf(stderr, "rtsim: --length: %s\n", e.what());
                return 2;
            }
        } else if (!a.empty() && a[0] == '-') {
            std::fprintf(stderr, "rtsim: unknown option %s\n", a.c_str());
            return 2;
        } else if (input.empty()) input = a;
        else {
            usage();
            return 2;
        }
    }
    if (input.empty()) {
        usage();
        return 2;
    }

    // Accept both "samples/janeliu" (old usage) and "samples/janeliu.in".
    std::string base = input;
    if (base.size() > 3 && base.compare(base.size() - 3, 3, ".in") == 0) base.erase(base.size() - 3);
    std::string inPath = base + ".in";
    if (outPath.empty()) outPath = base + ".out";
    if (svgPath.empty()) svgPath = base + ".svg";

    std::ifstream in(inPath);
    if (!in) {
        std::fprintf(stderr, "rtsim: cannot open %s\n", inPath.c_str());
        return 1;
    }

    SystemSpec sys;
    std::vector<std::string> warnings;
    try {
        sys = parseSystem(in, warnings);
    } catch (const ParseError& e) {
        std::fprintf(stderr, "rtsim: %s: %s\n", inPath.c_str(), e.what());
        return 1;
    }
    for (const auto& w : warnings) std::fprintf(stderr, "rtsim: warning: %s\n", w.c_str());

    Analysis an = analyze(sys);
    SimResult res = simulate(sys, simOpt);
    res.warnings.insert(res.warnings.begin(), warnings.begin(), warnings.end());

    writeReport(std::cout, sys, an, res);

    std::ofstream out(outPath);
    if (!out) {
        std::fprintf(stderr, "rtsim: cannot write %s\n", outPath.c_str());
        return 1;
    }
    writeReport(out, sys, an, res);

    Drawing drawing = buildDrawing(res);
    std::ofstream svg(svgPath);
    if (!svg) {
        std::fprintf(stderr, "rtsim: cannot write %s\n", svgPath.c_str());
        return 1;
    }
    writeSvg(svg, drawing);
    std::printf("\nWrote %s and %s\n", outPath.c_str(), svgPath.c_str());

    if (gui || !screenshot.empty()) {
#ifdef RTSIM_WITH_GUI
        return runViewer(drawing, "RTSim - " + inPath, screenshot);
#else
        std::fprintf(stderr, "rtsim: built without the viewer (raylib); use --no-gui\n");
        return gui && screenshot.empty() ? 0 : 1;
#endif
    }
    return 0;
}
