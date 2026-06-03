#pragma once
// document.h - Engineering document model (two solids + intersection + drafting sheet)

#include "shapes.h"
#include "intersection.h"
#include "projection.h"
#include <vector>

enum class SeparationAxis {
    X = 0,
    Y,
    Z
};

struct CadDocument {
    ShapeInstance objectA;
    ShapeInstance objectB;
    std::vector<ShapeInstance> contextShapes;

    IntersectionResult intersection;
    ProjectionSheet sheet;

    IntersectionQuality quality = IntersectionQuality::Medium;
    bool dirtyIntersection = true;
    bool dirtyProjection = true;

    int currentPreset = 0;

    void InitDefault();
    void ResetPreset(int presetId);

    ShapeInstance& GetA() { return objectA; }
    ShapeInstance& GetB() { return objectB; }
    const ShapeInstance& GetA() const { return objectA; }
    const ShapeInstance& GetB() const { return objectB; }

    void MarkDirty();
    void SetShapeType(bool isB, ShapeType t);
    void MoveShape(bool isB, Vec3 delta);
    void RotateShape(bool isB, Vec3 deltaEuler);
    void SetUniformScale(bool isB, float s);

    void RecomputeIntersection();
    void RecomputeProjection();
    void RecomputeAll();

    float CenterDistance() const;
};