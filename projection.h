#pragma once
// projection.h - 2D drafting views from 3D model + intersection curve

#include "math3d.h"
#include "shapes.h"
#include "intersection.h"
#include <vector>

enum class DraftViewType {
    Front = 0,
    Top,
    Right,
    Iso
};

struct ProjectedCurve2D {
    std::vector<Vec2> points;
    DraftViewType view = DraftViewType::Front;
};

struct ProjectionSheet {
    ProjectedCurve2D curveFront;
    ProjectedCurve2D curveTop;
    ProjectedCurve2D curveRight;
    ProjectedCurve2D outlineAFront, outlineBFront;
    ProjectedCurve2D outlineATop, outlineBTop;
    ProjectedCurve2D outlineARight, outlineBRight;
    Vec2 origin = Vec2(0, 0);
    float scale = 1.0f;
    Vec2 boundsMin = Vec2(0, 0);
    Vec2 boundsMax = Vec2(1, 1);
};

Vec2 ProjectWorldPoint(const Vec3& p, DraftViewType view);
void ProjectMeshOutline(const ShapeInstance& shape, DraftViewType view, std::vector<Vec2>& outPts);

ProjectionSheet BuildProjectionSheet(const IntersectionResult& intersection,
    const ShapeInstance& A, const ShapeInstance& B);