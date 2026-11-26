#include "SvgWriter.h"

#include <cstdio>
#include <string>

namespace rtsim {

namespace {

constexpr float kGutter = 170.0f;  // left column with the row names
constexpr float kRight = 30.0f;

std::string col(RGBA c) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "#%02x%02x%02x", c.r, c.g, c.b);
    return buf;
}

std::string num(float v) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.2f", v);
    std::string s = buf;
    while (s.back() == '0') s.pop_back();
    if (s.back() == '.') s.pop_back();
    return s;
}

std::string esc(const std::string& s) {
    std::string o;
    for (char c : s) {
        switch (c) {
            case '&': o += "&amp;"; break;
            case '<': o += "&lt;"; break;
            case '>': o += "&gt;"; break;
            case '"': o += "&quot;"; break;
            default: o += c;
        }
    }
    return o;
}

void line(std::ostream& out, const LinePrim& l) {
    out << "<line x1=\"" << num(l.x1) << "\" y1=\"" << num(l.y1) << "\" x2=\"" << num(l.x2) << "\" y2=\""
        << num(l.y2) << "\" stroke=\"" << col(l.color) << "\" stroke-width=\"" << num(l.width) << "\"/>\n";
}

}  // namespace

void writeSvg(std::ostream& out, const Drawing& d) {
    const float W = kGutter + d.width + kRight;
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << num(W) << "\" height=\"" << num(d.height)
        << "\" viewBox=\"0 0 " << num(W) << " " << num(d.height) << "\">\n"
        << "<rect width=\"100%\" height=\"100%\" fill=\"#ffffff\"/>\n"
        << "<g font-family=\"DejaVu Sans Mono, Menlo, Consolas, monospace\">\n";

    for (const auto& r : d.rows) {
        if (r.shaded)
            out << "<rect x=\"0\" y=\"" << num(r.y) << "\" width=\"" << num(W) << "\" height=\"" << num(r.h)
                << "\" fill=\"#f6f6f9\"/>\n";
        out << "<text x=\"10\" y=\"" << num(r.y + 22) << "\" font-size=\"13\" font-weight=\"bold\" fill=\"#222222\">"
            << esc(r.title) << "</text>\n";
        out << "<text x=\"10\" y=\"" << num(r.y + 38) << "\" font-size=\"11\" fill=\"#666666\">" << esc(r.subtitle)
            << "</text>\n";
    }

    out << "<g transform=\"translate(" << num(kGutter) << ",0)\">\n";
    for (const auto& l : d.grid) line(out, l);
    for (const auto& r : d.rects)
        out << "<rect x=\"" << num(r.x) << "\" y=\"" << num(r.y) << "\" width=\"" << num(r.w) << "\" height=\""
            << num(r.h) << "\" fill=\"" << col(r.fill) << "\" stroke=\"" << col(r.stroke)
            << "\" stroke-width=\"1\"/>\n";
    for (const auto& l : d.lines) line(out, l);
    for (const auto& p : d.polys) {
        out << "<polyline fill=\"none\" stroke=\"" << col(p.color) << "\" stroke-width=\"" << num(p.width)
            << "\" stroke-linejoin=\"round\" points=\"";
        for (std::size_t i = 0; i + 1 < p.xy.size(); i += 2) out << num(p.xy[i]) << "," << num(p.xy[i + 1]) << " ";
        out << "\"/>\n";
    }
    for (const auto& t : d.texts) {
        out << "<text x=\"" << num(t.x) << "\" y=\"" << num(t.y) << "\" font-size=\"" << num(t.size) << "\" fill=\""
            << col(t.color) << "\"" << (t.centered ? " text-anchor=\"middle\"" : "") << ">" << esc(t.text) << "</text>\n";
    }
    out << "</g>\n</g>\n</svg>\n";
}

}  // namespace rtsim
