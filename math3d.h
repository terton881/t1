#pragma once
// math3d.h - Minimal 3D math for modern shader-based renderer
#include <math.h>

#define M3D_PI 3.14159265358979323846f
#define M3D_DEG2RAD(x) ((x) * M3D_PI / 180.0f)
#define M3D_RAD2DEG(x) ((x) * 180.0f / M3D_PI)

struct Vec3 {
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float v) : x(v), y(v), z(v) {}
    Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
};

inline Vec3 operator+(Vec3 a, Vec3 b) { return Vec3(a.x + b.x, a.y + b.y, a.z + b.z); }
inline Vec3 operator-(Vec3 a, Vec3 b) { return Vec3(a.x - b.x, a.y - b.y, a.z - b.z); }
inline Vec3 operator*(Vec3 a, float s) { return Vec3(a.x * s, a.y * s, a.z * s); }
inline Vec3 operator*(float s, Vec3 a) { return a * s; }
inline Vec3 operator/(Vec3 a, float s) { float inv = 1.0f / s; return a * inv; }
inline Vec3 operator*(Vec3 a, Vec3 b) { return Vec3(a.x * b.x, a.y * b.y, a.z * b.z); }

inline float Dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 Cross(Vec3 a, Vec3 b) {
    return Vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}
inline float Length(Vec3 a) { return sqrtf(Dot(a, a)); }
inline Vec3 Normalize(Vec3 a) {
    float l = Length(a);
    if (l < 1e-12f) return Vec3(0, 0, 0);
    return a * (1.0f / l);
}
inline Vec3 Lerp(Vec3 a, Vec3 b, float t) { return a + (b - a) * t; }

struct Vec2 {
    float x, y;
    Vec2() : x(0), y(0) {}
    Vec2(float _x, float _y) : x(_x), y(_y) {}
};

inline Vec2 operator+(Vec2 a, Vec2 b) { return Vec2(a.x + b.x, a.y + b.y); }
inline Vec2 operator-(Vec2 a, Vec2 b) { return Vec2(a.x - b.x, a.y - b.y); }
inline Vec2 operator*(Vec2 a, float s) { return Vec2(a.x * s, a.y * s); }
inline float Dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }

struct Rect2D {
    float x = 0, y = 0, w = 0, h = 0;
    Rect2D() = default;
    Rect2D(float _x, float _y, float _w, float _h) : x(_x), y(_y), w(_w), h(_h) {}
};

struct Vec4 {
    float x, y, z, w;
    Vec4() : x(0), y(0), z(0), w(0) {}
    Vec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
    Vec4(Vec3 v, float _w) : x(v.x), y(v.y), z(v.z), w(_w) {}
};

struct Mat4 {
    float m[16]; // column-major for GLSL
    Mat4() { Identity(); }
    void Identity() {
        for (int i = 0; i < 16; i++) m[i] = 0.0f;
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }
};

inline Mat4 Mul(const Mat4& a, const Mat4& b) {
    Mat4 r;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            float s = 0.0f;
            for (int k = 0; k < 4; k++) s += a.m[k*4 + i] * b.m[j*4 + k];
            r.m[j*4 + i] = s;
        }
    }
    return r;
}

inline Vec4 Mul(const Mat4& a, Vec4 v) {
    Vec4 r;
    r.x = a.m[0] * v.x + a.m[4] * v.y + a.m[8] * v.z + a.m[12] * v.w;
    r.y = a.m[1] * v.x + a.m[5] * v.y + a.m[9] * v.z + a.m[13] * v.w;
    r.z = a.m[2] * v.x + a.m[6] * v.y + a.m[10] * v.z + a.m[14] * v.w;
    r.w = a.m[3] * v.x + a.m[7] * v.y + a.m[11] * v.z + a.m[15] * v.w;
    return r;
}

inline Vec3 MulPoint(const Mat4& a, Vec3 p) {
    Vec4 v = Mul(a, Vec4(p, 1.0f));
    if (fabsf(v.w) > 1e-12f) return Vec3(v.x / v.w, v.y / v.w, v.z / v.w);
    return Vec3(v.x, v.y, v.z);
}

inline Vec3 MulDir(const Mat4& a, Vec3 d) {
    Vec4 v = Mul(a, Vec4(d, 0.0f));
    return Vec3(v.x, v.y, v.z);
}

inline Mat4 Translate(Vec3 t) {
    Mat4 r; r.Identity();
    r.m[12] = t.x; r.m[13] = t.y; r.m[14] = t.z;
    return r;
}

inline Mat4 Scale(Vec3 s) {
    Mat4 r; r.Identity();
    r.m[0] = s.x; r.m[5] = s.y; r.m[10] = s.z;
    return r;
}

inline Mat4 RotateX(float rad) {
    Mat4 r; r.Identity();
    float c = cosf(rad), s = sinf(rad);
    r.m[5] = c; r.m[6] = s; r.m[9] = -s; r.m[10] = c;
    return r;
}
inline Mat4 RotateY(float rad) {
    Mat4 r; r.Identity();
    float c = cosf(rad), s = sinf(rad);
    r.m[0] = c; r.m[2] = -s; r.m[8] = s; r.m[10] = c;
    return r;
}
inline Mat4 RotateZ(float rad) {
    Mat4 r; r.Identity();
    float c = cosf(rad), s = sinf(rad);
    r.m[0] = c; r.m[1] = s; r.m[4] = -s; r.m[5] = c;
    return r;
}

inline Mat4 EulerToMat(Vec3 eulerDeg) {
    float rx = M3D_DEG2RAD(eulerDeg.x);
    float ry = M3D_DEG2RAD(eulerDeg.y);
    float rz = M3D_DEG2RAD(eulerDeg.z);
    return Mul(Mul(RotateZ(rz), RotateY(ry)), RotateX(rx));
}

inline Mat4 ModelMatrix(Vec3 pos, Vec3 eulerDeg, Vec3 scale3) {
    Mat4 T = Translate(pos);
    Mat4 R = EulerToMat(eulerDeg);
    Mat4 S = Scale(scale3);
    return Mul(T, Mul(R, S));
}

inline Mat4 Perspective(float fovyRad, float aspect, float znear, float zfar) {
    Mat4 r; r.Identity();
    float f = 1.0f / tanf(fovyRad * 0.5f);
    r.m[0] = f / aspect;
    r.m[5] = f;
    r.m[10] = (zfar + znear) / (znear - zfar);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * zfar * znear) / (znear - zfar);
    r.m[15] = 0.0f;
    return r;
}

inline Mat4 LookAt(Vec3 eye, Vec3 center, Vec3 up) {
    Vec3 f = Normalize(center - eye);
    Vec3 s = Normalize(Cross(f, up));
    Vec3 u = Cross(s, f);
    Mat4 r; r.Identity();
    r.m[0] = s.x; r.m[4] = s.y; r.m[8] = s.z;
    r.m[1] = u.x; r.m[5] = u.y; r.m[9] = u.z;
    r.m[2] = -f.x; r.m[6] = -f.y; r.m[10] = -f.z;
    r.m[12] = -Dot(s, eye);
    r.m[13] = -Dot(u, eye);
    r.m[14] = Dot(f, eye);
    return r;
}

// Orbit camera helper
struct OrbitCamera {
    Vec3 target = Vec3(0, 0, 0);
    float yaw = 25.0f;   // degrees
    float pitch = 20.0f;
    float distance = 450.0f;
    float fov = 45.0f;
    float aspect = 16.0f / 9.0f;
    float znear = 1.0f;
    float zfar = 10000.0f;

    void Pan(float dx, float dy) {
        // simple world pan approx
        float moveScale = distance * 0.0015f;
        Vec3 right = Vec3(cosf(M3D_DEG2RAD(yaw)), 0, sinf(M3D_DEG2RAD(yaw)));
        Vec3 fwd = Vec3(-sinf(M3D_DEG2RAD(yaw)), 0, cosf(M3D_DEG2RAD(yaw)));
        target = target + right * (-dx * moveScale) + Vec3(0,1,0) * (dy * moveScale);
    }

    void Orbit(float dYaw, float dPitch) {
        yaw += dYaw;
        pitch += dPitch;
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
    }

    void Zoom(float d) {
        distance *= (1.0f + d * 0.1f);
        if (distance < 5.0f) distance = 5.0f;
        if (distance > 8000.0f) distance = 8000.0f;
    }

    Vec3 GetEye() const {
        float py = M3D_DEG2RAD(pitch);
        float yw = M3D_DEG2RAD(yaw);
        float cy = cosf(py), sy = sinf(py);
        float cyw = cosf(yw), syw = sinf(yw);
        Vec3 dir = Vec3(syw * cy, sy, cyw * cy);
        return target - dir * distance;
    }

    Mat4 GetView() const {
        Vec3 eye = GetEye();
        Vec3 up = Vec3(0, 1, 0);
        if (fabsf(pitch) > 89.5f) up = Vec3(0,0,1); // avoid singularity
        return LookAt(eye, target, up);
    }

    Mat4 GetProj() const {
        return Perspective(M3D_DEG2RAD(fov), aspect, znear, zfar);
    }
};
