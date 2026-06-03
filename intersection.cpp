#include "intersection.h"
#include <algorithm>
#include <cmath>

int IntersectionSampleCount(IntersectionQuality q) {
    switch (q) {
    case IntersectionQuality::Low:    return 800;
    case IntersectionQuality::High:   return 6000;
    case IntersectionQuality::Medium:
    default:                          return 2200;
    }
}

Vec3 TransformToLocal(Vec3 worldP, Vec3 pos, Vec3 eulerDeg, Vec3 scale3) {
    Vec3 d = worldP - pos;
    d.x /= (scale3.x == 0 ? 1 : scale3.x);
    d.y /= (scale3.y == 0 ? 1 : scale3.y);
    d.z /= (scale3.z == 0 ? 1 : scale3.z);
    float rx = -M3D_DEG2RAD(eulerDeg.x);
    float ry = -M3D_DEG2RAD(eulerDeg.y);
    float rz = -M3D_DEG2RAD(eulerDeg.z);
    Mat4 Ri = Mul(Mul(RotateZ(rz), RotateY(ry)), RotateX(rx));
    return MulDir(Ri, d);
}

static void SampleSurface(const ShapeInstance& si, int maxPts, std::vector<Vec3>& outPts) {
    outPts.clear();
    ShapeType t = si.type;
    int n = (int)sqrtf((float)maxPts);
    if (n < 4) n = 4;

    if (t == SHAPE_SPHERE) {
        for (int i = 0; i < n; ++i) {
            float phi = M3D_PI * (i + 0.5f) / n;
            for (int j = 0; j < n * 2; ++j) {
                float th = 2.0f * M3D_PI * j / (n * 2);
                Vec3 l(cosf(phi) * cosf(th), sinf(phi), cosf(phi) * sinf(th));
                outPts.push_back(MulPoint(si.GetModel(), l));
                if ((int)outPts.size() >= maxPts) return;
            }
        }
    } else if (t == SHAPE_CONE || t == SHAPE_CYLINDER) {
        float hh = 0.95f;
        for (int i = 0; i < n; ++i) {
            float f = float(i) / (n - 1);
            float y = hh - f * 2.0f * hh;
            float rad = (t == SHAPE_CONE ? 0.85f * (1.0f - f) : 0.75f);
            for (int j = 0; j < n * 2; ++j) {
                float th = 2.0f * M3D_PI * j / (n * 2);
                Vec3 l(rad * cosf(th), y, rad * sinf(th));
                outPts.push_back(MulPoint(si.GetModel(), l));
                if ((int)outPts.size() >= maxPts) return;
            }
        }
    } else if (t == SHAPE_TORUS) {
        for (int i = 0; i < n * 2; ++i) {
            float a = 2.0f * M3D_PI * i / (n * 2);
            for (int j = 0; j < n; ++j) {
                float b = 2.0f * M3D_PI * j / n;
                float R = 0.7f, r = 0.32f;
                Vec3 l((R + r * cosf(b)) * cosf(a), r * sinf(b), (R + r * cosf(b)) * sinf(a));
                outPts.push_back(MulPoint(si.GetModel(), l));
                if ((int)outPts.size() >= maxPts) return;
            }
        }
    } else {
        ShapeMeshData md;
        ShapeGPUData gd;
        GenerateUnitMesh(t, md, gd);
        int ntris = (int)md.indices.size() / 3;
        if (ntris < 1) ntris = 1;
        for (int ti = 0; ti < ntris && (int)outPts.size() < maxPts; ++ti) {
            int i0 = md.indices[ti * 3 + 0] * 8;
            int i1 = md.indices[ti * 3 + 1] * 8;
            int i2 = md.indices[ti * 3 + 2] * 8;
            Vec3 a(md.vertices[i0], md.vertices[i0 + 1], md.vertices[i0 + 2]);
            Vec3 b(md.vertices[i1], md.vertices[i1 + 1], md.vertices[i1 + 2]);
            Vec3 c(md.vertices[i2], md.vertices[i2 + 1], md.vertices[i2 + 2]);
            for (int u = 0; u <= 2 && (int)outPts.size() < maxPts; ++u) {
                for (int v = 0; v <= 2 - u && (int)outPts.size() < maxPts; ++v) {
                    float uu = float(u) / 2.0f, vv = float(v) / 2.0f, ww = 1.0f - uu - vv;
                    Vec3 lp = a * ww + b * uu + c * vv;
                    outPts.push_back(MulPoint(si.GetModel(), lp));
                }
            }
        }
    }
}

struct AngEntry {
    float ang;
    Vec3 p;
};

void IntersectionSolver::BuildSortedCurve(IntersectionResult& result) {
    result.sortedCurvePoints.clear();
    std::vector<Vec3> pts = result.rawCurvePoints;
    if (pts.empty()) {
        pts = result.pointsAInsideB;
        for (auto& p : result.pointsBInsideA) pts.push_back(p);
    }
    if (pts.size() < 3) {
        result.sortedCurvePoints = pts;
        return;
    }

    Vec3 c(0, 0, 0);
    for (auto& p : pts) c = c + p;
    c = c * (1.0f / pts.size());

    int i0 = 0, i1 = 1;
    float maxD = 0.0f;
    for (size_t i = 0; i < pts.size(); ++i) {
        for (size_t j = i + 1; j < pts.size(); ++j) {
            float d = Length(pts[i] - pts[j]);
            if (d > maxD) { maxD = d; i0 = (int)i; i1 = (int)j; }
        }
    }

    Vec3 n = Normalize(Cross(pts[i1] - c, pts[i0] - c));
    if (Length(n) < 1e-3f) n = Vec3(0, 1, 0);

    Vec3 u = Normalize(Cross(n, Vec3(0, 1, 0)));
    if (Length(u) < 1e-3f) u = Normalize(Cross(n, Vec3(1, 0, 0)));
    Vec3 v = Cross(n, u);

    std::vector<float> dists;
    dists.reserve(pts.size());
    for (auto& p : pts) dists.push_back(Length(p - c));
    std::sort(dists.begin(), dists.end());
    float median = dists[dists.size() / 2];
    float thresh = median * 2.5f + 1e-3f;

    std::vector<AngEntry> entries;
    entries.reserve(pts.size());
    for (auto& p : pts) {
        if (pts.size() > 12 && Length(p - c) > thresh) continue;
        Vec3 d = p - c;
        float sx = Dot(d, u), sy = Dot(d, v);
        entries.push_back({ atan2f(sy, sx), p });
    }
    if (entries.size() < 3) {
        for (auto& p : pts) {
            Vec3 d = p - c;
            entries.push_back({ atan2f(Dot(d, v), Dot(d, u)), p });
        }
    }

    std::sort(entries.begin(), entries.end(),
        [](const AngEntry& a, const AngEntry& b) { return a.ang < b.ang; });

    result.sortedCurvePoints.reserve(entries.size());
    for (auto& e : entries) result.sortedCurvePoints.push_back(e.p);
    if (!result.sortedCurvePoints.empty())
        result.sortedCurvePoints.push_back(result.sortedCurvePoints.front());
}

IntersectionResult IntersectionSolver::Compute(const ShapeInstance& A, const ShapeInstance& B, int samplesPerShape) {
    IntersectionResult out;
    std::vector<Vec3> surfA, surfB;
    SampleSurface(A, samplesPerShape, surfA);
    SampleSurface(B, samplesPerShape, surfB);

    ShapeGPUData gA, gB;
    ShapeMeshData dA, dB;
    GenerateUnitMesh(A.type, dA, gA);
    GenerateUnitMesh(B.type, dB, gB);

    for (auto& p : surfA) {
        Vec3 local = TransformToLocal(p, B.pos, B.euler, B.scale);
        if (IsPointInsideLocal(B.type, local, gB))
            out.pointsAInsideB.push_back(p);
    }
    for (auto& p : surfB) {
        Vec3 local = TransformToLocal(p, A.pos, A.euler, A.scale);
        if (IsPointInsideLocal(A.type, local, gA))
            out.pointsBInsideA.push_back(p);
    }

    out.rawCurvePoints = out.pointsAInsideB;
    for (auto& p : out.pointsBInsideA) out.rawCurvePoints.push_back(p);

    int total = (int)(out.pointsAInsideB.size() + out.pointsBInsideA.size());
    out.overlapEstimate = total > 0 ? (float)total / 48.0f : 0.0f;
    if (out.overlapEstimate > 1.0f) out.overlapEstimate = 1.0f;
    out.valid = total > 0;

    BuildSortedCurve(out);
    if (!out.sortedCurvePoints.empty())
        out.curves.push_back(out.sortedCurvePoints);

    return out;
}