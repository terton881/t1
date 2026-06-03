#include "app.h"
#include "gl_core.h"
#include <stdio.h>

App* g_app = nullptr;

void App::Set(App* app) { g_app = app; }
App* App::Get() { return g_app; }

bool App::Init(HWND hwndIn, HDC hdcIn) {
    hwnd = hwndIn;
    hdc = hdcIn;
    winW = 1280;
    winH = 720;

    if (!RendererInit(renderer)) return false;
    SetUIRenderer(&renderer);
    renderer.showCurve = showCurve;

    document.InitDefault();
    return true;
}

void App::Shutdown() {
    RendererShutdown(renderer);
}

Vec3 App::ScreenToWorldDelta(int dx, int dy, float sens) const {
    float move = renderer.cam.distance * sens * 0.0022f;
    Vec3 right = Vec3(cosf(M3D_DEG2RAD(renderer.cam.yaw)), 0, sinf(M3D_DEG2RAD(renderer.cam.yaw)));
    Vec3 up = Vec3(0, 1, 0);
    return right * (float(-dx) * move) + up * (float(dy) * move);
}

void App::ApplySeparation(float amount) {
    Vec3 axis(0, 0, 0);
    switch (sepAxis) {
    case SeparationAxis::X: axis = Vec3(1, 0, 0); break;
    case SeparationAxis::Y: axis = Vec3(0, 1, 0); break;
    case SeparationAxis::Z: axis = Vec3(0, 0, 1); break;
    }
    document.objectB.pos = document.objectB.pos + axis * amount;
    document.MarkDirty();
}

void App::Update(float dt) {
    if (document.dirtyIntersection)
        document.RecomputeIntersection();

    if (!paused) {
        animTime += dt * 60.0f;
        if (renderer.rotateObjects) {
            float rs = renderer.rotSpeed;
            document.RotateShape(false, Vec3(0, rs * 0.7f * dt * 60.0f, 0));
            document.RotateShape(true, Vec3(0, rs * 1.15f * dt * 60.0f, 0));
            document.objectA.euler.x = sinf(animTime * 0.002f) * 6.0f;
        }
        float phase = animTime * 0.018f;
        document.objectB.pos.z = -210.0f + sinf(phase) * 95.0f;
        document.objectB.pos.x = sinf(phase * 0.6f) * 35.0f;
        document.MarkDirty();
    }

    if (document.dirtyProjection)
        document.RecomputeProjection();
}

void App::Render3DView(float vx, float vy, float vw, float vh) {
    (void)vx; (void)vy; (void)vw; (void)vh;
    auto& doc = document;

    for (auto& s : doc.contextShapes) {
        ShapeInstance tmp = s;
        DrawShape(renderer, tmp, nullptr);
    }

    DrawShape(renderer, doc.objectA, &doc.objectB);
    DrawShape(renderer, doc.objectB, &doc.objectA);
    DrawAxes(renderer);

    auto& ir = doc.intersection;
    DrawIntersectionPoints(renderer, ir.pointsAInsideB, Vec3(1.0f, 0.2f, 1.0f), 3.2f);
    DrawIntersectionPoints(renderer, ir.pointsBInsideA, Vec3(1.0f, 1.0f, 0.15f), 3.2f);
    DrawIntersectionCurve3D(renderer, ir.sortedCurvePoints);
}

void App::RenderHUD() {
    char buf[160];
    auto& ir = document.intersection;
    float ix = (float)winW - 260;
    Panel(ix, 8.0f, 248, 140, "INTERSECTION", 0.9f);
    float rtop = 8.0f + 140.0f - 16.0f;
    sprintf_s(buf, "A in B: %d pts", (int)ir.pointsAInsideB.size());
    Label(buf, ix + 10, rtop, Vec3(1.0f, 0.3f, 1.0f), 0.92f);
    sprintf_s(buf, "B in A: %d pts", (int)ir.pointsBInsideA.size());
    Label(buf, ix + 10, rtop - 15, Vec3(1.0f, 1.0f, 0.2f), 0.92f);
    sprintf_s(buf, "Overlap: %.0f%%", ir.overlapEstimate * 100.0f);
    Label(buf, ix + 10, rtop - 32, Vec3(0.6f, 0.95f, 0.7f), 0.9f);
    sprintf_s(buf, "Center dist: %.1f", document.CenterDistance());
    Label(buf, ix + 10, rtop - 48, Vec3(0.85f, 0.85f, 0.95f), 0.88f);
    const char* ax = sepAxis == SeparationAxis::X ? "X" : (sepAxis == SeparationAxis::Y ? "Y" : "Z");
    sprintf_s(buf, "Sep axis: %s  PgUp/Dn +/-", ax);
    Label(buf, ix + 10, rtop - 64, Vec3(0.85f, 0.85f, 0.9f), 0.82f);
    Label("LMB=orbit Ctrl=A Shift=B Alt=rot  X/Y/Z axis", ix + 8, rtop - 78, Vec3(0.85f, 0.85f, 0.9f), 0.78f);

    Label("Intersection Drafting Lab - 3D + drafting projections", 14, (float)winH - 28, Vec3(0.85f, 0.85f, 0.9f), 0.82f);
}

void App::RenderGUI() {
    float px = 14;
    float panelY = 8.0f;
    float panelH = 400.0f;
    Panel(px, panelY, 330, panelH, "CONTROLS", 0.94f);
    float top = panelY + panelH - 18.0f;

    const char* modes[] = { "3D", "DRAW", "SPLIT" };
    static int modeIdx = 2;
    if (Combo("Mode", px + 8, top, 100, modes, 3, modeIdx)) {
        viewMode = (AppViewMode)modeIdx;
    }

    top -= 28;
    Label("SHAPE A", px + 10, top, Vec3(0.6f, 0.85f, 1.0f), 0.95f);
    int typeA = (int)document.objectA.type;
    int newA = ShapeGrid(px + 8, top - 26, typeA, 56, 26);
    if (newA >= 0) document.SetShapeType(false, (ShapeType)newA);

    Label("SHAPE B", px + 10, top - 100, Vec3(1.0f, 0.75f, 0.5f), 0.95f);
    int typeB = (int)document.objectB.type;
    int newB = ShapeGrid(px + 8, top - 126, typeB, 56, 26);
    if (newB >= 0) document.SetShapeType(true, (ShapeType)newB);

    auto& A = document.objectA;
    auto& B = document.objectB;
    float sy = top - 166;
    if (Slider("A Scale", px + 8, sy, 260, 18, A.scale.x, 10.0f, 280.0f, "%.0f"))
        document.SetUniformScale(false, A.scale.x);
    sy -= 23;
    if (Slider("B Scale", px + 8, sy, 260, 18, B.scale.x, 10.0f, 280.0f, "%.0f"))
        document.SetUniformScale(true, B.scale.x);
    sy -= 23;
    Slider("Rot Speed", px + 8, sy, 260, 18, renderer.rotSpeed, 0.0f, 3.5f, "%.2f");
    sy -= 23;
    Slider("Cam Dist", px + 8, sy, 260, 18, renderer.cam.distance, 20.0f, 4000.0f, "%.0f");

    sy -= 28;
    static int qualIdx = 1;
    const char* quals[] = { "Low", "Med", "High" };
    if (Combo("Quality", px + 8, sy, 120, quals, 3, qualIdx)) {
        document.quality = (IntersectionQuality)qualIdx;
        document.MarkDirty();
    }

    sy -= 26;
    if (Button(renderer.showPoints ? "POINTS ON" : "POINTS OFF", px + 8, sy, 100, 22))
        renderer.showPoints = !renderer.showPoints;
    if (Button(renderer.showCurve ? "CURVE ON" : "CURVE OFF", px + 112, sy, 100, 22))
        renderer.showCurve = showCurve = !renderer.showCurve;
    if (Button(showProjections ? "PROJ ON" : "PROJ OFF", px + 216, sy, 100, 22))
        showProjections = !showProjections;

    sy -= 25;
    if (Button(renderer.rotateObjects ? "ROTATE ON" : "ROTATE", px + 8, sy, 100, 22))
        renderer.rotateObjects = !renderer.rotateObjects;
    if (Button(paused ? "PLAY" : "PAUSE", px + 112, sy, 80, 22)) paused = !paused;
    if (Button("RESET CAM", px + 196, sy, 120, 22)) {
        renderer.cam = OrbitCamera();
        renderer.cam.distance = 520;
        renderer.cam.target = Vec3(0, 0, -80);
    }

    sy -= 28;
    int np = PresetBar(px + 8, sy, document.currentPreset);
    if (np != document.currentPreset) document.ResetPreset(np);
}

void App::Render() {
    RendererBeginFrame(renderer, winW, winH);

    float sheetW = 0, sheetH = 0, sheetX = 0, sheetY = 0;
    float view3dW = (float)winW;
    float view3dH = (float)winH;

    if (viewMode == AppViewMode::Split) {
        view3dW = winW * 0.58f;
        sheetW = winW - view3dW - 8;
        sheetH = winH * 0.55f;
        sheetX = view3dW + 4;
        sheetY = 8;
    } else if (viewMode == AppViewMode::Drawing) {
        view3dW = 0;
        sheetW = (float)winW - 16;
        sheetH = (float)winH - 40;
        sheetX = 8;
        sheetY = 8;
    }

    if (view3dW > 64) Render3DView(0, 0, view3dW, view3dH);
    RendererEndFrame(renderer);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    GUIBegin(winW, winH, lastMX, lastMY, mouseDown);

    if (showProjections && sheetW > 80 && sheetH > 80)
        DrawProjectionSheet2D(document.sheet, Rect2D(sheetX, sheetY, sheetW, sheetH), true);

    RenderGUI();
    RenderHUD();

    GUIEnd();
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

void App::OnResize(int w, int h) {
    winW = w;
    winH = h;
    renderer.cam.aspect = w > 0 ? float(w) / float(h) : 1.6f;
}

void App::OnMouseMove(int mx, int my) {
    if (mouseDown) {
        int dx = mx - lastMX;
        int dy = my - lastMY;
        float sens = 0.6f;
        if (GetKeyState(VK_SHIFT) & 0x8000) sens *= 0.1f;

        if (dragMode == 1) {
            renderer.cam.Orbit(dx * 0.35f, -dy * 0.35f);
        } else if (dragMode == 2) {
            document.MoveShape(false, ScreenToWorldDelta(dx, dy, sens));
        } else if (dragMode == 3) {
            document.MoveShape(true, ScreenToWorldDelta(dx, dy, sens));
        } else if (dragMode == 4) {
            float rs = (GetKeyState(VK_SHIFT) & 0x8000) ? 0.06f : 0.6f;
            document.RotateShape(false, Vec3(dy * rs * 0.8f, dx * rs, 0));
            document.RotateShape(true, Vec3(dy * rs * 0.8f, dx * rs, 0));
        }
    }
    lastMX = mx;
    lastMY = my;
}

void App::OnMouseDown(int mx, int my) {
    mouseDown = true;
    lastMX = mx;
    lastMY = my;
    if (GetKeyState(VK_CONTROL) & 0x8000) dragMode = 2;
    else if (GetKeyState(VK_SHIFT) & 0x8000) dragMode = 3;
    else if (GetKeyState(VK_MENU) & 0x8000) dragMode = 4;
    else dragMode = 1;
}

void App::OnMouseUp() {
    mouseDown = false;
    dragMode = 0;
}

void App::OnWheel(float delta) {
    renderer.cam.Zoom(-delta * 0.8f);
}

void App::OnKeyDown(WPARAM key) {
    if (key == VK_SPACE) { paused = !paused; return; }
    if (key == 'R') {
        renderer.cam = OrbitCamera();
        renderer.cam.distance = 520;
        renderer.cam.target = Vec3(0, 0, -80);
        return;
    }
    if (key == 'T') { renderer.rotateObjects = !renderer.rotateObjects; return; }
    if (key == 'P') { renderer.showPoints = !renderer.showPoints; return; }
    if (key == 'W') { renderer.showWire = !renderer.showWire; return; }
    if (key == 'H') { renderer.highlightIntersect = !renderer.highlightIntersect; return; }
    if (key == 'A') { renderer.showAxes = !renderer.showAxes; return; }
    if (key == 'C') { renderer.showCurve = showCurve = !renderer.showCurve; return; }
    if (key == 'V') { showProjections = !showProjections; return; }
    if (key == VK_F5) { ReloadShaders(renderer); return; }
    if (key == VK_ESCAPE) { PostQuitMessage(0); return; }

    if (key == 'X') { sepAxis = SeparationAxis::X; return; }
    if (key == 'Y') { sepAxis = SeparationAxis::Y; return; }
    if (key == 'Z') { sepAxis = SeparationAxis::Z; return; }
    if (key == VK_PRIOR) { ApplySeparation(sepStep); return; }
    if (key == VK_NEXT) { ApplySeparation(-sepStep); return; }
    if (key == VK_ADD || key == 0xBB) { ApplySeparation(sepStep); return; }
    if (key == VK_SUBTRACT || key == 0xBD) { ApplySeparation(-sepStep); return; }

    if (key >= '1' && key <= '5') {
        ShapeType t = (ShapeType)((key - '1') % SHAPE_COUNT);
        document.SetShapeType(true, t);
    }
}