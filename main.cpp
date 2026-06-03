// main.cpp - Modern shader-powered 3D shape intersection visualizer
// Powerful extensible architecture, all rendering on shaders, custom GL GUI

#include <windows.h>
#include <stdio.h>
#include "math3d.h"
#include "gl_core.h"
#include "shapes.h"
#include "renderer.h"
#include "gui.h"

// App state
static HWND g_hWnd = NULL;
static HDC g_hdc = NULL;
static HGLRC g_hrc = NULL;
static int g_winW = 1280, g_winH = 720;

static Renderer g_renderer;
static ShapeScene g_scene;
static int g_currentPreset = 0;
static float g_animTime = 0.0f;
static bool g_paused = false;

static bool g_mouseDown = false;
static int g_lastMX = 0, g_lastMY = 0;
static int g_dragMode = 0; // 0=none, 1=orbit cam, 2=move A, 3=move B, 4=rotate A/B

// Forward
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void SetupModernGL();
void RenderFrame();
void UpdateLogic(float dt);
void HandleInput();
void OnResize(int w, int h);

// Helpers
static Vec3 ScreenToWorldDelta(int dx, int dy, float sens);

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nShow) {
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"ShapeIntersectGL4";

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, L"Register failed", L"Error", MB_OK);
        return 0;
    }

    g_hWnd = CreateWindowEx(0, L"ShapeIntersectGL4",
        L"Shape Intersection Lab - Modern OpenGL + Shaders",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, g_winW, g_winH,
        NULL, NULL, hInst, NULL);

    if (!g_hWnd) return 0;

    ShowWindow(g_hWnd, nShow);
    UpdateWindow(g_hWnd);

    // Init GL
    g_hdc = GetDC(g_hWnd);
    SetupModernGL();

    // Init systems
    if (!LoadGLProcs(g_hdc)) {
        MessageBoxA(NULL, "OpenGL functions load failed. Need driver >= 3.3", "Fatal", MB_ICONERROR);
        return 1;
    }

    // Recreate modern context
    HGLRC modern = CreateModernContext(g_hdc, 3, 3, false);
    if (modern) {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(g_hrc);
        g_hrc = modern;
        wglMakeCurrent(g_hdc, g_hrc);
    }

    SetVSync(true);

    if (!RendererInit(g_renderer)) {
        MessageBoxA(NULL, "Failed to compile shaders", "Renderer", MB_ICONERROR);
        return 1;
    }
    SetUIRenderer(&g_renderer);

    // Font for GUI text
    if (!CreateFontAtlas(g_hdc)) {
        MessageBoxA(NULL, "Font atlas creation failed", "GUI", MB_ICONERROR);
        return 1;
    }

    g_scene.InitDefault();

    // Initial sample
    SampleAndComputeIntersections(g_scene, 2200, g_renderer);

    // Timer
    LARGE_INTEGER freq, t0, t1;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t0);

    MSG msg = {0};
    while (msg.message != WM_QUIT) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        QueryPerformanceCounter(&t1);
        float dt = float(t1.QuadPart - t0.QuadPart) / freq.QuadPart;
        t0 = t1;
        if (dt > 0.1f) dt = 0.1f;

        UpdateLogic(dt);
        RenderFrame();
        SwapBuffers(g_hdc);

        // 60 fps cap rough
        Sleep(1);
    }

    DestroyFontAtlas();
    RendererShutdown(g_renderer);
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(g_hrc);
    ReleaseDC(g_hWnd, g_hdc);
    return 0;
}

void SetupModernGL() {
    PIXELFORMATDESCRIPTOR pfd = {0};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    int pf = ChoosePixelFormat(g_hdc, &pfd);
    SetPixelFormat(g_hdc, pf, &pfd);

    g_hrc = wglCreateContext(g_hdc);
    wglMakeCurrent(g_hdc, g_hrc);

    // Load minimal to get wglCreateContextAttribs
    LoadGLProcs(g_hdc);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
    case WM_CREATE:
        return 0;

    case WM_SIZE: {
        int w = LOWORD(l), h = HIWORD(l);
        if (w > 0 && h > 0) {
            g_winW = w; g_winH = h;
            glViewport(0, 0, w, h);
            OnResize(w, h);
        }
        return 0;
    }

    case WM_MOUSEMOVE: {
        int mx = LOWORD(l), my = HIWORD(l);
        if (g_mouseDown) {
            int dx = mx - g_lastMX;
            int dy = my - g_lastMY;

            float sens = 0.6f;
            if (GetKeyState(VK_SHIFT) & 0x8000) sens *= 0.1f; // slowly / precise with Shift
            if (g_dragMode == 1) {
                g_renderer.cam.Orbit(dx * 0.35f, -dy * 0.35f);
            } else if (g_dragMode == 2) {
                // move A (slow with Shift)
                Vec3 d = ScreenToWorldDelta(dx, dy, sens);
                g_scene.GetA().pos = g_scene.GetA().pos + d;
            } else if (g_dragMode == 3) {
                Vec3 d = ScreenToWorldDelta(dx, dy, sens);
                g_scene.GetB().pos = g_scene.GetB().pos + d;
            } else if (g_dragMode == 4) {
                float rs = (GetKeyState(VK_SHIFT) & 0x8000) ? 0.06f : 0.6f;
                g_scene.GetA().euler.y += dx * rs;
                g_scene.GetB().euler.y += dx * rs;
                g_scene.GetA().euler.x += dy * (rs * 0.8f);
                g_scene.GetB().euler.x += dy * (rs * 0.8f);
            }
            SampleAndComputeIntersections(g_scene, 1800, g_renderer);
        }
        g_lastMX = mx; g_lastMY = my;

        // feed GUI
        GUIBegin(g_winW, g_winH, mx, my, g_mouseDown);
        return 0;
    }

    case WM_LBUTTONDOWN:
        g_mouseDown = true;
        SetCapture(hwnd);
        g_lastMX = LOWORD(l); g_lastMY = HIWORD(l);

        // decide drag mode based on modifiers / GUI hit test later
        if (GetKeyState(VK_CONTROL) & 0x8000) g_dragMode = 2;      // ctrl = move A
        else if (GetKeyState(VK_SHIFT) & 0x8000) g_dragMode = 3;    // shift = move B
        else if (GetKeyState(VK_MENU) & 0x8000) g_dragMode = 4;     // alt = rotate both
        else g_dragMode = 1; // orbit
        return 0;

    case WM_LBUTTONUP:
        g_mouseDown = false;
        g_dragMode = 0;
        ReleaseCapture();
        return 0;

    case WM_MOUSEWHEEL: {
        float d = GET_WHEEL_DELTA_WPARAM(w) / 120.0f;
        g_renderer.cam.Zoom(-d * 0.8f);
        return 0;
    }

    case WM_KEYDOWN:
        if (w == VK_SPACE) { g_paused = !g_paused; return 0; }
        if (w == 'R') { g_renderer.cam = OrbitCamera(); g_renderer.cam.distance = 520; g_renderer.cam.target = Vec3(0,0,-80); return 0; }
        if (w == 'T') { g_renderer.rotateObjects = !g_renderer.rotateObjects; return 0; }
        if (w == 'P') { g_renderer.showPoints = !g_renderer.showPoints; return 0; }
        if (w == 'W') { g_renderer.showWire = !g_renderer.showWire; return 0; }
        if (w == 'H') { g_renderer.highlightIntersect = !g_renderer.highlightIntersect; return 0; }
        if (w == 'A') { g_renderer.showAxes = !g_renderer.showAxes; return 0; }
        if (w == VK_F5) { ReloadShaders(g_renderer); return 0; }
        if (w == VK_ESCAPE) { PostQuitMessage(0); return 0; }

        // quick pair change
        if (w >= '1' && w <= '5') {
            g_scene.SetPair(0, (w - '1') % (int)g_scene.shapes.size());
            SampleAndComputeIntersections(g_scene, 2000, g_renderer);
        }
        return 0;

    case WM_COMMAND:
        // future menu
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_ERASEBKGND:
        return 1;
    }
    return DefWindowProc(hwnd, msg, w, l);
}

void OnResize(int w, int h) {
    g_renderer.cam.aspect = w > 0 ? float(w) / float(h) : 1.6f;
}

static Vec3 ScreenToWorldDelta(int dx, int dy, float sens) {
    // Approximate pan in camera plane
    float move = g_renderer.cam.distance * sens * 0.0022f;
    Vec3 right = Vec3(cosf(M3D_DEG2RAD(g_renderer.cam.yaw)), 0, sinf(M3D_DEG2RAD(g_renderer.cam.yaw)));
    Vec3 up = Vec3(0, 1, 0);
    return right * (float(-dx) * move) + up * (float(dy) * move);
}

void UpdateLogic(float dt) {
    if (!g_paused) {
        g_animTime += dt * 60.0f;
        if (g_renderer.rotateObjects) {
            float rs = g_renderer.rotSpeed;
            g_scene.GetA().euler.y += rs * 0.7f * dt * 60.0f;
            g_scene.GetB().euler.y += rs * 1.15f * dt * 60.0f;
            g_scene.GetA().euler.x = sinf(g_animTime * 0.002f) * 6.0f;
        }
        // gentle animation of B along arc
        float phase = g_animTime * 0.018f;
        Vec3 base = g_scene.GetB().pos;
        g_scene.GetB().pos.z = -210.0f + sinf(phase) * 95.0f;
        g_scene.GetB().pos.x = sinf(phase * 0.6f) * 35.0f;
    }

    // Recompute intersections at lower rate or on demand
    static int frame = 0;
    if ((++frame % 3) == 0) {
        SampleAndComputeIntersections(g_scene, 2400, g_renderer);
    }
}

void RenderFrame() {
    RendererBeginFrame(g_renderer, g_winW, g_winH);

    // Draw context shapes first (faded)
    for (size_t i = 0; i < g_scene.shapes.size(); ++i) {
        auto& s = g_scene.shapes[i];
        if (i == g_scene.indexA || i == g_scene.indexB) continue;
        ShapeInstance tmp = s;
        tmp.alpha = 0.35f;
        DrawShape(g_renderer, tmp, nullptr);
    }

    // Main A (no other tint for base)
    DrawShape(g_renderer, g_scene.GetA(), &g_scene.GetB());

    // Main B (with A as test)
    DrawShape(g_renderer, g_scene.GetB(), &g_scene.GetA());

    // Axes
    DrawAxes(g_renderer);

    // Intersection markers (shader points)
    DrawIntersectionPoints(g_renderer, g_renderer.intersectPtsA, Vec3(1.0f, 0.2f, 1.0f), 3.2f);
    DrawIntersectionPoints(g_renderer, g_renderer.intersectPtsB, Vec3(1.0f, 1.0f, 0.15f), 3.2f);

    // Draw intersection curve approximation as line strip through the sampled boundary points
    // (dense surface sampling traces the intersection "line" reasonably for visualization)
    {
      auto& pts = g_renderer.intersectPtsA.size() > g_renderer.intersectPtsB.size() ? g_renderer.intersectPtsA : g_renderer.intersectPtsB;
      if (pts.size() > 3) {
        glDisable(GL_DEPTH_TEST);
        gl.glUseProgram(g_renderer.progUI);
        Mat4 vp = Mul(g_renderer.cam.GetProj(), g_renderer.cam.GetView());
        GLint loc = gl.glGetUniformLocation(g_renderer.progUI, "uOrtho");
        if (loc >= 0) gl.glUniformMatrix4fv(loc, 1, GL_FALSE, vp.m);
        loc = gl.glGetUniformLocation(g_renderer.progUI, "uRectColor");
        if (loc >= 0) gl.glUniform4f(loc, 1.0f, 0.85f, 0.2f, 0.9f); // bright yellow-orange curve
        glLineWidth(2.8f);
        GLuint vao = 0, vbo = 0;
        gl.glGenVertexArrays(1, &vao);
        gl.glGenBuffers(1, &vbo);
        gl.glBindVertexArray(vao);
        gl.glBindBuffer(GL_ARRAY_BUFFER, vbo);
        gl.glBufferData(GL_ARRAY_BUFFER, pts.size() * sizeof(Vec3), pts.data(), GL_DYNAMIC_DRAW);
        gl.glEnableVertexAttribArray(0);
        gl.glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3), 0);
        glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)pts.size());
        gl.glDeleteVertexArrays(1, &vao);
        gl.glDeleteBuffers(1, &vbo);
        glLineWidth(1.0f);
        glEnable(GL_DEPTH_TEST);
      }
    }

    RendererEndFrame(g_renderer);

    // ========== 2D GUI / HUD (all on shaders) - overlay, ignore depth ==========
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    GUIBegin(g_winW, g_winH, g_lastMX, g_lastMY, g_mouseDown);

    // Left control panel - anchored near bottom for stable clean look, reasonable fixed height
    float px = 14;
    float panelY = 8.0f;     // bottom edge (in y-up coords)
    float panelH = 368.0f;
    Panel(px, panelY, 320, panelH, "CONTROLS", 0.94f);

    // Inner content laid out from top of this panel (y-up). Avoids winH magic and overlap.
    float top = panelY + panelH - 18.0f;

    // Shape selectors - more space, larger buttons so text fits and looks clean
    Label("SHAPE A", px + 10, top, Vec3(0.6f,0.85f,1.0f), 0.95f);
    int newA = ShapeGrid(px + 8, top - 26, g_scene.indexA, 56, 26);
    if (newA >= 0) {
        g_scene.GetA().type = (ShapeType)newA;
        SampleAndComputeIntersections(g_scene, 2000, g_renderer);
    }

    Label("SHAPE B", px + 10, top - 100, Vec3(1.0f,0.75f,0.5f), 0.95f);
    int newB = ShapeGrid(px + 8, top - 126, g_scene.indexB, 56, 26);
    if (newB >= 0) {
        g_scene.GetB().type = (ShapeType)newB;
        SampleAndComputeIntersections(g_scene, 2000, g_renderer);
    }

    // Sliders for A/B - consistent row spacing
    auto& A = g_scene.GetA();
    auto& B = g_scene.GetB();

    float sy = top - 166;
    Slider("A Scale", px+8, sy, 248, 18, A.scale.x, 10.0f, 280.0f, "%.0f"); A.scale.y = A.scale.z = A.scale.x;
    sy -= 23;
    Slider("B Scale", px+8, sy, 248, 18, B.scale.x, 10.0f, 280.0f, "%.0f"); B.scale.y = B.scale.z = B.scale.x;
    sy -= 23;
    Slider("Rot Speed", px+8, sy, 248, 18, g_renderer.rotSpeed, 0.0f, 3.5f, "%.2f");
    sy -= 23;
    Slider("Cam Dist", px+8, sy, 248, 18, g_renderer.cam.distance, 20.0f, 4000.0f, "%.0f");

    // Viz toggles row - taller buttons (h=22) for larger readable text, high contrast
    sy -= 28;
    if (Button(g_renderer.showPoints ? "POINTS ON" : "POINTS OFF", px+8, sy, 130, 22)) g_renderer.showPoints = !g_renderer.showPoints;
    if (Button(g_renderer.showWire ? "WIRE ON" : "WIRE OFF", px+142, sy, 110, 22)) g_renderer.showWire = !g_renderer.showWire;
    if (Button(g_renderer.highlightIntersect ? "HILITE ON" : "HILITE", px+256, sy, 130, 22)) g_renderer.highlightIntersect = !g_renderer.highlightIntersect;

    sy -= 25;
    if (Button(g_renderer.rotateObjects ? "ROTATE ON" : "ROTATE", px+8, sy, 130, 22)) g_renderer.rotateObjects = !g_renderer.rotateObjects;
    if (Button(g_paused ? "PLAY" : "PAUSE", px+142, sy, 80, 22)) g_paused = !g_paused;
    if (Button("RESET CAM", px+226, sy, 130, 22)) {
        g_renderer.cam = OrbitCamera(); g_renderer.cam.distance = 520; g_renderer.cam.target = Vec3(0,0,-80);
    }

    // Presets
    sy -= 28;
    int np = PresetBar(px + 8, sy, g_currentPreset);
    if (np != g_currentPreset) {
        g_currentPreset = np;
        g_scene.ResetToPreset(g_currentPreset);
        SampleAndComputeIntersections(g_scene, 2200, g_renderer);
    }

    // Right info panel - also anchored near bottom for consistent clean layout
    float ix = (float)g_winW - 245;
    Panel(ix, 8.0f, 228, 122, "INTERSECTION", 0.9f);

    char buf[128];
    int nA = (int)g_renderer.intersectPtsA.size();
    int nB = (int)g_renderer.intersectPtsB.size();
    // relative to the bottom-anchored right panel (y=8, h=122)
    float rtop = 8.0f + 122.0f - 16.0f;
    sprintf_s(buf, "A in B: %d pts", nA);
    Label(buf, ix + 10, rtop, Vec3(1.0f,0.3f,1.0f), 0.92f);
    sprintf_s(buf, "B in A: %d pts", nB);
    Label(buf, ix + 10, rtop - 15, Vec3(1.0f,1.0f,0.2f), 0.92f);

    float overlap = (nA + nB) > 0 ? (float)(nA + nB) / 48.0f : 0.0f;
    if (overlap > 1) overlap = 1;
    sprintf_s(buf, "Overlap est: %.0f%%", overlap * 100.0f);
    Label(buf, ix + 10, rtop - 32, Vec3(0.6f,0.95f,0.7f), 0.9f);

    Label("Drag: LMB=orbit  Ctrl=moveA  Shift=moveB  Alt=rotBoth", ix+8, rtop - 50, Vec3(0.85f,0.85f,0.9f), 0.82f);
    Label("Wheel=zoom  Space=pause  T=rot  P=pts  W=wire  F5=reload shaders", ix+8, rtop - 64, Vec3(0.85f,0.85f,0.9f), 0.82f);

    // Top HUD - brighter for contrast on dark bg
    Label("Shape Intersection Lab - All rendering on GLSL shaders | Extensible shape system", 14, (float)g_winH - 28, Vec3(0.85f,0.85f,0.9f), 0.82f);

    // FPS rough
    static float fpsAcc = 0; static int fpsCnt=0; static float fpsShow=60;
    fpsAcc += 1.0f / 0.016f; fpsCnt++;
    if (fpsCnt > 20) { fpsShow = fpsAcc / fpsCnt; fpsAcc=0; fpsCnt=0; }
    sprintf_s(buf, "FPS: %.0f", fpsShow);
    Label(buf, (float)g_winW - 70, (float)g_winH - 28, Vec3(0.6f,0.85f,0.6f), 0.85f);

    GUIEnd();

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}
