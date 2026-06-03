// SphereCone3D.cpp
// Win32 OpenGL Application demonstrating 3D shape intersection
// Supports: Sphere, Cone, Cube, Octahedron, Dodecahedron, Tetrahedron, Icosahedron
// Select shapes via menu or keyboard shortcuts.

#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <math.h>
#include <stdio.h>

#define PI 3.14159265358979323846f

// ===================== Vec3 =====================
struct Vec3 {
    float x, y, z;
};

static inline Vec3 V3(float x, float y, float z) { Vec3 v; v.x=x; v.y=y; v.z=z; return v; }
static inline Vec3 Add(Vec3 a, Vec3 b) { return V3(a.x+b.x, a.y+b.y, a.z+b.z); }
static inline Vec3 Sub(Vec3 a, Vec3 b) { return V3(a.x-b.x, a.y-b.y, a.z-b.z); }
static inline Vec3 Scale(Vec3 a, float s) { return V3(a.x*s, a.y*s, a.z*s); }
static inline float Dot(Vec3 a, Vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static inline Vec3 Cross(Vec3 a, Vec3 b) {
    return V3(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x);
}
static inline float Length(Vec3 a) { return sqrtf(Dot(a, a)); }
static inline Vec3 Normalize(Vec3 a) {
    float l = Length(a);
    if (l < 1e-10f) return V3(0,0,0);
    return Scale(a, 1.0f / l);
}

// ===================== Plane (half-space) =====================
struct Plane {
    float nx, ny, nz, d; // n·p + d <= 0 means inside
};

// ===================== Shape types =====================
enum ShapeType {
    SHAPE_SPHERE = 0,
    SHAPE_CONE,
    SHAPE_CUBE,
    SHAPE_OCTAHEDRON,
    SHAPE_DODECAHEDRON,
    SHAPE_TETRAHEDRON,
    SHAPE_ICOSAHEDRON,
    SHAPE_COUNT
};

static const wchar_t* ShapeNames[] = {
    L"Sphere", L"Cone", L"Cube", L"Octahedron",
    L"Dodecahedron", L"Tetrahedron", L"Icosahedron"
};

// Per-shape mesh data (for polyhedra; sphere/cone are drawn procedurally)
struct MeshData {
    Vec3* vertices;
    int   numVertices;
    int*  indices;   // triangle list (3 per triangle)
    int   numTriangles;
    Plane* planes;   // precomputed (numTriangles planes)
};

// ===================== Mesh data for polyhedra =====================

// --- Tetrahedron ---
static Vec3 s_tetraVerts[] = {
    V3( 1, 1, 1), V3( 1,-1,-1), V3(-1, 1,-1), V3(-1,-1, 1)
};
static int s_tetraFaces[] = {
    0,1,2, 0,2,3, 0,3,1, 1,3,2
};

// --- Cube ---
static Vec3 s_cubeVerts[] = {
    V3( 1, 1, 1), V3( 1, 1,-1), V3( 1,-1,-1), V3( 1,-1, 1),
    V3(-1, 1, 1), V3(-1, 1,-1), V3(-1,-1,-1), V3(-1,-1, 1)
};
static int s_cubeFaces[] = {
    // +X
    3,2,1, 3,1,0,
    // -X
    4,5,6, 4,6,7,
    // +Y
    0,1,5, 0,5,4,
    // -Y
    7,6,2, 7,2,3,
    // +Z
    0,4,7, 0,7,3,
    // -Z
    2,6,5, 2,5,1
};

// --- Octahedron ---
static Vec3 s_octaVerts[] = {
    V3( 1, 0, 0), V3(-1, 0, 0),
    V3( 0, 1, 0), V3( 0,-1, 0),
    V3( 0, 0, 1), V3( 0, 0,-1)
};
static int s_octaFaces[] = {
    0,2,4, 0,4,3, 0,3,5, 0,5,2,
    1,4,2, 1,3,4, 1,5,3, 1,2,5
};

// --- Dodecahedron ---
// Built programmatically in InitMeshes() as the dual of the icosahedron.
#define PHI 1.618033988749895f

static Vec3 s_dodecaFinalVerts[20];
static int  s_dodecaFinalIndices[108]; // 36 triangles * 3
static int  s_dodecaNumTris = 0;

// --- Icosahedron ---
static Vec3 s_icosaVerts[] = {
    V3( 0,  1, PHI),   // 0
    V3( 0,  1,-PHI),   // 1
    V3( 0, -1, PHI),   // 2
    V3( 0, -1,-PHI),   // 3
    V3( 1, PHI,  0),   // 4
    V3( 1,-PHI,  0),   // 5
    V3(-1, PHI,  0),   // 6
    V3(-1,-PHI,  0),   // 7
    V3( PHI,  0,  1),  // 8
    V3( PHI,  0, -1),  // 9
    V3(-PHI,  0,  1),  // 10
    V3(-PHI,  0, -1)   // 11
};
static int s_icosaFaces[] = {
    0, 8, 4,   0, 4, 6,   0, 6,10,   0,10, 2,   0, 2, 8,
    3, 9, 5,   3, 5, 7,   3, 7,11,   3,11, 1,   3, 1, 9,
    4, 8, 9,   4, 9, 1,   6, 4, 1,   6, 1,11,  10, 6,11,
    8, 2, 5,   2, 7, 5,   7,10, 2,  10,11, 7,   8, 5, 9
};

// ===================== Mesh objects =====================
static MeshData g_meshes[SHAPE_COUNT];
static Plane g_tetraPlanes[4], g_cubePlanes[12], g_octaPlanes[8];
static Plane g_dodecaPlanes[36], g_icosaPlanes[20];

// ===================== Globals =====================
HWND g_hWnd = NULL;
HDC g_hdc = NULL;
HGLRC g_hrc = NULL;

float g_cameraX = 0.0f, g_cameraY = 0.0f, g_cameraZ = 500.0f;
float g_cameraRotX = 0.0f, g_cameraRotY = 0.0f;

ShapeType g_shapeA = SHAPE_SPHERE;
ShapeType g_shapeB = SHAPE_CONE;

float g_shapeAScale = 100.0f;
float g_shapeBScale = 80.0f;
Vec3 g_shapeAPos = V3(0, 0, 0);
Vec3 g_shapeBPos = V3(0, 0, -250);

bool g_bRotateObjects = true;
float g_rotationSpeed = 0.5f;
float g_objectRotationX = 0.0f;
float g_objectRotationY = 0.0f;

bool g_mouseCaptured = false;
int g_lastMouseX = 0, g_lastMouseY = 0;

float g_animPhase = 0.0f;

#define IDT_TIMER1 1

// Menu IDs
#define IDM_SHAPEA_BASE   100
#define IDM_SHAPEB_BASE   200

// ===================== Forward declarations =====================
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void SetupPixelFormat(HDC hdc);
void RenderScene();
void DrawShapeMesh(ShapeType type, float scale);
void DrawShapeProcedural(ShapeType type, float scale);
void DrawShape(ShapeType type, Vec3 pos, float rotX, float rotY, float scale, bool isSecond);
bool IsPointInsideShape(ShapeType type, Vec3 p, Vec3 pos, float rotX, float rotY, float scale);
void InitMeshes();
void DrawIntersectionMarkers();
HMENU CreateAppMenu();
void UpdateWindowTitle();
void SampleSurfacePoints(ShapeType type, Vec3 pos, float rotX, float rotY, float scale,
                         Vec3* outPts, int maxPts, int& outCount);

// ===================== Rotation helpers =====================
static inline Vec3 RotateVec(Vec3 v, float rx, float ry) {
    float c1 = cosf(rx * PI / 180.0f), s1 = sinf(rx * PI / 180.0f);
    float c2 = cosf(ry * PI / 180.0f), s2 = sinf(ry * PI / 180.0f);
    // Ry
    float x1 = v.x * c2 + v.z * s2;
    float z1 = -v.x * s2 + v.z * c2;
    float y1 = v.y;
    // Rx
    float y2 = y1 * c1 - z1 * s1;
    float z2 = y1 * s1 + z1 * c1;
    return V3(x1, y2, z2);
}

static inline Vec3 TransformToWorld(Vec3 v, Vec3 pos, float rotX, float rotY, float scale) {
    Vec3 r = RotateVec(v, rotX, rotY);
    return V3(r.x * scale + pos.x, r.y * scale + pos.y, r.z * scale + pos.z);
}

static inline Vec3 TransformToLocal(Vec3 p, Vec3 pos, float rotX, float rotY, float scale) {
    Vec3 d = Sub(p, pos);
    float c1 = cosf(-rotX * PI / 180.0f), s1 = sinf(-rotX * PI / 180.0f);
    float c2 = cosf(-rotY * PI / 180.0f), s2 = sinf(-rotY * PI / 180.0f);
    // Inverse Rx
    float y1 = d.y * c1 - d.z * s1;
    float z1 = d.y * s1 + d.z * c1;
    float x1 = d.x;
    // Inverse Ry
    float x2 = x1 * c2 + z1 * s2;
    float z2 = -x1 * s2 + z1 * c2;
    return V3(x2 / scale, y1 / scale, z2 / scale);
}

// ===================== InitMeshes =====================
void InitMeshes() {
    // Normalize tetrahedron vertices
    for (int i = 0; i < 4; i++)
        s_tetraVerts[i] = Normalize(s_tetraVerts[i]);
    g_meshes[SHAPE_TETRAHEDRON].vertices = s_tetraVerts;
    g_meshes[SHAPE_TETRAHEDRON].numVertices = 4;
    g_meshes[SHAPE_TETRAHEDRON].indices = s_tetraFaces;
    g_meshes[SHAPE_TETRAHEDRON].numTriangles = 4;
    g_meshes[SHAPE_TETRAHEDRON].planes = g_tetraPlanes;

    // Normalize cube vertices
    for (int i = 0; i < 8; i++)
        s_cubeVerts[i] = Normalize(s_cubeVerts[i]);
    g_meshes[SHAPE_CUBE].vertices = s_cubeVerts;
    g_meshes[SHAPE_CUBE].numVertices = 8;
    g_meshes[SHAPE_CUBE].indices = s_cubeFaces;
    g_meshes[SHAPE_CUBE].numTriangles = 12;
    g_meshes[SHAPE_CUBE].planes = g_cubePlanes;

    // Octahedron vertices are already on unit sphere
    g_meshes[SHAPE_OCTAHEDRON].vertices = s_octaVerts;
    g_meshes[SHAPE_OCTAHEDRON].numVertices = 6;
    g_meshes[SHAPE_OCTAHEDRON].indices = s_octaFaces;
    g_meshes[SHAPE_OCTAHEDRON].numTriangles = 8;
    g_meshes[SHAPE_OCTAHEDRON].planes = g_octaPlanes;

    // Icosahedron - normalize
    for (int i = 0; i < 12; i++)
        s_icosaVerts[i] = Normalize(s_icosaVerts[i]);
    g_meshes[SHAPE_ICOSAHEDRON].vertices = s_icosaVerts;
    g_meshes[SHAPE_ICOSAHEDRON].numVertices = 12;
    g_meshes[SHAPE_ICOSAHEDRON].indices = s_icosaFaces;
    g_meshes[SHAPE_ICOSAHEDRON].numTriangles = 20;
    g_meshes[SHAPE_ICOSAHEDRON].planes = g_icosaPlanes;

    // Build dodecahedron programmatically using dual of icosahedron
    // The dodecahedron vertices = face centers of icosahedron
    // (after normalization, all on unit sphere)
    for (int i = 0; i < 20; i++) {
        Vec3 v0 = s_icosaVerts[s_icosaFaces[i*3]];
        Vec3 v1 = s_icosaVerts[s_icosaFaces[i*3+1]];
        Vec3 v2 = s_icosaVerts[s_icosaFaces[i*3+2]];
        Vec3 center = Scale(Add(Add(v0, v1), v2), 1.0f / 3.0f);
        s_dodecaFinalVerts[i] = Normalize(center);
    }

    // Build dodecahedron triangles by connecting adjacent face centers.
    // Two icosahedron faces sharing an edge produce adjacent dodecahedron vertices.
    // Each icosahedron vertex is shared by 5 faces, forming a pentagon of dodecahedron vertices.
    // For each icosahedron vertex, find the 5 faces that contain it,
    // and create a fan of triangles from those 5 dodecahedron vertices.

    s_dodecaNumTris = 0;
    for (int iv = 0; iv < 12; iv++) {
        int adjCount = 0;

        // We need faces in cyclic order around the vertex.
        // Use an angular sort around the vertex direction.
        Vec3 vDir = s_icosaVerts[iv];
        // Build orthonormal basis
        Vec3 up = (fabsf(vDir.y) < 0.9f) ? V3(0,1,0) : V3(1,0,0);
        Vec3 right = Normalize(Cross(up, vDir));
        Vec3 fwd = Cross(vDir, right);

        struct FaceAngle { int faceIdx; float angle; };
        FaceAngle fas[5];

        for (int f = 0; f < 20 && adjCount < 5; f++) {
            if (s_icosaFaces[f*3] == iv || s_icosaFaces[f*3+1] == iv || s_icosaFaces[f*3+2] == iv) {
                Vec3 fc = Scale(Add(Add(
                    s_icosaVerts[s_icosaFaces[f*3]],
                    s_icosaVerts[s_icosaFaces[f*3+1]]),
                    s_icosaVerts[s_icosaFaces[f*3+2]]), 1.0f/3.0f);
                float da = Dot(fc, right);
                float db = Dot(fc, fwd);
                fas[adjCount].faceIdx = f;
                fas[adjCount].angle = atan2f(db, da);
                adjCount++;
            }
        }

        // Sort by angle
        for (int a = 0; a < adjCount - 1; a++)
            for (int b = a + 1; b < adjCount; b++)
                if (fas[a].angle > fas[b].angle) {
                    FaceAngle tmp = fas[a]; fas[a] = fas[b]; fas[b] = tmp;
                }

        // Create triangle fan from first dodecahedron vertex
        if (adjCount >= 3) {
            int base = fas[0].faceIdx;
            for (int k = 1; k < adjCount - 1; k++) {
                s_dodecaFinalIndices[s_dodecaNumTris * 3 + 0] = base;
                s_dodecaFinalIndices[s_dodecaNumTris * 3 + 1] = fas[k].faceIdx;
                s_dodecaFinalIndices[s_dodecaNumTris * 3 + 2] = fas[k+1].faceIdx;
                s_dodecaNumTris++;
            }
        }
    }

    g_meshes[SHAPE_DODECAHEDRON].vertices = s_dodecaFinalVerts;
    g_meshes[SHAPE_DODECAHEDRON].numVertices = 20;
    g_meshes[SHAPE_DODECAHEDRON].indices = s_dodecaFinalIndices;
    g_meshes[SHAPE_DODECAHEDRON].numTriangles = s_dodecaNumTris;
    g_meshes[SHAPE_DODECAHEDRON].planes = g_dodecaPlanes;

    // Precompute planes for all polyhedra
    for (int s = 0; s < SHAPE_COUNT; s++) {
        if (s == SHAPE_SPHERE || s == SHAPE_CONE) continue; // special handling
        MeshData& m = g_meshes[s];
        for (int t = 0; t < m.numTriangles; t++) {
            Vec3 v0 = m.vertices[m.indices[t*3]];
            Vec3 v1 = m.vertices[m.indices[t*3+1]];
            Vec3 v2 = m.vertices[m.indices[t*3+2]];
            Vec3 n = Normalize(Cross(Sub(v1, v0), Sub(v2, v0)));
            Vec3 c = Scale(Add(Add(v0, v1), v2), 1.0f / 3.0f);
            if (Dot(n, c) < 0) n = Scale(n, -1.0f); // ensure outward normal
            m.planes[t].nx = n.x;
            m.planes[t].ny = n.y;
            m.planes[t].nz = n.z;
            m.planes[t].d = -Dot(n, v0);
        }
    }
}

// ===================== Drawing =====================
void DrawShapeMesh(ShapeType type, float scale) {
    MeshData& m = g_meshes[type];
    glBegin(GL_TRIANGLES);
    for (int t = 0; t < m.numTriangles; t++) {
        Vec3 v0 = m.vertices[m.indices[t*3]];
        Vec3 v1 = m.vertices[m.indices[t*3+1]];
        Vec3 v2 = m.vertices[m.indices[t*3+2]];
        Vec3 n = Normalize(Cross(Sub(v1, v0), Sub(v2, v0)));
        if (Dot(n, v0) < 0) n = Scale(n, -1.0f);
        glNormal3f(n.x, n.y, n.z);
        glVertex3f(v0.x * scale, v0.y * scale, v0.z * scale);
        glVertex3f(v1.x * scale, v1.y * scale, v1.z * scale);
        glVertex3f(v2.x * scale, v2.y * scale, v2.z * scale);
    }
    glEnd();

    // Draw wireframe overlay
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDisable(GL_LIGHTING);
    glBegin(GL_TRIANGLES);
    for (int t = 0; t < m.numTriangles; t++) {
        Vec3 v0 = m.vertices[m.indices[t*3]];
        Vec3 v1 = m.vertices[m.indices[t*3+1]];
        Vec3 v2 = m.vertices[m.indices[t*3+2]];
        glVertex3f(v0.x * scale, v0.y * scale, v0.z * scale);
        glVertex3f(v1.x * scale, v1.y * scale, v1.z * scale);
        glVertex3f(v2.x * scale, v2.y * scale, v2.z * scale);
    }
    glEnd();
    glEnable(GL_LIGHTING);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void DrawShapeProcedural(ShapeType type, float scale) {
    int slices = 32, stacks = 24;

    if (type == SHAPE_SPHERE) {
        for (int i = 0; i < stacks; i++) {
            float phi1 = i * PI / stacks;
            float phi2 = (i + 1) * PI / stacks;
            glBegin(GL_TRIANGLE_STRIP);
            for (int j = 0; j <= slices; j++) {
                float theta = j * 2.0f * PI / slices;
                float x = cosf(phi1) * cosf(theta);
                float y = cosf(phi1) * sinf(theta);
                float z = sinf(phi1);
                glNormal3f(x, y, z); // simplified
                glVertex3f(x * scale, z * scale, y * scale);

                x = cosf(phi2) * cosf(theta);
                y = cosf(phi2) * sinf(theta);
                z = sinf(phi2);
                glNormal3f(x, y, z);
                glVertex3f(x * scale, z * scale, y * scale);
            }
            glEnd();
        }

        // Wireframe
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_LIGHTING);
        for (int i = 0; i < stacks; i += 4) {
            float phi = i * PI / stacks;
            glBegin(GL_LINE_LOOP);
            for (int j = 0; j < slices; j++) {
                float theta = j * 2.0f * PI / slices;
                glVertex3f(cosf(phi)*cosf(theta)*scale, sinf(phi)*scale, cosf(phi)*sinf(theta)*scale);
            }
            glEnd();
        }
        glEnable(GL_LIGHTING);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    else if (type == SHAPE_CONE) {
        float baseR = scale * 0.8f;
        float height = scale * 2.0f;
        // Side surface
        glBegin(GL_TRIANGLE_FAN);
        glNormal3f(0, 1, 0);
        glVertex3f(0, height * 0.5f, 0); // apex
        for (int i = 0; i <= slices; i++) {
            float theta = i * 2.0f * PI / slices;
            float x = baseR * cosf(theta);
            float z = baseR * sinf(theta);
            glNormal3f(cosf(theta), 0.3f, sinf(theta));
            glVertex3f(x, -height * 0.5f, z);
        }
        glEnd();

        // Base disc
        glBegin(GL_TRIANGLE_FAN);
        glNormal3f(0, -1, 0);
        glVertex3f(0, -height * 0.5f, 0);
        for (int i = slices; i >= 0; i--) {
            float theta = i * 2.0f * PI / slices;
            glVertex3f(baseR * cosf(theta), -height * 0.5f, baseR * sinf(theta));
        }
        glEnd();

        // Wireframe
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_LIGHTING);
        glBegin(GL_LINES);
        for (int i = 0; i < slices; i += 4) {
            float theta = i * 2.0f * PI / slices;
            glVertex3f(0, height * 0.5f, 0);
            glVertex3f(baseR * cosf(theta), -height * 0.5f, baseR * sinf(theta));
        }
        glEnd();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_LIGHTING);
    }
}

void DrawShape(ShapeType type, Vec3 pos, float rotX, float rotY, float scale, bool isSecond) {
    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);
    glRotatef(rotX, 1.0f, 0.0f, 0.0f);
    glRotatef(rotY, 0.0f, 1.0f, 0.0f);

    if (isSecond) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        // Set color with alpha
        switch (type) {
            case SHAPE_SPHERE:       glColor4f(0.0f, 0.6f, 0.9f, 0.6f); break;
            case SHAPE_CONE:         glColor4f(0.9f, 0.5f, 0.1f, 0.6f); break;
            case SHAPE_CUBE:         glColor4f(0.2f, 0.8f, 0.2f, 0.6f); break;
            case SHAPE_OCTAHEDRON:   glColor4f(0.8f, 0.2f, 0.8f, 0.6f); break;
            case SHAPE_DODECAHEDRON: glColor4f(0.9f, 0.8f, 0.1f, 0.6f); break;
            case SHAPE_TETRAHEDRON:  glColor4f(0.1f, 0.9f, 0.9f, 0.6f); break;
            case SHAPE_ICOSAHEDRON:  glColor4f(0.9f, 0.3f, 0.5f, 0.6f); break;
            default:                 glColor4f(0.5f, 0.5f, 0.5f, 0.6f); break;
        }
    } else {
        switch (type) {
            case SHAPE_SPHERE:       glColor3f(0.0f, 0.5f, 0.8f); break;
            case SHAPE_CONE:         glColor3f(0.8f, 0.4f, 0.0f); break;
            case SHAPE_CUBE:         glColor3f(0.2f, 0.7f, 0.2f); break;
            case SHAPE_OCTAHEDRON:   glColor3f(0.7f, 0.2f, 0.7f); break;
            case SHAPE_DODECAHEDRON: glColor3f(0.8f, 0.7f, 0.1f); break;
            case SHAPE_TETRAHEDRON:  glColor3f(0.1f, 0.8f, 0.8f); break;
            case SHAPE_ICOSAHEDRON:  glColor3f(0.8f, 0.3f, 0.4f); break;
            default:                 glColor3f(0.5f, 0.5f, 0.5f); break;
        }
    }

    if (type == SHAPE_SPHERE || type == SHAPE_CONE) {
        DrawShapeProcedural(type, scale);
    } else {
        DrawShapeMesh(type, scale);
    }

    if (isSecond) {
        glDisable(GL_BLEND);
    }

    glPopMatrix();
}

// ===================== Point-in-shape tests =====================
bool IsPointInsidePolyhedron(Vec3 p, MeshData& m) {
    for (int i = 0; i < m.numTriangles; i++) {
        float test = m.planes[i].nx * p.x + m.planes[i].ny * p.y + m.planes[i].nz * p.z + m.planes[i].d;
        if (test > 0.001f) return false;
    }
    return true;
}

bool IsPointInsideShape(ShapeType type, Vec3 p, Vec3 pos, float rotX, float rotY, float scale) {
    // Transform point to shape's local space (unit sphere)
    Vec3 local = TransformToLocal(p, pos, rotX, rotY, scale);

    if (type == SHAPE_SPHERE) {
        return Length(local) <= 1.0f;
    }
    else if (type == SHAPE_CONE) {
        // Cone: apex at y=0.5, base at y=-0.5, base radius = 0.8
        float halfH = 1.0f;
        float baseR = 0.8f;
        float localY = local.y;
        if (localY > halfH * 0.5f || localY < -halfH * 0.5f) return false;
        float frac = (halfH * 0.5f - localY) / halfH; // 0 at apex, 1 at base
        float allowedR = baseR * frac;
        float r = sqrtf(local.x * local.x + local.z * local.z);
        return r <= allowedR;
    }
    else {
        return IsPointInsidePolyhedron(local, g_meshes[type]);
    }
}

// ===================== Surface sampling =====================
void SampleSurfacePoints(ShapeType type, Vec3 pos, float rotX, float rotY, float scale,
                         Vec3* outPts, int maxPts, int& outCount) {
    outCount = 0;

    if (type == SHAPE_SPHERE) {
        int n = (int)sqrtf((float)maxPts);
        for (int i = 0; i < n && outCount < maxPts; i++) {
            float phi = PI * i / (n - 1);
            for (int j = 0; j < n && outCount < maxPts; j++) {
                float theta = 2.0f * PI * j / (n - 1);
                Vec3 local = V3(cosf(phi)*cosf(theta), sinf(phi), cosf(phi)*sinf(theta));
                outPts[outCount++] = TransformToWorld(local, pos, rotX, rotY, scale);
            }
        }
    }
    else if (type == SHAPE_CONE) {
        int n = (int)sqrtf((float)maxPts);
        float halfH = 1.0f, baseR = 0.8f;
        // Sample side surface
        for (int i = 0; i < n && outCount < maxPts; i++) {
            float frac = (float)i / (n - 1);
            float y = halfH * 0.5f - frac * halfH;
            float r = baseR * frac;
            for (int j = 0; j < n && outCount < maxPts; j++) {
                float theta = 2.0f * PI * j / (n - 1);
                Vec3 local = V3(r * cosf(theta), y, r * sinf(theta));
                outPts[outCount++] = TransformToWorld(local, pos, rotX, rotY, scale);
            }
        }
    }
    else {
        // Sample on triangle faces
        MeshData& m = g_meshes[type];
        int perTri = maxPts / m.numTriangles;
        if (perTri < 1) perTri = 1;
        int n = (int)sqrtf((float)perTri * 2);
        if (n < 2) n = 2;

        for (int t = 0; t < m.numTriangles && outCount < maxPts; t++) {
            Vec3 v0 = m.vertices[m.indices[t*3]];
            Vec3 v1 = m.vertices[m.indices[t*3+1]];
            Vec3 v2 = m.vertices[m.indices[t*3+2]];
            for (int i = 0; i <= n && outCount < maxPts; i++) {
                for (int j = 0; j <= n - i && outCount < maxPts; j++) {
                    float u = (float)i / n;
                    float v = (float)j / n;
                    float w = 1.0f - u - v;
                    Vec3 local = Add(Add(Scale(v0, w), Scale(v1, u)), Scale(v2, v));
                    outPts[outCount++] = TransformToWorld(local, pos, rotX, rotY, scale);
                }
            }
        }
    }
}

// ===================== Intersection markers =====================
void DrawIntersectionMarkers() {
    static Vec3 ptsA[4000], ptsB[4000];
    int countA = 0, countB = 0;

    float bRotX = g_objectRotationX;
    float bRotY = g_objectRotationY;
    float aRotX = g_objectRotationX;
    float aRotY = g_objectRotationY;

    SampleSurfacePoints(g_shapeA, g_shapeAPos, aRotX, aRotY, g_shapeAScale, ptsA, 4000, countA);
    SampleSurfacePoints(g_shapeB, g_shapeBPos, bRotX, bRotY, g_shapeBScale, ptsB, 4000, countB);

    glDisable(GL_LIGHTING);

    // Points of A inside B - magenta
    if (countA > 0) {
        glColor3f(1.0f, 0.0f, 1.0f);
        glPointSize(3.0f);
        glBegin(GL_POINTS);
        for (int i = 0; i < countA; i++) {
            if (IsPointInsideShape(g_shapeB, ptsA[i], g_shapeBPos, bRotX, bRotY, g_shapeBScale)) {
                glVertex3f(ptsA[i].x, ptsA[i].y, ptsA[i].z);
            }
        }
        glEnd();
    }

    // Points of B inside A - yellow
    if (countB > 0) {
        glColor3f(1.0f, 1.0f, 0.0f);
        glPointSize(3.0f);
        glBegin(GL_POINTS);
        for (int i = 0; i < countB; i++) {
            if (IsPointInsideShape(g_shapeA, ptsB[i], g_shapeAPos, aRotX, aRotY, g_shapeAScale)) {
                glVertex3f(ptsB[i].x, ptsB[i].y, ptsB[i].z);
            }
        }
        glEnd();
    }

    glEnable(GL_LIGHTING);
}

// ===================== Coordinate axes =====================
void DrawAxes() {
    glDisable(GL_LIGHTING);
    glBegin(GL_LINES);
    glColor3f(1,0,0); glVertex3f(0,0,0); glVertex3f(200,0,0);
    glColor3f(0,1,0); glVertex3f(0,0,0); glVertex3f(0,200,0);
    glColor3f(0,0,1); glVertex3f(0,0,0); glVertex3f(0,0,200);
    glEnd();
    glEnable(GL_LIGHTING);
}

// ===================== Render =====================
void RenderScene() {
    DrawAxes();

    float aRotX = g_objectRotationX;
    float aRotY = g_objectRotationY;
    float bRotX = g_objectRotationX;
    float bRotY = g_objectRotationY;

    // Shape A (fixed at origin)
    DrawShape(g_shapeA, g_shapeAPos, aRotX, aRotY, g_shapeAScale, false);

    // Shape B (moving along Z axis)
    DrawShape(g_shapeB, g_shapeBPos, bRotX, bRotY, g_shapeBScale, true);

    // Intersection markers
    DrawIntersectionMarkers();
}

// ===================== Menu =====================
HMENU CreateAppMenu() {
    HMENU hMenuBar = CreateMenu();

    // Shape A submenu
    HMENU hShapeA = CreatePopupMenu();
    for (int i = 0; i < SHAPE_COUNT; i++) {
        UINT flags = MF_STRING;
        if ((ShapeType)i == g_shapeA) flags |= MF_CHECKED;
        AppendMenu(hShapeA, flags, IDM_SHAPEA_BASE + i, ShapeNames[i]);
    }

    // Shape B submenu
    HMENU hShapeB = CreatePopupMenu();
    for (int i = 0; i < SHAPE_COUNT; i++) {
        UINT flags = MF_STRING;
        if ((ShapeType)i == g_shapeB) flags |= MF_CHECKED;
        AppendMenu(hShapeB, flags, IDM_SHAPEB_BASE + i, ShapeNames[i]);
    }

    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hShapeA, L"Shape A");
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hShapeB, L"Shape B");

    // Help
    HMENU hHelp = CreatePopupMenu();
    AppendMenu(hHelp, MF_STRING, 300, L"Controls");
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hHelp, L"Help");

    return hMenuBar;
}

void UpdateWindowTitle() {
    wchar_t title[256];
    swprintf(title, 256, L"%s - %s Intersection (3D OpenGL)",
             ShapeNames[g_shapeA], ShapeNames[g_shapeB]);
    SetWindowText(g_hWnd, title);
}

void RefreshMenu() {
    HMENU hMenu = CreateAppMenu();
    SetMenu(g_hWnd, hMenu);
    DrawMenuBar(g_hWnd);
}

// ===================== Main =====================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEX wc = {0};
    MSG msg;

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"ShapeIntersection3D";
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, L"Window Registration Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    g_hWnd = CreateWindowEx(0, L"ShapeIntersection3D",
        L"Sphere - Cone Intersection (3D OpenGL)",
        WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 1024, 768,
        NULL, NULL, hInstance, NULL);

    if (!g_hWnd) {
        MessageBox(NULL, L"Window Creation Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_hdc = GetDC(hwnd);
        SetupPixelFormat(g_hdc);
        g_hrc = wglCreateContext(g_hdc);
        wglMakeCurrent(g_hdc, g_hrc);

        glClearColor(0.8f, 0.9f, 1.0f, 1.0f);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

        GLfloat lightPos[] = { 200.0f, 300.0f, 200.0f, 1.0f };
        GLfloat lightAmb[] = { 0.3f, 0.3f, 0.3f, 1.0f };
        GLfloat lightDif[] = { 0.8f, 0.8f, 0.8f, 1.0f };
        glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
        glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmb);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDif);

        InitMeshes();

        SetTimer(hwnd, IDT_TIMER1, 16, NULL);

        SetMenu(hwnd, CreateAppMenu());
        UpdateWindowTitle();

        RECT rect;
        GetClientRect(hwnd, &rect);
        g_lastMouseX = (rect.right - rect.left) / 2;
        g_lastMouseY = (rect.bottom - rect.top) / 2;
        return 0;
    }

    case WM_DESTROY:
        KillTimer(hwnd, IDT_TIMER1);
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(g_hrc);
        ReleaseDC(hwnd, g_hdc);
        PostQuitMessage(0);
        return 0;

    case WM_SIZE: {
        int w = LOWORD(lParam), h = HIWORD(lParam);
        if (h == 0) h = 1;
        glViewport(0, 0, w, h);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(45.0f, (float)w / h, 1.0f, 10000.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        return 0;
    }

    case WM_KEYDOWN: {
        switch (wParam) {
            case 'W': g_cameraZ -= 5.0f; break;
            case 'S': g_cameraZ += 5.0f; break;
            case 'A': g_cameraX -= 5.0f; break;
            case 'D': g_cameraX += 5.0f; break;
            case 'Q': g_cameraY += 5.0f; break;
            case 'E': g_cameraY -= 5.0f; break;
            case 'R':
                g_cameraX = g_cameraY = 0; g_cameraZ = 500.0f;
                g_cameraRotX = g_cameraRotY = 0;
                break;
            case 'T': g_bRotateObjects = !g_bRotateObjects; break;
            case '1':
                g_shapeA = SHAPE_SPHERE; g_shapeB = SHAPE_CONE;
                RefreshMenu(); UpdateWindowTitle(); break;
            case '2':
                g_shapeA = SHAPE_CUBE; g_shapeB = SHAPE_OCTAHEDRON;
                RefreshMenu(); UpdateWindowTitle(); break;
            case '3':
                g_shapeA = SHAPE_DODECAHEDRON; g_shapeB = SHAPE_TETRAHEDRON;
                RefreshMenu(); UpdateWindowTitle(); break;
            case '4':
                g_shapeA = SHAPE_ICOSAHEDRON; g_shapeB = SHAPE_SPHERE;
                RefreshMenu(); UpdateWindowTitle(); break;
        }
        return 0;
    }

    case WM_LBUTTONDOWN:
        g_mouseCaptured = true;
        SetCapture(hwnd);
        return 0;

    case WM_LBUTTONUP:
        g_mouseCaptured = false;
        ReleaseCapture();
        return 0;

    case WM_MOUSEMOVE: {
        if (g_mouseCaptured) {
            int mx = LOWORD(lParam), my = HIWORD(lParam);
            int dx = mx - g_lastMouseX, dy = my - g_lastMouseY;
            g_cameraRotY += dx * 0.5f;
            g_cameraRotX += dy * 0.5f;
            if (g_cameraRotX > 89) g_cameraRotX = 89;
            if (g_cameraRotX < -89) g_cameraRotX = -89;
            RECT rect;
            GetClientRect(hwnd, &rect);
            g_lastMouseX = (rect.right - rect.left) / 2;
            g_lastMouseY = (rect.bottom - rect.top) / 2;
            SetCursorPos(g_lastMouseX, g_lastMouseY);
        }
        return 0;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id >= IDM_SHAPEA_BASE && id < IDM_SHAPEA_BASE + SHAPE_COUNT) {
            g_shapeA = (ShapeType)(id - IDM_SHAPEA_BASE);
            RefreshMenu(); UpdateWindowTitle();
        }
        else if (id >= IDM_SHAPEB_BASE && id < IDM_SHAPEB_BASE + SHAPE_COUNT) {
            g_shapeB = (ShapeType)(id - IDM_SHAPEB_BASE);
            RefreshMenu(); UpdateWindowTitle();
        }
        else if (id == 300) {
            MessageBox(hwnd,
                L"Camera: WASD = move, QE = up/down, R = reset\n"
                L"Mouse: left-drag = orbit\n"
                L"T = toggle rotation\n"
                L"1 = Sphere+Cone  2 = Cube+Octahedron\n"
                L"3 = Dodecahedron+Tetrahedron  4 = Icosahedron+Sphere\n"
                L"Menu: Shape A / Shape B to pick any combination",
                L"Controls", MB_OK | MB_ICONINFORMATION);
        }
        return 0;
    }

    case WM_TIMER:
        if (wParam == IDT_TIMER1) {
            if (g_bRotateObjects) {
                g_objectRotationX += g_rotationSpeed;
                g_objectRotationY += g_rotationSpeed * 0.7f;
            }
            // Animate shape B along Z axis
            g_animPhase += 3.0f;
            g_shapeBPos.z = 250.0f * sinf(g_animPhase * PI / 180.0f);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();
        glRotatef(g_cameraRotX, 1, 0, 0);
        glRotatef(g_cameraRotY, 0, 1, 0);
        glTranslatef(-g_cameraX, -g_cameraY, -g_cameraZ);
        RenderScene();
        SwapBuffers(g_hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void SetupPixelFormat(HDC hdc) {
    static PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR), 1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA, 32,
        0,0,0,0,0,0, 0,0,
        0, 0,0,0,0,
        24, 8, 0,
        PFD_MAIN_PLANE, 0, 0,0,0
    };
    int pf = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pf, &pfd);
}
