#include "projection.h"
#include <algorithm>
#include <cmath>

Vec2 ProjectWorldPoint(const Vec3& p, DraftViewType view) {
    switch (view) {
    case DraftViewType::Front: return Vec2(p.x, p.y);
    case DraftViewType::Top:   return Vec2(p.x, p.z);
    case DraftViewType::Right: return Vec2(p.z, p.y);
    case DraftViewType::Iso:
    default:
        return Vec2(p.x * 0.866f - p.z * 0.5f, p.y + p.x * 0.25f + p.z * 0.25f);
    }
}

void ProjectMeshOutline(const ShapeInstance& shape, DraftViewType view, std::vector<Vec2>& outPts) {
    outPts.clear();
    ShapeMeshData md;
    ShapeGPUData gd;
    GenerateUnitMesh(shape.type, md, gd);
    Mat4 M = shape.GetModel();
    const int stride = 8;
    const int nVert = (int)md.vertices.size() / stride;
    outPts.reserve((size_t)nVert);
    for (int i = 0; i < nVert; ++i) {
        Vec3 lp(md.vertices[i * stride], md.vertices[i * stride + 1], md.vertices[i * stride + 2]);
        Vec3 wp = MulPoint(M, lp);
        outPts.push_back(ProjectWorldPoint(wp, view));
    }
}

static void ProjectCurve(const std::vector<Vec3>& curve3d, DraftViewType view, ProjectedCurve2D& out) {
    out.view = view;
    out.points.clear();
    out.points.reserve(curve3d.size());
    for (auto& p : curve3d)
        out.points.push_back(ProjectWorldPoint(p, view));
}

static void UpdateBounds(ProjectionSheet& sheet, const Vec2& p) {
    if (sheet.boundsMin.x > sheet.boundsMax.x) {
        sheet.boundsMin = p;
        sheet.boundsMax = p;
        return;
    }
    if (p.x < sheet.boundsMin.x) sheet.boundsMin.x = p.x;
    if (p.y < sheet.boundsMin.y) sheet.boundsMin.y = p.y;
    if (p.x > sheet.boundsMax.x) sheet.boundsMax.x = p.x;
    if (p.y > sheet.boundsMax.y) sheet.boundsMax.y = p.y;
}

ProjectionSheet BuildProjectionSheet(const IntersectionResult& intersection,
    const ShapeInstance& A, const ShapeInstance& B)
{
    ProjectionSheet sheet;
    const std::vector<Vec3>& curve = intersection.sortedCurvePoints.empty()
        ? intersection.rawCurvePoints : intersection.sortedCurvePoints;

    sheet.curveFront.view = DraftViewType::Front;
    sheet.curveTop.view = DraftViewType::Top;
    sheet.curveRight.view = DraftViewType::Right;
    ProjectCurve(curve, DraftViewType::Front, sheet.curveFront);
    ProjectCurve(curve, DraftViewType::Top, sheet.curveTop);
    ProjectCurve(curve, DraftViewType::Right, sheet.curveRight);

    ProjectMeshOutline(A, DraftViewType::Front, sheet.outlineAFront.points);
    ProjectMeshOutline(B, DraftViewType::Front, sheet.outlineBFront.points);
    ProjectMeshOutline(A, DraftViewType::Top, sheet.outlineATop.points);
    ProjectMeshOutline(B, DraftViewType::Top, sheet.outlineBTop.points);
    ProjectMeshOutline(A, DraftViewType::Right, sheet.outlineARight.points);
    ProjectMeshOutline(B, DraftViewType::Right, sheet.outlineBRight.points);

    sheet.boundsMin = Vec2(1e9f, 1e9f);
    sheet.boundsMax = Vec2(-1e9f, -1e9f);
    auto acc = [&](const std::vector<Vec2>& pts) {
        for (auto& p : pts) UpdateBounds(sheet, p);
    };
    acc(sheet.curveFront.points);
    acc(sheet.curveTop.points);
    acc(sheet.curveRight.points);
    acc(sheet.outlineAFront.points);
    acc(sheet.outlineBFront.points);

    if (sheet.boundsMin.x > sheet.boundsMax.x) {
        sheet.boundsMin = Vec2(-50, -50);
        sheet.boundsMax = Vec2(50, 50);
    }

    Vec2 ext = sheet.boundsMax - sheet.boundsMin;
    float span = ext.x > ext.y ? ext.x : ext.y;
    if (span < 1.0f) span = 1.0f;
    sheet.scale = 1.0f / span;
    sheet.origin = (sheet.boundsMin + sheet.boundsMax) * 0.5f;
    return sheet;
}