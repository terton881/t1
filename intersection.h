#pragma once
// intersection.h - CPU intersection sampling (geometry only, no rendering)

#include "math3d.h"
#include "shapes.h"
#include <vector>

enum class IntersectionQuality {
    Low = 0,
    Medium = 1,
    High = 2
};

int IntersectionSampleCount(IntersectionQuality q);

struct IntersectionResult {
    std::vector<Vec3> pointsAInsideB;
    std::vector<Vec3> pointsBInsideA;
    std::vector<Vec3> rawCurvePoints;
    std::vector<Vec3> sortedCurvePoints;
    std::vector<std::vector<Vec3>> curves;
    float overlapEstimate = 0.0f;
    bool valid = false;
};

// World point -> target shape local space (matches shader / CPU tests)
Vec3 TransformToLocal(Vec3 worldP, Vec3 pos, Vec3 eulerDeg, Vec3 scale3);

class IntersectionSolver {
public:
    IntersectionResult Compute(const ShapeInstance& A, const ShapeInstance& B, int samplesPerShape);

    // Build sortedCurvePoints from inside samples (angular sort in best-fit plane)
    static void BuildSortedCurve(IntersectionResult& result);
};