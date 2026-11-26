#include "Viewer.h"

#include <raylib.h>
#include <rlgl.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace rtsim {

namespace {

constexpr float kGutter = 180.0f;  // fixed left column with row names
constexpr float kBarH = 52.0f;     // help bar at the top
constexpr int kInitW = 1280, kInitH = 720;

const char* kHelp1 = "Arrows/drag: move   Shift: fast   PgUp/PgDn/wheel: zoom   A: 1:1   F: fit   Home/End: begin/end";
const char* kHelp2 = "S: save PNG   Esc: exit";

// Monospace TTF if the system has one; raylib's built-in font otherwise.
const char* kFontCandidates[] = {
    "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
    "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
    "/usr/share/fonts/dejavu/DejaVuSansMono.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
    "/System/Library/Fonts/Menlo.ttc",
    "C:/Windows/Fonts/consola.ttf",
};

const ::Color kWhite{255, 255, 255, 255};
const ::Color kBlack{0, 0, 0, 255};

::Color rc(RGBA c) { return ::Color{c.r, c.g, c.b, c.a}; }

struct View {
    float offX = -16.0f;  // world x shown at the left edge of the timeline area
    float offY = -4.0f;  // world y shown right below the help bar
    float zoom = 1.0f;   // horizontal scale (world px -> screen px)
};

class App {
public:
    App(const Drawing& d) : d_(d) {}

    void load() {
        font_ = GetFontDefault();
        ownFont_ = false;
        for (const char* path : kFontCandidates) {
            if (!FileExists(path)) continue;
            Font f = LoadFontEx(path, 32, nullptr, 0);
            if (f.texture.id != 0) {
                SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
                font_ = f;
                ownFont_ = true;
                break;
            }
        }
    }

    void unload() {
        if (ownFont_) UnloadFont(font_);
    }

    void update() {
        // With event waiting the frame time can be long; cap it so a key held
        // down moves the view smoothly instead of jumping.
        const float dt = std::min(GetFrameTime(), 1.0f / 30.0f);
        const bool fast = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        const float speed = (fast ? 1600.0f : 400.0f) * dt;  // screen px per frame

        if (IsKeyDown(KEY_RIGHT)) v_.offX += speed / v_.zoom;
        if (IsKeyDown(KEY_LEFT)) v_.offX -= speed / v_.zoom;
        if (IsKeyDown(KEY_DOWN)) v_.offY += speed;
        if (IsKeyDown(KEY_UP)) v_.offY -= speed;

        const float centerX = kGutter + (GetScreenWidth() - kGutter) / 2.0f;
        if (IsKeyDown(KEY_PAGE_UP) || IsKeyDown(KEY_EQUAL) || IsKeyDown(KEY_KP_ADD))
            zoomAt(centerX, std::pow(2.0f, dt * 1.5f));
        if (IsKeyDown(KEY_PAGE_DOWN) || IsKeyDown(KEY_MINUS) || IsKeyDown(KEY_KP_SUBTRACT))
            zoomAt(centerX, std::pow(0.5f, dt * 1.5f));
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) zoomAt(std::max(GetMouseX() * 1.0f, kGutter), std::pow(1.15f, wheel));

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 md = GetMouseDelta();
            v_.offX -= md.x / v_.zoom;
            v_.offY -= md.y;
        }

        // Discrete commands come from the key queue, so a press is never lost
        // even if press and release happen within the same frame.
        for (int k = GetKeyPressed(); k != 0; k = GetKeyPressed()) {
            switch (k) {
                case KEY_HOME: v_.offX = -16.0f / v_.zoom; break;
                case KEY_END: v_.offX = d_.width - (GetScreenWidth() - kGutter - 40) / v_.zoom; break;
                case KEY_A: zoomAt(centerX, 1.0f / v_.zoom); break;
                case KEY_F:
                    v_.zoom = (GetScreenWidth() - kGutter - 40) / std::max(d_.width, 1.0f);
                    v_.offX = -16.0f / v_.zoom;
                    break;
                case KEY_S: wantPng_ = true; break;
                default: break;
            }
        }
        clamp();
    }

    void draw() {
        const float W = static_cast<float>(GetScreenWidth());
        const float H = static_cast<float>(GetScreenHeight());
        ClearBackground(kWhite);
        DrawRectangle(0, 0, static_cast<int>(W), static_cast<int>(H), kWhite);

        BeginScissorMode(0, static_cast<int>(kBarH), static_cast<int>(W), static_cast<int>(H - kBarH));
        for (const auto& r : d_.rows)
            if (r.shaded) DrawRectangleRec({0, sy(r.y), W, r.h}, ::Color{246, 246, 249, 255});
        EndScissorMode();

        BeginScissorMode(static_cast<int>(kGutter), static_cast<int>(kBarH), static_cast<int>(W - kGutter),
                         static_cast<int>(H - kBarH));
        const int gridStep = stepFor(8.0f);
        const int labelStep = stepFor(48.0f);
        const float x0 = v_.offX, x1 = v_.offX + (W - kGutter) / v_.zoom;

        for (const auto& l : d_.grid) {
            if (l.gridUnit >= 0 && l.gridUnit % gridStep != 0) continue;
            if (l.x1 < x0 - 1 || l.x1 > x1 + 1) continue;
            DrawLineEx({sx(l.x1), sy(l.y1)}, {sx(l.x2), sy(l.y2)}, l.width, rc(l.color));
        }
        for (const auto& r : d_.rects) {
            if (r.x + r.w < x0 || r.x > x1) continue;
            Rectangle rr{sx(r.x), sy(r.y), std::max(r.w * v_.zoom, 1.0f), r.h};
            DrawRectangleRec(rr, rc(r.fill));
            if (rr.width > 3) DrawRectangleLinesEx(rr, 1.0f, rc(r.stroke));
        }
        for (const auto& l : d_.lines) {
            if (l.gridUnit >= 0 && l.gridUnit % gridStep != 0) continue;
            if (l.spacing > 0 && l.spacing * v_.zoom < 6.0f) continue;
            if (std::max(l.x1, l.x2) < x0 - 10 || std::min(l.x1, l.x2) > x1 + 10) continue;
            DrawLineEx({sx(l.x1), sy(l.y1)}, {sx(l.x2), sy(l.y2)}, l.width, rc(l.color));
        }
        for (const auto& p : d_.polys)
            for (std::size_t i = 0; i + 3 < p.xy.size(); i += 2) {
                if (p.xy[i + 2] < x0 || p.xy[i] > x1) continue;
                DrawLineEx({sx(p.xy[i]), sy(p.xy[i + 1])}, {sx(p.xy[i + 2]), sy(p.xy[i + 3])}, p.width,
                           rc(p.color));
            }
        for (const auto& t : d_.texts) {
            if (t.axisUnit >= 0) {
                if (t.axisUnit % labelStep != 0) continue;
            } else if (v_.zoom < 0.35f) {
                continue;  // annotations would overlap when zoomed far out
            }
            if (t.x < x0 - 400 || t.x > x1 + 50) continue;
            float size = t.size + 3;
            Vector2 m = MeasureTextEx(font_, t.text.c_str(), size, 0);
            float x = sx(t.x) - (t.centered ? m.x / 2 : 0);
            DrawTextEx(font_, t.text.c_str(), {x, sy(t.y) - size + 2}, size, 0, rc(t.color));
        }

        // Cursor hairline with the time under the mouse.
        const float mx = static_cast<float>(GetMouseX());
        if (mx > kGutter && GetMouseY() > kBarH) {
            DrawLineEx({mx, kBarH}, {mx, H}, 1.0f, ::Color{0, 0, 0, 60});
        }
        EndScissorMode();

        // Sticky gutter with the row names.
        DrawRectangleRec({0, kBarH, kGutter, H - kBarH}, ::Color{255, 255, 255, 235});
        BeginScissorMode(0, static_cast<int>(kBarH), static_cast<int>(kGutter), static_cast<int>(H - kBarH));
        for (const auto& r : d_.rows) {
            if (r.shaded) DrawRectangleRec({0, sy(r.y), kGutter, r.h}, ::Color{246, 246, 249, 255});
            DrawTextEx(font_, r.title.c_str(), {10, sy(r.y) + 8}, 17, 0, ::Color{34, 34, 34, 255});
            DrawTextEx(font_, r.subtitle.c_str(), {10, sy(r.y) + 28}, 13, 0, ::Color{100, 100, 100, 255});
        }
        EndScissorMode();
        DrawLineEx({kGutter, kBarH}, {kGutter, H}, 1.0f, ::Color{180, 180, 180, 255});

        // Help bar (same lavender as the 2006 version).
        DrawRectangleRec({0, 0, W, kBarH}, ::Color{200, 200, 240, 255});
        DrawTextEx(font_, kHelp1, {8, 6}, 15, 0, kBlack);
        char status[160];
        float tUnits = (mx > kGutter) ? ((mx - kGutter) / v_.zoom + v_.offX) / d_.pxPerUnit : -1.0f;
        if (tUnits >= 0)
            std::snprintf(status, sizeof status, "%s      t = %.1f      zoom %.2fx", kHelp2, tUnits, v_.zoom);
        else
            std::snprintf(status, sizeof status, "%s      zoom %.2fx", kHelp2, v_.zoom);
        DrawTextEx(font_, status, {8, 28}, 15, 0, kBlack);
    }

private:
    bool wantPng_ = false;
    float sx(float x) const { return kGutter + (x - v_.offX) * v_.zoom; }
    float sy(float y) const { return kBarH + (y - v_.offY); }

    // Smallest "nice" step (in time units) whose on-screen spacing is >= minPx.
    int stepFor(float minPx) const {
        static const int steps[] = {1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000};
        for (int s : steps)
            if (s * d_.pxPerUnit * v_.zoom >= minPx) return s;
        return 100000;
    }

    void zoomAt(float screenX, float factor) {
        float worldX = (screenX - kGutter) / v_.zoom + v_.offX;
        v_.zoom = std::clamp(v_.zoom * factor, 0.002f, 12.0f);
        v_.offX = worldX - (screenX - kGutter) / v_.zoom;
    }

    void clamp() {
        float visibleW = (GetScreenWidth() - kGutter) / v_.zoom;
        v_.offX = std::clamp(v_.offX, -visibleW * 0.5f, std::max(d_.width - visibleW * 0.5f, 0.0f));
        float visibleH = GetScreenHeight() - kBarH;
        v_.offY = std::clamp(v_.offY, -visibleH * 0.5f, std::max(d_.height - visibleH * 0.5f, 0.0f));
    }

public:
    // Called between draw() and EndDrawing(), while the frame is in the back buffer.
    void savePngIfRequested() {
        if (!wantPng_) return;
        wantPng_ = false;
        static int counter = 0;
        char name[64];
        std::snprintf(name, sizeof name, "rtsim_%03d.png", ++counter);
        rlDrawRenderBatchActive();  // flush pending draws before reading the framebuffer
        Image img = LoadImageFromScreen();
        ExportImage(img, name);
        UnloadImage(img);
        TraceLog(LOG_WARNING, "saved %s", name);
    }

private:
    const Drawing& d_;
    View v_;
    Font font_{};
    bool ownFont_ = false;
};

}  // namespace

int runViewer(const Drawing& d, const std::string& title, const std::string& screenshotPath) {
    const bool shot = !screenshotPath.empty();
    SetTraceLogLevel(LOG_WARNING);
    unsigned flags = FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT;
    if (shot) flags |= FLAG_WINDOW_HIDDEN;
    SetConfigFlags(flags);
    InitWindow(kInitW, kInitH, title.c_str());
    if (!IsWindowReady()) {
        std::fprintf(stderr, "rtsim: could not open a window (no display?). Use --no-gui.\n");
        return 1;
    }
    SetTargetFPS(60);

    App app(d);
    app.load();

    int rc = 0;
    if (shot) {
        RenderTexture2D target = LoadRenderTexture(kInitW, kInitH);
        BeginTextureMode(target);
        app.draw();
        EndTextureMode();
        Image img = LoadImageFromTexture(target.texture);
        ImageFlipVertical(&img);
        if (!ExportImage(img, screenshotPath.c_str())) rc = 1;
        UnloadImage(img);
        UnloadRenderTexture(target);
    } else {
        EnableEventWaiting();  // redraw only on input: ~0% CPU while idle
        while (!WindowShouldClose()) {
            app.update();
            BeginDrawing();
            app.draw();
            app.savePngIfRequested();
            EndDrawing();
        }
    }
    app.unload();
    CloseWindow();
    return rc;
}

}  // namespace rtsim
