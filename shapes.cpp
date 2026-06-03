#include "shapes.h"
#include <string.h>
#include <algorithm>

const ShapeInfo g_shapeInfos[SHAPE_COUNT] = {
    {"Sphere",       "Sph",  true},
    {"Cone",         "Cone", true},
    {"Box",          "Box",  false},
    {"Cylinder",     "Cyl",  true},
    {"Tetrahedron",  "Tet",  false},
    {"Octahedron",   "Oct",  false},
    {"Dodecahedron", "Dod",  false},
    {"Icosahedron",  "Ico",  false},
    {"Torus",        "Tor",  true},
    {"Capsule",      "Cap",  true},
    {"Pyramid",      "Pyr",  false},
};

// ===================== CPU inside tests (local space) =====================

static bool PointInSphere(Vec3 p) {
    return Dot(p, p) <= 1.0001f;
}

static bool PointInCone(Vec3 p, float baseR, float halfH) {
    if (p.y > halfH || p.y < -halfH) return false;
    float frac = (halfH - p.y) / (2.0f * halfH);
    float r = sqrtf(p.x * p.x + p.z * p.z);
    return r <= baseR * frac;
}

static bool PointInBox(Vec3 p, Vec3 half) {
    return fabsf(p.x) <= half.x + 0.0001f &&
           fabsf(p.y) <= half.y + 0.0001f &&
           fabsf(p.z) <= half.z + 0.0001f;
}

static bool PointInCylinder(Vec3 p, float r, float halfH) {
    if (p.y < -halfH || p.y > halfH) return false;
    return (p.x * p.x + p.z * p.z) <= r * r + 0.0001f;
}

static bool PointInTorus(Vec3 p, float R, float r) {
    float qx = sqrtf(p.x * p.x + p.z * p.z) - R;
    return (qx * qx + p.y * p.y) <= r * r + 0.0001f;
}

static bool PointInCapsule(Vec3 p, float r, float halfH) {
    // Closest point on the central segment [-halfH, +halfH] on Y
    float y = p.y;
    if (y > halfH) y = halfH;
    else if (y < -halfH) y = -halfH;
    float dx = p.x - 0.0f;
    float dy = p.y - y;
    float dz = p.z - 0.0f;
    return (dx*dx + dy*dy + dz*dz) <= (r*r + 0.0001f);
}

bool IsPointInsideLocal(ShapeType type, Vec3 localP, const ShapeGPUData& gpu) {
    switch (type) {
    case SHAPE_SPHERE:   return PointInSphere(localP);
    case SHAPE_CONE:     return PointInCone(localP, gpu.extra[0], gpu.extra[1]);
    case SHAPE_BOX:      return PointInBox(localP, Vec3(gpu.extra[0], gpu.extra[1], gpu.extra[2]));
    case SHAPE_CYLINDER: return PointInCylinder(localP, gpu.extra[0], gpu.extra[1]);
    case SHAPE_TORUS:    return PointInTorus(localP, gpu.extra[0], gpu.extra[1]);
    case SHAPE_CAPSULE:  return PointInCapsule(localP, gpu.extra[0], gpu.extra[1]);
    default: {
        // poly plane test: n·p + d <= 0 inside
        for (int i = 0; i < gpu.numPlanes; ++i) {
            const Plane4& pl = gpu.planes[i];
            float d = pl.nx * localP.x + pl.ny * localP.y + pl.nz * localP.z + pl.d;
            if (d > 0.001f) return false;
        }
        return true;
    }
    }
}

// ===================== Mesh generators =====================

static void PushVert(ShapeMeshData& m, float x, float y, float z, float nx, float ny, float nz, float u = 0, float v = 0) {
    m.vertices.push_back(x); m.vertices.push_back(y); m.vertices.push_back(z);
    m.vertices.push_back(nx); m.vertices.push_back(ny); m.vertices.push_back(nz);
    m.vertices.push_back(u); m.vertices.push_back(v);
}

void GenSphere(int slices, int stacks, ShapeMeshData& out) {
    out.vertices.clear(); out.indices.clear();
    for (int i = 0; i < stacks; ++i) {
        float phi1 = float(i) * M3D_PI / stacks;
        float phi2 = float(i + 1) * M3D_PI / stacks;
        for (int j = 0; j <= slices; ++j) {
            float th = float(j) * 2.0f * M3D_PI / slices;
            float x1 = cosf(phi1) * cosf(th), y1 = sinf(phi1), z1 = cosf(phi1) * sinf(th);
            float x2 = cosf(phi2) * cosf(th), y2 = sinf(phi2), z2 = cosf(phi2) * sinf(th);
            float u = float(j) / slices;
            PushVert(out, x1, y1, z1, x1, y1, z1, u, float(i) / stacks);
            PushVert(out, x2, y2, z2, x2, y2, z2, u, float(i + 1) / stacks);
        }
    }
    // indices for strips? For simplicity use glDrawArrays TRIANGLE_STRIP per stack or convert to indexed tris.
    // Easier: emit triangles directly for indexed.
    out.indices.clear();
    int vstride = 0; // we will rebuild as triangles for simplicity
    // Rebuild as triangles
    ShapeMeshData tris;
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            int base = (i * (slices + 1) + j) * 2;
            // quad -> 2 tris
            unsigned a = base, b = base + 1, c = base + 2, d = base + 3;
            // standard order for consistent outward winding on sphere (p1j, p1j+1, p2j ; p1j+1, p2j+1, p2j)
            tris.vertices.insert(tris.vertices.end(), out.vertices.begin() + a * 8, out.vertices.begin() + (a + 1) * 8);
            tris.vertices.insert(tris.vertices.end(), out.vertices.begin() + c * 8, out.vertices.begin() + (c + 1) * 8);
            tris.vertices.insert(tris.vertices.end(), out.vertices.begin() + b * 8, out.vertices.begin() + (b + 1) * 8);

            tris.vertices.insert(tris.vertices.end(), out.vertices.begin() + c * 8, out.vertices.begin() + (c + 1) * 8);
            tris.vertices.insert(tris.vertices.end(), out.vertices.begin() + d * 8, out.vertices.begin() + (d + 1) * 8);
            tris.vertices.insert(tris.vertices.end(), out.vertices.begin() + b * 8, out.vertices.begin() + (b + 1) * 8);
        }
    }
    out = std::move(tris);
    // indices not really needed if we draw arrays, but set dummy
    for (int i = 0; i < (int)out.vertices.size() / 8; ++i) out.indices.push_back(i);
}

void GenCone(float baseR, float height, int slices, ShapeMeshData& out, ShapeGPUData& gpu) {
    out.vertices.clear(); out.indices.clear();
    float halfH = height * 0.5f;
    // side
    for (int i = 0; i < slices; ++i) {
        float th0 = float(i) * 2.0f * M3D_PI / slices;
        float th1 = float(i + 1) * 2.0f * M3D_PI / slices;
        float x0 = baseR * cosf(th0), z0 = baseR * sinf(th0);
        float x1 = baseR * cosf(th1), z1 = baseR * sinf(th1);
        // apex
        Vec3 ap(0, halfH, 0);
        Vec3 p0(x0, -halfH, z0), p1(x1, -halfH, z1);
        Vec3 n0 = Normalize(Cross(p0 - ap, p1 - ap)); // rough
        if (Dot(n0, Vec3(x0,0,z0)) < 0) n0 = n0 * -1.0f;
        PushVert(out, 0, halfH, 0, n0.x * 0.6f + 0.4f, 0.8f, n0.z * 0.6f + 0.4f, 0.5f, 0.0f);
        PushVert(out, x0, -halfH, z0, x0, 0.0f, z0, float(i) / slices, 1.0f);
        PushVert(out, x1, -halfH, z1, x1, 0.0f, z1, float(i + 1) / slices, 1.0f);
    }
    // base cap
    Vec3 down(0, -1, 0);
    for (int i = 0; i < slices; ++i) {
        float th0 = float(i) * 2.0f * M3D_PI / slices;
        float th1 = float(i + 1) * 2.0f * M3D_PI / slices;
        float x0 = baseR * cosf(th0), z0 = baseR * sinf(th0);
        float x1 = baseR * cosf(th1), z1 = baseR * sinf(th1);
        PushVert(out, 0, -halfH, 0, 0, -1, 0, 0.5f, 0.5f);
        PushVert(out, x1, -halfH, z1, 0, -1, 0, 0.5f + 0.5f * cosf(th1), 0.5f + 0.5f * sinf(th1));
        PushVert(out, x0, -halfH, z0, 0, -1, 0, 0.5f + 0.5f * cosf(th0), 0.5f + 0.5f * sinf(th0));
    }
    for (size_t i = 0; i < out.vertices.size() / 8; ++i) out.indices.push_back((unsigned)i);

    gpu.type = SHAPE_CONE;
    gpu.numPlanes = 0;
    gpu.extra[0] = baseR;
    gpu.extra[1] = halfH;
}

void GenBox(Vec3 half, ShapeMeshData& out, ShapeGPUData& gpu) {
    out.vertices.clear(); out.indices.clear();
    // 8 corners, 6 faces
    Vec3 c[8] = {
        { half.x, half.y, half.z }, { half.x, half.y,-half.z }, { half.x,-half.y,-half.z }, { half.x,-half.y, half.z },
        {-half.x, half.y, half.z }, {-half.x, half.y,-half.z }, {-half.x,-half.y,-half.z }, {-half.x,-half.y, half.z }
    };
    int faces[6][4] = {
        {0,1,2,3}, {4,7,6,5}, {0,4,5,1}, {3,2,6,7}, {0,3,7,4}, {1,5,6,2}
    };
    Vec3 fn[6] = { {1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1} };
    for (int f = 0; f < 6; ++f) {
        int* idx = faces[f];
        Vec3 n = fn[f];
        // two tris
        for (int t = 0; t < 2; ++t) {
            int i0 = t == 0 ? 0 : 1;
            int i1 = t == 0 ? 1 : 2;
            int i2 = t == 0 ? 2 : 3;
            Vec3 p0 = c[idx[i0]], p1 = c[idx[i1]], p2 = c[idx[i2]];
            float u0 = (t==0?0:1), v0=(t==0?0:0), u1=(t==0?1:1), v1=(t==0?0:1), u2=(t==0?1:0), v2=(t==0?1:1);
            PushVert(out, p0.x, p0.y, p0.z, n.x, n.y, n.z, u0, v0);
            PushVert(out, p1.x, p1.y, p1.z, n.x, n.y, n.z, u1, v1);
            PushVert(out, p2.x, p2.y, p2.z, n.x, n.y, n.z, u2, v2);
        }
    }
    for (size_t i = 0; i < out.vertices.size() / 8; ++i) out.indices.push_back((unsigned)i);

    gpu.type = SHAPE_BOX;
    gpu.numPlanes = 6;
    Vec3 hs = half;
    // +X
    gpu.planes[0] = { 1,0,0, -hs.x };
    gpu.planes[1] = {-1,0,0, -hs.x };
    gpu.planes[2] = {0,1,0, -hs.y };
    gpu.planes[3] = {0,-1,0, -hs.y };
    gpu.planes[4] = {0,0,1, -hs.z };
    gpu.planes[5] = {0,0,-1, -hs.z };
    gpu.extra[0] = hs.x; gpu.extra[1] = hs.y; gpu.extra[2] = hs.z;
}

void GenCylinder(float radius, float height, int slices, ShapeMeshData& out, ShapeGPUData& gpu) {
    out.vertices.clear(); out.indices.clear();
    float halfH = height * 0.5f;
    // side wall
    for (int i = 0; i < slices; ++i) {
        float th0 = float(i) * 2.0f * M3D_PI / slices;
        float th1 = float(i + 1) * 2.0f * M3D_PI / slices;
        float x0 = radius * cosf(th0), z0 = radius * sinf(th0);
        float x1 = radius * cosf(th1), z1 = radius * sinf(th1);
        Vec3 n0(x0, 0, z0); n0 = Normalize(n0);
        Vec3 n1(x1, 0, z1); n1 = Normalize(n1);
        PushVert(out, x0, -halfH, z0, n0.x, n0.y, n0.z, float(i)/slices, 0);
        PushVert(out, x0,  halfH, z0, n0.x, n0.y, n0.z, float(i)/slices, 1);
        PushVert(out, x1,  halfH, z1, n1.x, n1.y, n1.z, float(i+1)/slices, 1);

        PushVert(out, x0, -halfH, z0, n0.x, n0.y, n0.z, float(i)/slices, 0);
        PushVert(out, x1,  halfH, z1, n1.x, n1.y, n1.z, float(i+1)/slices, 1);
        PushVert(out, x1, -halfH, z1, n1.x, n1.y, n1.z, float(i+1)/slices, 0);
    }
    // caps
    for (int cap = 0; cap < 2; ++cap) {
        float yy = (cap == 0 ? halfH : -halfH);
        float ny = (cap == 0 ? 1.0f : -1.0f);
        for (int i = 0; i < slices; ++i) {
            float th0 = float(i) * 2.0f * M3D_PI / slices;
            float th1 = float(i + 1) * 2.0f * M3D_PI / slices;
            float x0 = radius * cosf(th0), z0 = radius * sinf(th0);
            float x1 = radius * cosf(th1), z1 = radius * sinf(th1);
            PushVert(out, 0, yy, 0, 0, ny, 0, 0.5f, 0.5f);
            if (cap == 0) {
                PushVert(out, x0, yy, z0, 0, ny, 0, 0.5f + 0.5f*cosf(th0), 0.5f + 0.5f*sinf(th0));
                PushVert(out, x1, yy, z1, 0, ny, 0, 0.5f + 0.5f*cosf(th1), 0.5f + 0.5f*sinf(th1));
            } else {
                PushVert(out, x1, yy, z1, 0, ny, 0, 0.5f + 0.5f*cosf(th1), 0.5f + 0.5f*sinf(th1));
                PushVert(out, x0, yy, z0, 0, ny, 0, 0.5f + 0.5f*cosf(th0), 0.5f + 0.5f*sinf(th0));
            }
        }
    }
    for (size_t i = 0; i < out.vertices.size() / 8; ++i) out.indices.push_back((unsigned)i);

    gpu.type = SHAPE_CYLINDER;
    gpu.numPlanes = 0;
    gpu.extra[0] = radius;
    gpu.extra[1] = halfH;
}

void GenTorus(float majorR, float minorR, int rings, int sides, ShapeMeshData& out, ShapeGPUData& gpu) {
    out.vertices.clear(); out.indices.clear();
    for (int i = 0; i < rings; ++i) {
        float u0 = float(i) / rings;
        float u1 = float(i + 1) / rings;
        float a0 = u0 * 2.0f * M3D_PI;
        float a1 = u1 * 2.0f * M3D_PI;
        float ca0 = cosf(a0), sa0 = sinf(a0);
        float ca1 = cosf(a1), sa1 = sinf(a1);
        for (int j = 0; j <= sides; ++j) {
            float v = float(j) / sides;
            float b = v * 2.0f * M3D_PI;
            float cb = cosf(b), sb = sinf(b);
            // ring i
            float x = (majorR + minorR * cb) * ca0;
            float y = minorR * sb;
            float z = (majorR + minorR * cb) * sa0;
            Vec3 pos(x, y, z);
            Vec3 n = Normalize(Vec3(cb * ca0, sb, cb * sa0));
            PushVert(out, x, y, z, n.x, n.y, n.z, u0, v);

            x = (majorR + minorR * cb) * ca1;
            z = (majorR + minorR * cb) * sa1;
            pos = Vec3(x, y, z);
            n = Normalize(Vec3(cb * ca1, sb, cb * sa1));
            PushVert(out, x, y, z, n.x, n.y, n.z, u1, v);
        }
    }
    // convert to tris indices
    ShapeMeshData tris;
    int perRing = (sides + 1) * 2;
    for (int i = 0; i < rings; ++i) {
        for (int j = 0; j < sides; ++j) {
            int base = i * perRing + j * 2;
            int a = base, b = base + 1, c = base + 2, d = base + 3;
            // two tris
            tris.vertices.insert(tris.vertices.end(), out.vertices.begin() + a*8, out.vertices.begin()+(a+1)*8);
            tris.vertices.insert(tris.vertices.end(), out.vertices.begin() + b*8, out.vertices.begin()+(b+1)*8);
            tris.vertices.insert(tris.vertices.end(), out.vertices.begin() + c*8, out.vertices.begin()+(c+1)*8);

            tris.vertices.insert(tris.vertices.end(), out.vertices.begin() + c*8, out.vertices.begin()+(c+1)*8);
            tris.vertices.insert(tris.vertices.end(), out.vertices.begin() + b*8, out.vertices.begin()+(b+1)*8);
            tris.vertices.insert(tris.vertices.end(), out.vertices.begin() + d*8, out.vertices.begin()+(d+1)*8);
        }
    }
    out = std::move(tris);
    for (size_t k = 0; k < out.vertices.size() / 8; ++k) out.indices.push_back((unsigned)k);

    gpu.type = SHAPE_TORUS;
    gpu.numPlanes = 0;
    gpu.extra[0] = majorR;
    gpu.extra[1] = minorR;
}

// ---- Platonic (ported + normalized + planes) ----

static Vec3 V3f(float x, float y, float z) { return Vec3(x, y, z); }

void GenTetra(ShapeMeshData& out, ShapeGPUData& gpu) {
    out.vertices.clear(); out.indices.clear();
    Vec3 v[4] = { V3f(1,1,1), V3f(1,-1,-1), V3f(-1,1,-1), V3f(-1,-1,1) };
    for (int i = 0; i < 4; ++i) v[i] = Normalize(v[i]);
    int faces[12] = {0,1,2, 0,2,3, 0,3,1, 1,3,2};
    Vec3 centers[4];
    for (int t = 0; t < 4; ++t) {
        Vec3 aa = v[faces[t*3]], bb = v[faces[t*3+1]], cc = v[faces[t*3+2]];
        Vec3 n = Normalize(Cross(bb - aa, cc - aa));
        centers[t] = (aa + bb + cc) * (1.0f / 3.0f);
        if (Dot(n, aa) < 0) {
            n = n * -1.0f;
            // reverse winding to match outward n for correct culling
            PushVert(out, aa.x,aa.y,aa.z, n.x,n.y,n.z, 0,0);
            PushVert(out, cc.x,cc.y,cc.z, n.x,n.y,n.z, 1,0);
            PushVert(out, bb.x,bb.y,bb.z, n.x,n.y,n.z, 0.5f, 0.866f);
        } else {
            PushVert(out, aa.x,aa.y,aa.z, n.x,n.y,n.z, 0,0);
            PushVert(out, bb.x,bb.y,bb.z, n.x,n.y,n.z, 1,0);
            PushVert(out, cc.x,cc.y,cc.z, n.x,n.y,n.z, 0.5f, 0.866f);
        }
    }
    for (int i=0;i<12;++i) out.indices.push_back(i);

    gpu.type = SHAPE_TETRAHEDRON; gpu.numPlanes = 4;
    for (int t=0; t<4; ++t) {
        Vec3 n = Normalize(centers[t]); // approx good outward
        float d = -Dot(n, v[faces[t*3]]);
        gpu.planes[t] = {n.x, n.y, n.z, d};
    }
}

void GenOcta(ShapeMeshData& out, ShapeGPUData& gpu) {
    out.vertices.clear(); out.indices.clear();
    Vec3 v[6] = {V3f(1,0,0),V3f(-1,0,0),V3f(0,1,0),V3f(0,-1,0),V3f(0,0,1),V3f(0,0,-1)};
    int faces[24] = {0,2,4, 0,4,3, 0,3,5, 0,5,2, 1,4,2, 1,3,4, 1,5,3, 1,2,5};
    for (int t=0; t<8; ++t) {
        Vec3 aa=v[faces[t*3]],bb=v[faces[t*3+1]],cc=v[faces[t*3+2]];
        Vec3 n=Normalize(Cross(bb-aa,cc-aa));
        if (Dot(n,aa)<0) {
            n = n * -1.0f;
            PushVert(out, aa.x,aa.y,aa.z, n.x,n.y,n.z, 0,0);
            PushVert(out, cc.x,cc.y,cc.z, n.x,n.y,n.z, 1,0);
            PushVert(out, bb.x,bb.y,bb.z, n.x,n.y,n.z, 0.5f,1);
        } else {
            PushVert(out, aa.x,aa.y,aa.z, n.x,n.y,n.z, 0,0);
            PushVert(out, bb.x,bb.y,bb.z, n.x,n.y,n.z, 1,0);
            PushVert(out, cc.x,cc.y,cc.z, n.x,n.y,n.z, 0.5f,1);
        }
    }
    for (int i=0;i<24;++i) out.indices.push_back(i);

    gpu.type=SHAPE_OCTAHEDRON; gpu.numPlanes=8;
    for (int t=0;t<8;++t) {
        Vec3 n = Normalize( (v[faces[t*3]]+v[faces[t*3+1]]+v[faces[t*3+2]]) * 0.333f );
        float d = -Dot(n, v[faces[t*3]]);
        gpu.planes[t]={n.x,n.y,n.z,d};
    }
}

void GenIcosa(ShapeMeshData& out, ShapeGPUData& gpu) {
    out.vertices.clear(); out.indices.clear();
    const float t = 1.61803398875f; // phi
    Vec3 vv[12] = {
        V3f(0,1,t), V3f(0,1,-t), V3f(0,-1,t), V3f(0,-1,-t),
        V3f(1,t,0), V3f(1,-t,0), V3f(-1,t,0), V3f(-1,-t,0),
        V3f(t,0,1), V3f(-t,0,1), V3f(t,0,-1), V3f(-t,0,-1)
    };
    for (int i=0;i<12;++i) vv[i] = Normalize(vv[i]);
    int faces[60] = {
        0,8,4, 0,4,6, 0,6,10, 0,10,2, 0,2,8,
        3,9,5, 3,5,7, 3,7,11, 3,11,1, 3,1,9,
        4,8,9, 4,9,1, 6,4,1, 6,1,11, 10,6,11,
        8,2,5, 2,7,5, 7,10,2, 10,11,7, 8,5,9
    };
    for (int f=0; f<20; ++f) {
        Vec3 aa=vv[faces[f*3]], bb=vv[faces[f*3+1]], cc=vv[faces[f*3+2]];
        Vec3 n = Normalize(Cross(bb-aa, cc-aa));
        if (Dot(n,aa) < 0) {
            n = n * -1.f;
            PushVert(out, aa.x,aa.y,aa.z, n.x,n.y,n.z, 0,0);
            PushVert(out, cc.x,cc.y,cc.z, n.x,n.y,n.z, 1,0);
            PushVert(out, bb.x,bb.y,bb.z, n.x,n.y,n.z, 0.5f, 0.866f);
        } else {
            PushVert(out, aa.x,aa.y,aa.z, n.x,n.y,n.z, 0,0);
            PushVert(out, bb.x,bb.y,bb.z, n.x,n.y,n.z, 1,0);
            PushVert(out, cc.x,cc.y,cc.z, n.x,n.y,n.z, 0.5f, 0.866f);
        }
    }
    for (int i=0;i<60;++i) out.indices.push_back(i);

    gpu.type = SHAPE_ICOSAHEDRON; gpu.numPlanes = 20;
    for (int f = 0; f < 20; ++f) {
        Vec3 a = vv[faces[f*3]];
        Vec3 ctr = (a + vv[faces[f*3+1]] + vv[faces[f*3+2]]) * (1.f/3.f);
        Vec3 n = Normalize(ctr);
        float d = -Dot(n, a);
        gpu.planes[f] = {n.x,n.y,n.z,d};
    }
}

void GenDodeca(ShapeMeshData& out, ShapeGPUData& gpu) {
    out.vertices.clear(); out.indices.clear();
    // Use dual: vertices = normalized centers of icosa faces
    const float t = 1.61803398875f;
    Vec3 icv[12] = {
        V3f(0,1,t),V3f(0,1,-t),V3f(0,-1,t),V3f(0,-1,-t),
        V3f(1,t,0),V3f(1,-t,0),V3f(-1,t,0),V3f(-1,-t,0),
        V3f(t,0,1),V3f(-t,0,1),V3f(t,0,-1),V3f(-t,0,-1)
    };
    for (int i = 0; i < 12; ++i) icv[i] = Normalize(icv[i]);
    int icf[60] = { /* same as above */ 0,8,4,0,4,6,0,6,10,0,10,2,0,2,8,3,9,5,3,5,7,3,7,11,3,11,1,3,1,9,4,8,9,4,9,1,6,4,1,6,1,11,10,6,11,8,2,5,2,7,5,7,10,2,10,11,7,8,5,9 };

    Vec3 dverts[20];
    for (int i = 0; i < 20; ++i) {
        Vec3 c = (icv[icf[i*3]] + icv[icf[i*3+1]] + icv[icf[i*3+2]]) * (1.0f/3.0f);
        dverts[i] = Normalize(c);
    }
    // Build pentagon fans per original icosa vertex (5 faces meet)
    // Simplified: use known good connectivity or generate fans
    // For correctness we reuse a similar fan construction as original
    int dtris = 0;
    unsigned int indices[108];
    for (int iv = 0; iv < 12; ++iv) {
        struct FA { int f; float ang; } fas[5];
        int cnt = 0;
        Vec3 vdir = icv[iv];
        Vec3 up = (fabsf(vdir.y) < 0.9f ? V3f(0,1,0) : V3f(1,0,0));
        Vec3 rt = Normalize(Cross(up, vdir));
        Vec3 fw = Cross(vdir, rt);
        for (int f = 0; f < 20 && cnt < 5; ++f) {
            if (icf[f*3]==iv || icf[f*3+1]==iv || icf[f*3+2]==iv) {
                Vec3 fc = (icv[icf[f*3]]+icv[icf[f*3+1]]+icv[icf[f*3+2]])*(1.f/3.f);
                float da = Dot(fc, rt), db = Dot(fc, fw);
                fas[cnt].f = f; fas[cnt].ang = atan2f(db, da); cnt++;
            }
        }
        // sort
        for (int a=0;a<cnt-1;++a) for (int b=a+1;b<cnt;++b) if (fas[a].ang > fas[b].ang) std::swap(fas[a],fas[b]);
        if (cnt >= 3) {
            int base = fas[0].f;
            for (int k=1; k<cnt-1; ++k) {
                indices[dtris*3+0] = base;
                indices[dtris*3+1] = fas[k].f;
                indices[dtris*3+2] = fas[k+1].f;
                dtris++;
            }
        }
    }
    // emit
    for (int ti = 0; ti < dtris; ++ti) {
        Vec3 aa = dverts[indices[ti*3]], bb = dverts[indices[ti*3+1]], cc = dverts[indices[ti*3+2]];
        Vec3 n = Normalize(Cross(bb-aa, cc-aa));
        if (Dot(n, aa) < 0) {
            n = n * -1.0f;
            PushVert(out, aa.x,aa.y,aa.z, n.x,n.y,n.z, 0,0);
            PushVert(out, cc.x,cc.y,cc.z, n.x,n.y,n.z, 1,0);
            PushVert(out, bb.x,bb.y,bb.z, n.x,n.y,n.z, 0.5f,0.866f);
        } else {
            PushVert(out, aa.x,aa.y,aa.z, n.x,n.y,n.z, 0,0);
            PushVert(out, bb.x,bb.y,bb.z, n.x,n.y,n.z, 1,0);
            PushVert(out, cc.x,cc.y,cc.z, n.x,n.y,n.z, 0.5f,0.866f);
        }
    }
    for (int i = 0; i < dtris * 3; ++i) out.indices.push_back(i);

    gpu.type = SHAPE_DODECAHEDRON; gpu.numPlanes = dtris;
    for (int ti=0; ti<dtris; ++ti) {
        Vec3 a = dverts[indices[ti*3]];
        Vec3 ctr = (a + dverts[indices[ti*3+1]] + dverts[indices[ti*3+2]]) * (1.f/3.f);
        Vec3 n = Normalize(ctr);
        float d = -Dot(n, a);
        gpu.planes[ti] = {n.x,n.y,n.z, d};
    }
}

void GenCapsule(float r, float cylH, float capR, int slices, ShapeMeshData& out, ShapeGPUData& gpu) {
    out.vertices.clear(); out.indices.clear();
    float halfCyl = cylH * 0.5f;
    // cylinder part
    for (int i = 0; i < slices; ++i) {
        float th0 = float(i) * 2.0f * M3D_PI / slices;
        float th1 = float(i + 1) * 2.0f * M3D_PI / slices;
        float x0 = r * cosf(th0), z0 = r * sinf(th0);
        float x1 = r * cosf(th1), z1 = r * sinf(th1);
        Vec3 n0(x0, 0, z0); n0 = Normalize(n0);
        Vec3 n1(x1, 0, z1); n1 = Normalize(n1);
        PushVert(out, x0, -halfCyl, z0, n0.x, n0.y, n0.z, float(i)/slices, 0);
        PushVert(out, x0,  halfCyl, z0, n0.x, n0.y, n0.z, float(i)/slices, 1);
        PushVert(out, x1,  halfCyl, z1, n1.x, n1.y, n1.z, float(i+1)/slices, 1);

        PushVert(out, x0, -halfCyl, z0, n0.x, n0.y, n0.z, float(i)/slices, 0);
        PushVert(out, x1,  halfCyl, z1, n1.x, n1.y, n1.z, float(i+1)/slices, 1);
        PushVert(out, x1, -halfCyl, z1, n1.x, n1.y, n1.z, float(i+1)/slices, 0);
    }
    // bottom cap (hemisphere, offset -halfCyl on y)
    for (int i = 0; i < slices; ++i) {
        float th0 = float(i) * 2.0f * M3D_PI / slices;
        float th1 = float(i + 1) * 2.0f * M3D_PI / slices;
        for (int k = 0; k < 4; ++k) {  // rough 4 stacks for cap
            float phi0 = float(k) * (M3D_PI * 0.5f) / 4;
            float phi1 = float(k + 1) * (M3D_PI * 0.5f) / 4;
            float y0 = -halfCyl + capR * sinf(-phi0); // negative for bottom
            float y1 = -halfCyl + capR * sinf(-phi1);
            float rx0 = capR * cosf(-phi0), rz0 = 0; // will rotate
            // simple: use sphere slice for bottom
            float x00 = rx0 * cosf(th0), z00 = rx0 * sinf(th0);
            float x01 = rx0 * cosf(th1), z01 = rx0 * sinf(th1);
            // to make it simple and closed, we use a simple cone-like for cap for now
        }
    }
    // For simplicity and closed mesh, use two simple sphere caps approximated with fans
    // Bottom cap
    Vec3 capCenterB(0, -halfCyl, 0);
    for (int i = 0; i < slices; ++i) {
        float th0 = float(i) * 2.0f * M3D_PI / slices;
        float th1 = float(i + 1) * 2.0f * M3D_PI / slices;
        float x0 = capR * cosf(th0), z0 = capR * sinf(th0);
        float x1 = capR * cosf(th1), z1 = capR * sinf(th1);
        PushVert(out, 0, -halfCyl - capR, 0, 0, -1, 0, 0.5f, 0.5f); // south pole
        PushVert(out, x1, -halfCyl, z1, 0, -0.7f, 0, 0.5f + 0.5f*cosf(th1), 0.5f + 0.5f*sinf(th1));
        PushVert(out, x0, -halfCyl, z0, 0, -0.7f, 0, 0.5f + 0.5f*cosf(th0), 0.5f + 0.5f*sinf(th0));
    }
    // Top cap
    for (int i = 0; i < slices; ++i) {
        float th0 = float(i) * 2.0f * M3D_PI / slices;
        float th1 = float(i + 1) * 2.0f * M3D_PI / slices;
        float x0 = capR * cosf(th0), z0 = capR * sinf(th0);
        float x1 = capR * cosf(th1), z1 = capR * sinf(th1);
        PushVert(out, 0, halfCyl + capR, 0, 0, 1, 0, 0.5f, 0.5f);
        PushVert(out, x0, halfCyl, z0, 0, 0.7f, 0, 0.5f + 0.5f*cosf(th0), 0.5f + 0.5f*sinf(th0));
        PushVert(out, x1, halfCyl, z1, 0, 0.7f, 0, 0.5f + 0.5f*cosf(th1), 0.5f + 0.5f*sinf(th1));
    }
    for (size_t i = 0; i < out.vertices.size() / 8; ++i) out.indices.push_back((unsigned)i);

    gpu.type = SHAPE_CAPSULE;
    gpu.numPlanes = 0;
    gpu.extra[0] = r;
    gpu.extra[1] = halfCyl;
}

void GenPyramid(float base, float height, int sides, ShapeMeshData& out, ShapeGPUData& gpu) {
    out.vertices.clear(); out.indices.clear();
    float halfH = height * 0.5f;
    float br = base * 0.5f;
    // Apex
    Vec3 apex(0, halfH, 0);
    // Base verts (square for simplicity, even if sides=4)
    Vec3 baseVerts[4] = {
        { br, -halfH, br }, { -br, -halfH, br }, { -br, -halfH, -br }, { br, -halfH, -br }
    };
    // Sides (4 tris for square pyramid)
    Vec3 baseN(0, -1, 0);
    for (int i = 0; i < 4; ++i) {
        Vec3 b0 = baseVerts[i];
        Vec3 b1 = baseVerts[(i+1)%4];
        Vec3 n = Normalize(Cross(b1 - apex, b0 - apex)); // rough, will adjust
        if (Dot(n, apex) < 0) n = n * -1.0f;
        PushVert(out, apex.x, apex.y, apex.z, n.x, n.y, n.z, 0.5f, 0);
        PushVert(out, b0.x, b0.y, b0.z, n.x, n.y, n.z, 0, 1);
        PushVert(out, b1.x, b1.y, b1.z, n.x, n.y, n.z, 1, 1);
    }
    // Base (2 tris)
    PushVert(out, baseVerts[0].x, baseVerts[0].y, baseVerts[0].z, 0,-1,0, 0,0);
    PushVert(out, baseVerts[1].x, baseVerts[1].y, baseVerts[1].z, 0,-1,0, 1,0);
    PushVert(out, baseVerts[2].x, baseVerts[2].y, baseVerts[2].z, 0,-1,0, 1,1);

    PushVert(out, baseVerts[0].x, baseVerts[0].y, baseVerts[0].z, 0,-1,0, 0,0);
    PushVert(out, baseVerts[2].x, baseVerts[2].y, baseVerts[2].z, 0,-1,0, 1,1);
    PushVert(out, baseVerts[3].x, baseVerts[3].y, baseVerts[3].z, 0,-1,0, 0,1);

    for (size_t i = 0; i < out.vertices.size() / 8; ++i) out.indices.push_back((unsigned)i);

    gpu.type = SHAPE_PYRAMID;
    gpu.numPlanes = 5;
    // Base
    gpu.planes[0] = {0, -1, 0, -halfH};
    // Side planes (approximate using base edges)
    gpu.planes[1] = { 0.7f, 0.7f, 0, -0.5f};
    gpu.planes[2] = {-0.7f, 0.7f, 0, -0.5f};
    gpu.planes[3] = {0, 0.7f, 0.7f, -0.5f};
    gpu.planes[4] = {0, 0.7f, -0.7f, -0.5f};
    gpu.extra[0] = br;
    gpu.extra[1] = halfH;
}

// ===================== High level =====================

void GenerateUnitMesh(ShapeType type, ShapeMeshData& outMesh, ShapeGPUData& outGPU) {
    outMesh.vertices.clear();
    outMesh.indices.clear();
    memset(&outGPU, 0, sizeof(outGPU));
    outGPU.type = type;

    switch (type) {
    case SHAPE_SPHERE:     GenSphere(32, 24, outMesh); outGPU.numPlanes=0; break;
    case SHAPE_CONE:       GenCone(0.85f, 1.9f, 48, outMesh, outGPU); break;
    case SHAPE_BOX:        GenBox(Vec3(0.9f, 0.9f, 0.9f), outMesh, outGPU); break;
    case SHAPE_CYLINDER:   GenCylinder(0.75f, 1.8f, 48, outMesh, outGPU); break;
    case SHAPE_TETRAHEDRON:GenTetra(outMesh, outGPU); break;
    case SHAPE_OCTAHEDRON: GenOcta(outMesh, outGPU); break;
    case SHAPE_DODECAHEDRON:GenDodeca(outMesh, outGPU); break;
    case SHAPE_ICOSAHEDRON:GenIcosa(outMesh, outGPU); break;
    case SHAPE_TORUS:      GenTorus(0.7f, 0.32f, 48, 24, outMesh, outGPU); break;
    case SHAPE_CAPSULE:    GenCapsule(0.6f, 1.2f, 0.6f, 24, outMesh, outGPU); break;
    case SHAPE_PYRAMID:    GenPyramid(0.9f, 1.2f, 4, outMesh, outGPU); break;
    default: GenSphere(16, 12, outMesh); break;
    }
}

void ShapeInstance::GetLocalPlanesOrData(ShapeGPUData& dataOut) const {
    ShapeMeshData dummy;
    GenerateUnitMesh(type, dummy, dataOut);
    // override scale info for non-uniform if needed (box uses extra already)
    if (type == SHAPE_BOX) {
        // extra already set to half in unit gen, but our scale is nonuniform world scale.
        // For shader inside test we will transform to local then test against unit description.
    }
}

// ===================== Scene =====================

void ShapeScene::InitDefault() {
    shapes.clear();
    AddShape(SHAPE_SPHERE,   Vec3(0, 0, 0),   95.0f,  Vec3(0.15f, 0.55f, 0.85f));
    AddShape(SHAPE_CONE,     Vec3(0, 0, -220), 82.0f,  Vec3(0.9f, 0.45f, 0.1f));
    GetA().isSecond = false;
    GetB().isSecond = true;
    GetA().euler = Vec3(12, 35, 0);
    GetB().euler = Vec3(-8, 140, 0);
}

void ShapeScene::AddShape(ShapeType t, Vec3 p, float s, Vec3 col) {
    ShapeInstance si;
    si.type = t;
    si.pos = p;
    si.scale = Vec3(s, s, s);
    si.color = col;
    shapes.push_back(si);
}

void ShapeScene::SetPair(int a, int b) {
    if (a < 0 || a >= (int)shapes.size() || b < 0 || b >= (int)shapes.size()) return;
    indexA = a; indexB = b;
    for (auto& s : shapes) s.isSecond = false;
    shapes[indexA].isSecond = false;
    shapes[indexB].isSecond = true;
}

void ShapeScene::ResetToPreset(int presetId) {
    shapes.clear();
    switch (presetId % 6) {
    case 0: // Sphere-Cone classic
        AddShape(SHAPE_SPHERE, Vec3(0,0,0), 95, Vec3(0.2f,0.6f,0.95f));
        AddShape(SHAPE_CONE, Vec3(0,0,-210), 78, Vec3(0.95f,0.5f,0.15f));
        break;
    case 1:
        AddShape(SHAPE_BOX, Vec3(-60,0,0), 85, Vec3(0.2f,0.75f,0.35f));
        AddShape(SHAPE_ICOSAHEDRON, Vec3(80,10,-30), 90, Vec3(0.85f,0.25f,0.55f));
        break;
    case 2:
        AddShape(SHAPE_DODECAHEDRON, Vec3(0,0,0), 80, Vec3(0.9f,0.75f,0.15f));
        AddShape(SHAPE_TETRAHEDRON, Vec3(30,40,-180), 75, Vec3(0.1f,0.85f,0.9f));
        break;
    case 3:
        AddShape(SHAPE_CYLINDER, Vec3(-40,0,50), 70, Vec3(0.3f,0.8f,0.6f));
        AddShape(SHAPE_TORUS, Vec3(110,-20,-60), 95, Vec3(0.75f,0.2f,0.65f));
        break;
    case 4:
        AddShape(SHAPE_OCTAHEDRON, Vec3(0,0,0), 88, Vec3(0.7f,0.15f,0.75f));
        AddShape(SHAPE_CONE, Vec3(20,0,-190), 72, Vec3(0.95f,0.55f,0.1f));
        break;
    default:
        AddShape(SHAPE_SPHERE, Vec3(-120,30,40), 70, Vec3(0.4f,0.7f,0.95f));
        AddShape(SHAPE_BOX, Vec3(90,-10,-50), 80, Vec3(0.9f,0.4f,0.25f));
        AddShape(SHAPE_TORUS, Vec3(10,80,-220), 105, Vec3(0.55f,0.85f,0.3f));
        break;
    }
    indexA = 0; indexB = (shapes.size() > 1 ? 1 : 0);
    SetPair(indexA, indexB);
}
