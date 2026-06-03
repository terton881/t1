#pragma once
// renderer.h - Modern shader-based renderer for shapes + intersection viz + 2D GUI elements

#include "gl_core.h"
#include "math3d.h"
#include "shapes.h"
#include <string>

struct CameraUniforms {
    Mat4 viewProj;
    Vec3 eye;
    float _pad;
};

struct ShapeUniforms {
    Mat4 model;
    Vec3 color;
    float alpha;
    int  shapeType;
    int  numPlanes;
    float extra[4];
    Vec3 lightDir;
    float _pad2;
    // For intersection viz with the "other" shape
    int  otherType;
    int  otherNumPlanes;
    float otherExtra[4];
    float _pad3[2];
    // We pass other planes via a uniform array separately
};

// Global renderer state
struct Renderer {
    // Programs
    GLuint progShape = 0;   // main phong + inside-test viz
    GLuint progPoints = 0;  // for intersection markers
    GLuint progUI = 0;      // 2D colored rects/lines
    GLuint progText = 0;    // textured text

    // Camera
    OrbitCamera cam;

    // Lights / params
    Vec3 lightDir = Vec3(0.4f, 0.8f, 0.3f);
    bool showWire = false;
    bool showPoints = true;
    bool highlightIntersect = true;
    bool showAxes = true;
    bool rotateObjects = true;
    float rotSpeed = 0.6f;

    // Intersection sampling (CPU for now, can move to compute later)
    std::vector<Vec3> intersectPtsA; // points of A inside B
    std::vector<Vec3> intersectPtsB;
};

// Init / shutdown
bool RendererInit(Renderer& r);
void RendererShutdown(Renderer& r);

// Per frame
void RendererBeginFrame(Renderer& r, int winW, int winH);
void RendererEndFrame(Renderer& r);

// Draw 3D
void DrawShape(const Renderer& r, const ShapeInstance& inst, const ShapeInstance* otherForTest = nullptr);
void DrawAxes(const Renderer& r);
void DrawIntersectionPoints(const Renderer& r, const std::vector<Vec3>& pts, Vec3 color, float size = 2.8f);

// 2D overlay (call after 3D, with ortho set inside)
void DrawRect2D(float x, float y, float w, float h, Vec3 color, float alpha = 1.0f);
void DrawLine2D(float x0, float y0, float x1, float y1, Vec3 color, float alpha = 1.0f, float thick = 1.0f);

// Text (requires font texture prepared by GUI system)
void DrawText2D(float x, float y, const char* text, Vec3 color, float scale = 1.0f);

// Hot reload
bool ReloadShaders(Renderer& r);

// Helpers to sample intersection (CPU side, matches shader logic)
void SampleAndComputeIntersections(ShapeScene& scene, int samplesPerShape, Renderer& r);

// Shared with gui for 2D drawing
extern Renderer* g_rendererForUI;
void SetUIRenderer(Renderer* r);
