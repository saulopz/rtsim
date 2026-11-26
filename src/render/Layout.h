// Turns a simulation result into a list of simple drawing primitives.
// The same Drawing is rendered by the SVG writer and by the raylib viewer,
// so both always show exactly the same picture.
//
// Coordinates: x = time * pxPerUnit (x = 0 is t = 0), y grows downwards.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../core/Simulator.h"

namespace rtsim {

struct RGBA {
    std::uint8_t r = 0, g = 0, b = 0, a = 255;
};

struct LinePrim {
    float x1, y1, x2, y2;
    RGBA color;
    float width = 1.0f;
    int gridUnit = -1;    // >= 0: grid line at this time unit (viewer thins them out)
    float spacing = 0.0f; // > 0: repeats every `spacing` px (viewer hides it when too dense)
};

struct RectPrim {
    float x, y, w, h;
    RGBA fill;
    RGBA stroke;
};

struct PolyPrim {
    std::vector<float> xy;  // x0, y0, x1, y1, ...
    RGBA color;
    float width = 1.5f;
};

struct TextPrim {
    float x, y;  // baseline-left (or baseline-center if centered)
    std::string text;
    RGBA color;
    float size = 11.0f;
    bool centered = false;
    int axisUnit = -1;  // >= 0: time axis label (viewer thins them out)
};

struct RowBand {
    float y, h;
    std::string title;     // e.g. "SS3 (5.0, 1.5)"
    std::string subtitle;  // e.g. "sporadic server"
    bool shaded;
};

struct Drawing {
    float width = 0, height = 0;
    float pxPerUnit = 40.0f;
    float axisY = 0;
    std::vector<RowBand> rows;
    std::vector<LinePrim> grid;  // drawn first
    std::vector<RectPrim> rects;
    std::vector<LinePrim> lines;
    std::vector<PolyPrim> polys;
    std::vector<TextPrim> texts;
};

Drawing buildDrawing(const SimResult& res, float pxPerUnit = 40.0f);

}  // namespace rtsim
