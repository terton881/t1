#pragma once
// gui.h - Fully shader-powered immediate-mode GUI (no Dear ImGui)
// Panels, buttons, sliders, radio/combo, labels, all drawn with GL + text shader.

#include "renderer.h"
#include <functional>

struct GUIState {
    int winW = 1280, winH = 720;
    int mouseX = 0, mouseY = 0;
    bool mouseDown = false;
    bool mouseWasDown = false;

    int hotItem = 0;      // widget under mouse
    int activeItem = 0;   // widget being dragged

    float dt = 1.0f / 60.0f;
};

extern GUIState g_gui;

// Call once per frame before widgets
void GUIBegin(int winW, int winH, int mx, int my, bool lmbDown);

// Call after all widgets to finish (reset states)
void GUIEnd();

// Low level
void GUISetOrtho(float w, float h); // sets uOrtho for UI program
bool GUIRectHit(float x, float y, float w, float h) ; // current mouse

// Widgets
bool Button(const char* label, float x, float y, float w, float h);
bool Slider(const char* label, float x, float y, float w, float h, float& value, float vmin, float vmax, const char* fmt = "%.1f");
bool Checkbox(const char* label, float x, float y, bool& checked);
int  Combo(const char* label, float x, float y, float w, const char* const* items, int count, int& current);
void Label(const char* text, float x, float y, Vec3 col = Vec3(0.9f,0.9f,0.92f), float sc=1.0f);
void Panel(float x, float y, float w, float h, const char* title = nullptr, float alpha=0.92f);

// Shape picker as nice grid of buttons
int ShapeGrid(float x, float y, int current, float btnW = 78, float btnH = 26);

// Preset bar
int PresetBar(float x, float y, int currentPreset);

// Call this after GL init to build font atlas from system font
bool CreateFontAtlas(HDC hdcForRef);
void DestroyFontAtlas();

// Font state shared for text rendering (used by renderer DrawText2D too)
extern GLuint g_fontTex;
extern int g_fontTexW, g_fontTexH;
extern float g_glyphU[128], g_glyphV[128], g_glyphW[128];

// Allow other modules (renderer text draw) to apply the current ortho to a program (e.g. text prog)
void GUISetOrthoForProgram(GLuint prog);
