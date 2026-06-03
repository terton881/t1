// main.cpp - Win32 bootstrap only; forwards to App layer

#include <windows.h>
#include "gl_core.h"
#include "app.h"

static App g_appInstance;
static HGLRC g_hrc = nullptr;

static void SetupModernGL(HDC hdc) {
    PIXELFORMATDESCRIPTOR pfd = {0};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    int pf = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pf, &pfd);
    g_hrc = wglCreateContext(hdc);
    wglMakeCurrent(hdc, g_hrc);
    LoadGLProcs(hdc);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    App* app = App::Get();
    if (!app) return DefWindowProc(hwnd, msg, w, l);

    switch (msg) {
    case WM_SIZE: {
        int ww = LOWORD(l), hh = HIWORD(l);
        if (ww > 0 && hh > 0) {
            glViewport(0, 0, ww, hh);
            app->OnResize(ww, hh);
        }
        return 0;
    }
    case WM_MOUSEMOVE:
        app->OnMouseMove(LOWORD(l), HIWORD(l));
        return 0;
    case WM_LBUTTONDOWN:
        app->OnMouseDown(LOWORD(l), HIWORD(l));
        SetCapture(hwnd);
        return 0;
    case WM_LBUTTONUP:
        app->OnMouseUp();
        ReleaseCapture();
        return 0;
    case WM_MOUSEWHEEL:
        app->OnWheel(GET_WHEEL_DELTA_WPARAM(w) / 120.0f);
        return 0;
    case WM_KEYDOWN:
        app->OnKeyDown(w);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    default:
        break;
    }
    return DefWindowProc(hwnd, msg, w, l);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nShow) {
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"IntersectionDraftingLab";

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, L"Register failed", L"Error", MB_OK);
        return 0;
    }

    HWND hwnd = CreateWindowEx(0, L"IntersectionDraftingLab",
        L"Intersection Drafting Lab",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
        NULL, NULL, hInst, NULL);
    if (!hwnd) return 0;

    ShowWindow(hwnd, nShow);
    UpdateWindow(hwnd);

    HDC hdc = GetDC(hwnd);
    SetupModernGL(hdc);

    if (!LoadGLProcs(hdc)) {
        MessageBoxA(NULL, "OpenGL 3.3+ required", "Fatal", MB_ICONERROR);
        return 1;
    }

    HGLRC modern = CreateModernContext(hdc, 3, 3, false);
    if (modern) {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(g_hrc);
        g_hrc = modern;
        wglMakeCurrent(hdc, g_hrc);
    }
    SetVSync(true);

    if (!CreateFontAtlas(hdc)) {
        MessageBoxA(NULL, "Font atlas failed", "GUI", MB_ICONERROR);
        return 1;
    }

    g_appInstance.hwnd = hwnd;
    g_appInstance.hdc = hdc;
    g_appInstance.hrc = g_hrc;
    App::Set(&g_appInstance);

    if (!g_appInstance.Init(hwnd, hdc)) {
        MessageBoxA(NULL, "App init failed", "Error", MB_ICONERROR);
        return 1;
    }

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

        g_appInstance.Update(dt);
        g_appInstance.Render();
        SwapBuffers(hdc);
        Sleep(1);
    }

    g_appInstance.Shutdown();
    DestroyFontAtlas();
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(g_hrc);
    ReleaseDC(hwnd, hdc);
    return 0;
}