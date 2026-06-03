#pragma once
// shapes.h - Extensible shape system with mesh + SDF (CPU + shader-ready data)
// All shapes normalized to fit ~unit sphere/bounds where possible.

#include "math3d.h"
#include <vector>
#include <string>

enum ShapeType : int {
    SHAPE_SPHERE = 0,
    SHAPE_CONE,
    SHAPE_BOX,
    SHAPE_CYLINDER,
    SHAPE_TETRAHEDRON,
    SHAPE_OCTAHEDRON,
    SHAPE_DODECAHEDRON,
    SHAPE_ICOSAHEDRON,
    SHAPE_TORUS,
    SHAPE_CAPSULE,
    SHAPE_PYRAMID,
    SHAPE_COUNT
};

struct ShapeInfo {
    const char* name;
    const char* shortName;
    bool procedural; // if true, can have runtime params (not used yet)
};

extern const ShapeInfo g_shapeInfos[SHAPE_COUNT];

// Mesh data (unit)
struct ShapeMeshData {
    std::vector<float> vertices; // pos(3) + normal(3) + uv(2) = 8 floats per vert
    std::vector<unsigned int> indices;
};

// Precomputed for polyhedra: outward planes in local space (n·p + d <= 0 inside)
struct Plane4 { float nx, ny, nz, d; };

struct ShapeGPUData {
    int type;               // ShapeType
    int numPlanes;          // 0 for analytic
    Plane4 planes[40];      // max for dodeca ~36
    float extra[4];         // e.g. cone baseR/height ratio, torus R/r
};

// CPU: is point (already in local unit space) inside the shape?
bool IsPointInsideLocal(ShapeType type, Vec3 localP, const ShapeGPUData& gpuData);

// Generate unit mesh (approx scale 1.0 fits in unit ball for most)
void GenerateUnitMesh(ShapeType type, ShapeMeshData& outMesh, ShapeGPUData& outGPU);

// Helpers for specific generators
void GenSphere(int slices, int stacks, ShapeMeshData& out);
void GenCone(float baseR, float height, int slices, ShapeMeshData& out, ShapeGPUData& gpu);
void GenBox(Vec3 halfExt, ShapeMeshData& out, ShapeGPUData& gpu);
void GenCylinder(float radius, float height, int slices, ShapeMeshData& out, ShapeGPUData& gpu);
void GenTorus(float majorR, float minorR, int rings, int sides, ShapeMeshData& out, ShapeGPUData& gpu);

// Polyhedra (reuse/extend original data)
void GenTetra(ShapeMeshData& out, ShapeGPUData& gpu);
void GenOcta(ShapeMeshData& out, ShapeGPUData& gpu);
void GenDodeca(ShapeMeshData& out, ShapeGPUData& gpu);
void GenIcosa(ShapeMeshData& out, ShapeGPUData& gpu);

// Instance in world
struct ShapeInstance {
    ShapeType type = SHAPE_SPHERE;
    Vec3 pos = Vec3(0, 0, 0);
    Vec3 euler = Vec3(0, 0, 0);   // degrees
    Vec3 scale = Vec3(100, 100, 100);
    Vec3 color = Vec3(0.2f, 0.6f, 0.9f);
    float alpha = 1.0f;
    bool visible = true;
    bool isSecond = false; // for classic two-body viz

    Mat4 GetModel() const { return ModelMatrix(pos, euler, scale); }
    void GetLocalPlanesOrData(ShapeGPUData& dataOut) const; // fills for shader
};

// Scene manager (simple for two focused + extras)
struct ShapeScene {
    std::vector<ShapeInstance> shapes;
    int indexA = 0;
    int indexB = 1;

    ShapeInstance& GetA() { return shapes[indexA]; }
    ShapeInstance& GetB() { return shapes[indexB]; }
    const ShapeInstance& GetA() const { return shapes[indexA]; }
    const ShapeInstance& GetB() const { return shapes[indexB]; }

    void InitDefault();
    void AddShape(ShapeType t, Vec3 p, float s, Vec3 col);
    void SetPair(int a, int b);
    void ResetToPreset(int presetId);
};
