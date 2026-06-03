#pragma once
// app.h - Application layer: owns document, renderer, input, view modes

#include <windows.h>
#include "document.h"
#include "renderer.h"
#include "gui.h"

enum class AppViewMode {
    View3D = 0,
    Drawing,
    Split
};

class App {
public:
    HWND hwnd = nullptr;
    HDC hdc = nullptr;
    HGLRC hrc = nullptr; // owned by main bootstrap
    int winW = 1280;
    int winH = 720;

    Renderer renderer;
    CadDocument document;

    AppViewMode viewMode = AppViewMode::Split;
    SeparationAxis sepAxis = SeparationAxis::Z;

    bool paused = false;
    bool showCurve = true;
    bool showProjections = true;
    float animTime = 0.0f;
    float sepStep = 8.0f;

    bool mouseDown = false;
    int lastMX = 0;
    int lastMY = 0;
    int dragMode = 0; // 0=none 1=orbit 2=moveA 3=moveB 4=rotateBoth

    bool Init(HWND hwndIn, HDC hdcIn);
    void Shutdown();
    void Update(float dt);
    void Render();
    void OnResize(int w, int h);
    void OnMouseMove(int mx, int my);
    void OnMouseDown(int mx, int my);
    void OnMouseUp();
    void OnWheel(float delta);
    void OnKeyDown(WPARAM key);

    static App* Get();
    static void Set(App* app);

private:
    void Render3DView(float x, float y, float w, float h);
    void RenderGUI();
    void RenderHUD();
    Vec3 ScreenToWorldDelta(int dx, int dy, float sens) const;
    void ApplySeparation(float amount);
};

extern App* g_app;