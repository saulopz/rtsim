#include "Layout.h"

namespace rtsim {

namespace {

const RGBA kInk{34, 34, 34};
const RGBA kMuted{120, 120, 120};
const RGBA kGridMinor{236, 236, 236};
const RGBA kGridMajor{205, 205, 205};

RGBA rgba(Color c) { return {c.r, c.g, c.b, 255}; }

// Slightly darker outline so pale fill colors stay visible on white.
RGBA outline(Color c) {
    return {static_cast<std::uint8_t>(c.r * 0.6), static_cast<std::uint8_t>(c.g * 0.6),
            static_cast<std::uint8_t>(c.b * 0.6), 255};
}

std::string spec(const TaskSpec& t) { return "(" + formatTicks(t.period) + ", " + formatTicks(t.exec) + ")"; }

}  // namespace

Drawing buildDrawing(const SimResult& res, float pxPerUnit) {
    Drawing d;
    d.pxPerUnit = pxPerUnit;
    const float pxPerTick = pxPerUnit / TICKS_PER_UNIT;
    const float width = res.length * pxPerTick;
    const long units = static_cast<long>(res.length / TICKS_PER_UNIT);
    auto X = [&](Tick t) { return t * pxPerTick; };

    float y = 8.0f;
    bool shade = false;

    auto taskBaseline = [&](const TaskTrace& tr, float base, bool periodMarks) {
        d.lines.push_back({0, base, width, base, kInk, 1.0f});
        for (long u = 0; u <= units; ++u)
            d.lines.push_back({u * pxPerUnit, base, u * pxPerUnit, base + 4, kMuted, 1.0f, static_cast<int>(u)});
        if (periodMarks)
            for (Tick t = 0; t <= res.length; t += tr.spec.period)
                d.lines.push_back({X(t), base - 30, X(t), base + 6, kInk, 1.5f, -1, X(tr.spec.period)});
    };

    for (const auto& tr : res.tasks) {
        const bool server = isServer(tr.spec.kind);
        const float rowH = server ? 86.0f : 64.0f;
        const float base = y + rowH - 14.0f;
        d.rows.push_back({y, rowH, tr.spec.name + " " + spec(tr.spec), kindName(tr.spec.kind), shade});
        shade = !shade;

        // Fixed replenishment period marks only make sense for PS and DS.
        taskBaseline(tr, base, tr.spec.kind != TaskKind::SporadicServer);

        for (const auto& s : tr.segments) {
            Color c = s.color;
            d.rects.push_back({X(s.start), base - 20, X(s.end) - X(s.start), 20, rgba(c), outline(c)});
        }

        if (!server) {
            for (const auto& j : tr.jobs) {
                if (!j.missed) continue;
                // Deadline miss: a black cross with text (does not rely on color).
                float x = X(j.deadline);
                float my = base - 40;
                d.lines.push_back({x - 5, my - 5, x + 5, my + 5, kInk, 2.0f});
                d.lines.push_back({x - 5, my + 5, x + 5, my - 5, kInk, 2.0f});
                d.texts.push_back({x + 8, my + 4, "miss", kInk, 11.0f});
            }
        } else {
            for (std::size_t k = 0; k < tr.aperiodics.size(); ++k) {
                const auto& a = tr.aperiodics[k];
                float x = X(a.spec.release);
                float ly = base - (k % 2 == 0 ? 36.0f : 54.0f);
                d.lines.push_back({x, base, x, ly - 10, rgba(a.spec.color), 2.0f});
                // Arrowhead pointing up (arrival).
                d.lines.push_back({x, ly - 10, x - 4, ly - 4, rgba(a.spec.color), 2.0f});
                d.lines.push_back({x, ly - 10, x + 4, ly - 4, rgba(a.spec.color), 2.0f});
                std::string label = "A" + std::to_string(k + 1) + " r=" + formatTicks(a.spec.release) +
                                    " e=" + formatTicks(a.spec.exec) +
                                    (a.done() ? " R=" + formatTicks(a.response()) : std::string(" unfinished"));
                d.texts.push_back({x + 6, ly - 2, label, kInk, 11.0f});
            }
        }
        y += rowH;

        if (server) {
            const float bh = 62.0f;
            const float top = y + 16, bottom = y + bh - 8;
            d.rows.push_back({y, bh, "budget " + tr.spec.name, "es = " + formatTicks(tr.spec.exec), shade});
            shade = !shade;
            d.lines.push_back({0, top, width, top, kGridMajor, 1.0f});
            d.lines.push_back({0, bottom, width, bottom, kInk, 1.0f});
            PolyPrim p;
            p.color = rgba(tr.spec.color);
            p.width = 2.0f;
            for (const auto& b : tr.budget) {
                float v = static_cast<float>(b.value) / static_cast<float>(tr.spec.exec);
                p.xy.push_back(X(b.t));
                p.xy.push_back(bottom - v * (bottom - top));
            }
            d.polys.push_back(std::move(p));
            y += bh;
        }
    }

    // Time axis.
    d.axisY = y + 6;
    d.lines.push_back({0, d.axisY, width, d.axisY, kInk, 1.5f});
    for (long u = 0; u <= units; ++u) {
        bool major = (u % 5 == 0);
        d.grid.push_back({u * pxPerUnit, 0, u * pxPerUnit, d.axisY, major ? kGridMajor : kGridMinor, 1.0f,
                          static_cast<int>(u)});
        d.lines.push_back({u * pxPerUnit, d.axisY, u * pxPerUnit, d.axisY + (major ? 7.0f : 4.0f), kInk, 1.0f,
                           static_cast<int>(u)});
        d.texts.push_back({u * pxPerUnit, d.axisY + 20, std::to_string(u), kInk, 11.0f, true, static_cast<int>(u)});
    }
    d.width = width;
    d.height = d.axisY + 30;
    return d;
}

}  // namespace rtsim
