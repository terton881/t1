#include "document.h"

void CadDocument::MarkDirty() {
    dirtyIntersection = true;
    dirtyProjection = true;
}

void CadDocument::InitDefault() {
    contextShapes.clear();
    objectA = ShapeInstance();
    objectB = ShapeInstance();
    objectA.type = SHAPE_SPHERE;
    objectA.pos = Vec3(0, 0, 0);
    objectA.scale = Vec3(95, 95, 95);
    objectA.color = Vec3(0.15f, 0.55f, 0.85f);
    objectA.euler = Vec3(12, 35, 0);

    objectB.type = SHAPE_CONE;
    objectB.pos = Vec3(0, 0, -220);
    objectB.scale = Vec3(82, 82, 82);
    objectB.color = Vec3(0.9f, 0.45f, 0.1f);
    objectB.euler = Vec3(-8, 140, 0);
    objectB.isSecond = true;

    currentPreset = 0;
    MarkDirty();
    RecomputeAll();
}

void CadDocument::ResetPreset(int presetId) {
    currentPreset = presetId % 6;
    contextShapes.clear();
    objectA = ShapeInstance();
    objectB = ShapeInstance();
    objectA.isSecond = false;
    objectB.isSecond = true;

    switch (currentPreset) {
    case 0:
        objectA.type = SHAPE_SPHERE; objectA.pos = Vec3(0, 0, 0); objectA.scale = Vec3(95, 95, 95);
        objectA.color = Vec3(0.2f, 0.6f, 0.95f);
        objectB.type = SHAPE_CONE; objectB.pos = Vec3(0, 0, -210); objectB.scale = Vec3(78, 78, 78);
        objectB.color = Vec3(0.95f, 0.5f, 0.15f);
        break;
    case 1:
        objectA.type = SHAPE_BOX; objectA.pos = Vec3(-60, 0, 0); objectA.scale = Vec3(85, 85, 85);
        objectA.color = Vec3(0.2f, 0.75f, 0.35f);
        objectB.type = SHAPE_ICOSAHEDRON; objectB.pos = Vec3(80, 10, -30); objectB.scale = Vec3(90, 90, 90);
        objectB.color = Vec3(0.85f, 0.25f, 0.55f);
        break;
    case 2:
        objectA.type = SHAPE_DODECAHEDRON; objectA.pos = Vec3(0, 0, 0); objectA.scale = Vec3(80, 80, 80);
        objectA.color = Vec3(0.9f, 0.75f, 0.15f);
        objectB.type = SHAPE_TETRAHEDRON; objectB.pos = Vec3(30, 40, -180); objectB.scale = Vec3(75, 75, 75);
        objectB.color = Vec3(0.1f, 0.85f, 0.9f);
        break;
    case 3:
        objectA.type = SHAPE_CYLINDER; objectA.pos = Vec3(-40, 0, 50); objectA.scale = Vec3(70, 70, 70);
        objectA.color = Vec3(0.3f, 0.8f, 0.6f);
        objectB.type = SHAPE_TORUS; objectB.pos = Vec3(110, -20, -60); objectB.scale = Vec3(95, 95, 95);
        objectB.color = Vec3(0.75f, 0.2f, 0.65f);
        break;
    case 4:
        objectA.type = SHAPE_OCTAHEDRON; objectA.pos = Vec3(0, 0, 0); objectA.scale = Vec3(88, 88, 88);
        objectA.color = Vec3(0.7f, 0.15f, 0.75f);
        objectB.type = SHAPE_CONE; objectB.pos = Vec3(20, 0, -190); objectB.scale = Vec3(72, 72, 72);
        objectB.color = Vec3(0.95f, 0.55f, 0.1f);
        break;
    default: {
        ShapeInstance s0; s0.type = SHAPE_SPHERE; s0.pos = Vec3(-120, 30, 40); s0.scale = Vec3(70, 70, 70);
        s0.color = Vec3(0.4f, 0.7f, 0.95f); s0.alpha = 0.35f;
        ShapeInstance s2; s2.type = SHAPE_TORUS; s2.pos = Vec3(10, 80, -220); s2.scale = Vec3(105, 105, 105);
        s2.color = Vec3(0.55f, 0.85f, 0.3f); s2.alpha = 0.35f;
        contextShapes.push_back(s0);
        contextShapes.push_back(s2);
        objectA.type = SHAPE_BOX; objectA.pos = Vec3(90, -10, -50); objectA.scale = Vec3(80, 80, 80);
        objectA.color = Vec3(0.9f, 0.4f, 0.25f);
        objectB.type = SHAPE_CONE; objectB.pos = Vec3(0, 0, -200); objectB.scale = Vec3(75, 75, 75);
        objectB.color = Vec3(0.95f, 0.5f, 0.15f);
        break;
    }
    }
    objectA.euler = Vec3(10, 30, 0);
    objectB.euler = Vec3(-5, 120, 0);
    MarkDirty();
    RecomputeAll();
}

void CadDocument::SetShapeType(bool isB, ShapeType t) {
    if (isB) objectB.type = t; else objectA.type = t;
    MarkDirty();
}

void CadDocument::MoveShape(bool isB, Vec3 delta) {
    if (isB) objectB.pos = objectB.pos + delta;
    else objectA.pos = objectA.pos + delta;
    MarkDirty();
}

void CadDocument::RotateShape(bool isB, Vec3 deltaEuler) {
    if (isB) objectB.euler = objectB.euler + deltaEuler;
    else objectA.euler = objectA.euler + deltaEuler;
    MarkDirty();
}

void CadDocument::SetUniformScale(bool isB, float s) {
    if (isB) objectB.scale = Vec3(s, s, s);
    else objectA.scale = Vec3(s, s, s);
    MarkDirty();
}

void CadDocument::RecomputeIntersection() {
    if (!dirtyIntersection) return;
    IntersectionSolver solver;
    int samples = IntersectionSampleCount(quality);
    intersection = solver.Compute(objectA, objectB, samples);
    dirtyIntersection = false;
    dirtyProjection = true;
}

void CadDocument::RecomputeProjection() {
    if (!dirtyProjection) return;
    if (dirtyIntersection)
        RecomputeIntersection();
    sheet = BuildProjectionSheet(intersection, objectA, objectB);
    dirtyProjection = false;
}

void CadDocument::RecomputeAll() {
    dirtyIntersection = true;
    dirtyProjection = true;
    RecomputeProjection();
}

float CadDocument::CenterDistance() const {
    return Length(objectB.pos - objectA.pos);
}