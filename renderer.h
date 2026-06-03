#pragma once
// renderer.h - Rendering only (no geometry computation)

#include "gl_core.h"
#include "math3d.h"
#include "shapes.h"
#include "projection.h"
#include <vector>

struct Renderer {
    GLuint progShape = 0;
    GLuint progPoints = 0;
    GLuint progUI = 0;
    GLuint progText = 0;

    OrbitCamera cam;
    Vec3 lightDir = Vec3(0.4f, 0.8f, 0.3f);
    bool showWire = false;
    bool showPoints = true;
    bool showCurve = true;
    bool highlightIntersect = true;
    bool showAxes = true;
    bool rotateObjects = true;
    float rotSpeed = 0.6f;
};

bool RendererInit(Renderer& r);
void RendererShutdown(Renderer& r);
void RendererBeginFrame(Renderer& r, int winW, int winH);
void RendererEndFrame(Renderer& r);

void DrawShape(const Renderer& r, const ShapeInstance& inst, const ShapeInstance* otherForTest = nullptr);
void DrawAxes(const Renderer& r);
void DrawIntersectionPoints(const Renderer& r, const std::vector<Vec3>& pts, Vec3 color, float size = 2.8f);
void DrawLineStrip3D(const Renderer& r, const std::vector<Vec3>& pts, Vec3 color, float alpha, float lineWidth);
void DrawIntersectionCurve3D(const Renderer& r, const std::vector<Vec3>& curve);

void DrawRect2D(float x, float y, float w, float h, Vec3 color, float alpha = 1.0f);
void DrawLine2D(float x0, float y0, float x1, float y1, Vec3 color, float alpha = 1.0f, float thick = 1.0f);
void DrawPolyline2D(const std::vector<Vec2>& pts, Vec3 color, float alpha, bool closed = false);
void DrawGrid2D(Rect2D area, int divisions, Vec3 color, float alpha);
void DrawProjectionSheet2D(const ProjectionSheet& sheet, Rect2D area, bool showOutlines);
void DrawText2D(float x, float y, const char* text, Vec3 color, float scale = 1.0f);

bool ReloadShaders(Renderer& r);

extern Renderer* g_rendererForUI;
void SetUIRenderer(Renderer* r);