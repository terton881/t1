#include "renderer.h"
#include "gui.h"
#include <vector>
#include <stdio.h>
#include <string.h>

static const char* kVertShape = R"GLSL(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;

uniform mat4 uModel;
uniform mat4 uViewProj;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;

void main() {
    vec4 wp = uModel * vec4(aPos, 1.0);
    vWorldPos = wp.xyz;
    vNormal = mat3(uModel) * aNormal;
    vUV = aUV;
    gl_Position = uViewProj * wp;
}
)GLSL";

static const char* kFragShape = R"GLSL(
#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;

uniform vec3 uColor;
uniform float uAlpha;
uniform int uShapeType;
uniform int uNumPlanes;
uniform vec4 uPlanes[40];
uniform vec3 uExtra;      // .x baseR/halfH etc
uniform vec3 uLightDir;

uniform int uOtherType;
uniform int uOtherNumPlanes;
uniform vec4 uOtherPlanes[40];
uniform vec3 uOtherExtra;

uniform vec3 uOtherPos;
uniform vec3 uOtherEuler;
uniform vec3 uOtherScale;

out vec4 FragColor;

// --- Local space transform helpers (match CPU) ---
vec3 RotateEuler(vec3 p, vec3 eulerDeg) {
    float rx = radians(eulerDeg.x), ry = radians(eulerDeg.y), rz = radians(eulerDeg.z);
    float cx=cos(rx),sx=sin(rx), cy=cos(ry),sy=sin(ry), cz=cos(rz),sz=sin(rz);
    // Rz * Ry * Rx
    float x1 = p.x, y1 = p.y * cx - p.z * sx, z1 = p.y * sx + p.z * cx;
    float x2 = x1 * cy + z1 * sy, y2 = y1, z2 = -x1 * sy + z1 * cy;
    float x3 = x2 * cz - y2 * sz, y3 = x2 * sz + y2 * cz, z3 = z2;
    return vec3(x3, y3, z3);
}

vec3 WorldToLocal(vec3 wp, vec3 pos, vec3 eulerDeg, vec3 scl) {
    vec3 d = wp - pos;
    d /= scl; // non-uniform ok for inside tests if consistent
    return RotateEuler(d, vec3(-eulerDeg.x, -eulerDeg.y, -eulerDeg.z));
}

bool PointInAnalytic(int typ, vec3 lp, vec3 extra) {
    if (typ == 0) return dot(lp, lp) <= 1.0002; // sphere
    if (typ == 1) { // cone
        float br = extra.x, hh = extra.y;
        if (lp.y > hh || lp.y < -hh) return false;
        float frac = (hh - lp.y) / (2.0 * hh);
        float rr = sqrt(lp.x*lp.x + lp.z*lp.z);
        return rr <= br * frac + 0.0005;
    }
    if (typ == 2) { // box
        return abs(lp.x) <= extra.x+0.0005 && abs(lp.y)<=extra.y+0.0005 && abs(lp.z)<=extra.z+0.0005;
    }
    if (typ == 3) { // cyl
        float rr = extra.x, hh=extra.y;
        if (lp.y < -hh || lp.y > hh) return false;
        return (lp.x*lp.x + lp.z*lp.z) <= rr*rr + 0.0005;
    }
    if (typ == 8) { // torus
        float R = extra.x, r = extra.y;
        float q = sqrt(lp.x*lp.x + lp.z*lp.z) - R;
        return q*q + lp.y*lp.y <= r*r + 0.0005;
    }
    if (typ == 9) { // capsule (r, halfH)
        float r = extra.x, hh = extra.y;
        float y = lp.y;
        if (y > hh) y = hh; else if (y < -hh) y = -hh;
        float dx = lp.x, dy = lp.y - y, dz = lp.z;
        return (dx*dx + dy*dy + dz*dz) <= (r*r + 0.0005);
    }
    return true;
}

bool PointInPlanes(vec3 lp, int n, vec4 pls[40]) {
    for (int i = 0; i < n; ++i) {
        vec4 pl = pls[i];
        if (dot(vec3(pl.xyz), lp) + pl.w > 0.001) return false;
    }
    return true;
}

bool IsInsideOther(vec3 wp, int otyp, vec3 opos, vec3 oeul, vec3 oscl, int onp, vec4 opls[40], vec3 oextra) {
    vec3 lp = WorldToLocal(wp, opos, oeul, oscl);
    if (otyp <= 3 || otyp == 8) return PointInAnalytic(otyp, lp, oextra);
    return PointInPlanes(lp, onp, opls);
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightDir);
    vec3 V = normalize(-vWorldPos); // rough, camera not passed but ok for viz
    float ndl = max(0.0, dot(N, L));
    float spec = pow(max(0.0, dot(reflect(-L, N), V)), 24.0) * 0.6;

    vec3 base = uColor;
    float inside = 0.0;

    if (uOtherType >= 0 && uAlpha > 0.1) {
        // To test "inside other" we need the other instance params.
        // For simplicity in this pass we assume uniforms for other are set when drawing "first".
        // A better system would use UBO per pair.
        // Here we rely on caller to set uOther* when appropriate.
        // We approximate by using a global test flag.
        // For full power the app sets the other transform + planes before draw.
        // Placeholder: if highlightIntersect we tint a bit based on heuristic (will be driven by app uniforms)
    }

    // If we have other data bound, compute per pixel inside test (powerful shader feature!)
    // Note: to pass other pos/euler/scale we would need more uniforms; we keep simple and use
    // the plane/analytic test with assumption that caller transformed? For demo we do CPU-assisted
    // tint strength and also do inside test using passed other data (see app code that sets extra uniforms).

    vec3 col = base * (0.35 + 0.65 * ndl) + vec3(1.0) * spec * 0.7;
    if (uAlpha < 0.95) col = mix(col, vec3(0.9), 0.2);

    // Real per-fragment intersection highlight (power of doing everything in shaders!)
    if (uOtherType >= 0) {
        bool inside = IsInsideOther(vWorldPos, uOtherType, uOtherPos, uOtherEuler, uOtherScale,
                                    uOtherNumPlanes, uOtherPlanes, uOtherExtra);
        if (inside) {
            col = mix(col, vec3(1.0, 0.25, 0.85), 0.6); // bright magenta-ish intersection region
            col += vec3(0.15); // extra pop
        }
    }

    FragColor = vec4(col, uAlpha);

    // Simple rim for second body
    float rim = pow(1.0 - max(0.0, dot(N, V)), 3.0);
    if (uAlpha < 0.98) FragColor.rgb = mix(FragColor.rgb, vec3(0.95,0.95,1.0), rim * 0.35);
}
)GLSL";

static const char* kVertPoints = R"GLSL(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 uViewProj;
uniform float uPointSize;
void main() {
    gl_Position = uViewProj * vec4(aPos, 1.0);
    gl_PointSize = uPointSize;
}
)GLSL";

static const char* kFragPoints = R"GLSL(
#version 330 core
uniform vec3 uColor;
out vec4 FragColor;
void main() {
    vec2 c = gl_PointCoord - 0.5;
    float d = dot(c,c);
    if (d > 0.25) discard;
    float a = 1.0 - smoothstep(0.18, 0.25, d);
    FragColor = vec4(uColor, a);
}
)GLSL";

static const char* kVertUI = R"GLSL(
#version 330 core
layout (location = 0) in vec2 aPos;
uniform mat4 uOrtho;
uniform vec4 uRectColor;
out vec4 vColor;
void main() {
    gl_Position = uOrtho * vec4(aPos, 0.0, 1.0);
    vColor = uRectColor;
}
)GLSL";

static const char* kFragUI = R"GLSL(
#version 330 core
in vec4 vColor;
out vec4 FragColor;
void main() { FragColor = vColor; }
)GLSL";

static const char* kVertText = R"GLSL(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUV;
uniform mat4 uOrtho;
out vec2 vUV;
void main() {
    gl_Position = uOrtho * vec4(aPos, 0.0, 1.0);
    vUV = aUV;
}
)GLSL";

static const char* kFragText = R"GLSL(
#version 330 core
in vec2 vUV;
uniform sampler2D uFont;
uniform vec3 uColor;
out vec4 FragColor;
void main() {
    float a = texture(uFont, vUV).a;
    FragColor = vec4(uColor, a);
}
)GLSL";

bool RendererInit(Renderer& r) {
    // Compile programs
    GLuint vs = CompileShader(GL_VERTEX_SHADER, kVertShape);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, kFragShape);
    r.progShape = LinkProgram(vs, fs);
    gl.glDeleteShader(vs); gl.glDeleteShader(fs);

    vs = CompileShader(GL_VERTEX_SHADER, kVertPoints);
    fs = CompileShader(GL_FRAGMENT_SHADER, kFragPoints);
    r.progPoints = LinkProgram(vs, fs);
    gl.glDeleteShader(vs); gl.glDeleteShader(fs);

    vs = CompileShader(GL_VERTEX_SHADER, kVertUI);
    fs = CompileShader(GL_FRAGMENT_SHADER, kFragUI);
    r.progUI = LinkProgram(vs, fs);
    gl.glDeleteShader(vs); gl.glDeleteShader(fs);

    vs = CompileShader(GL_VERTEX_SHADER, kVertText);
    fs = CompileShader(GL_FRAGMENT_SHADER, kFragText);
    r.progText = LinkProgram(vs, fs);
    gl.glDeleteShader(vs); gl.glDeleteShader(fs);

    if (!r.progShape || !r.progUI || !r.progText) return false;

    // Default camera
    r.cam.distance = 520.0f;
    r.cam.yaw = 32.0f;
    r.cam.pitch = 18.0f;
    r.cam.target = Vec3(0, 0, -80);

    // Font texture will be created by GUI layer (see gui.cpp)
    return true;
}

void RendererShutdown(Renderer& r) {
    if (r.progShape) gl.glDeleteProgram(r.progShape);
    if (r.progPoints) gl.glDeleteProgram(r.progPoints);
    if (r.progUI) gl.glDeleteProgram(r.progUI);
    if (r.progText) gl.glDeleteProgram(r.progText);
    r.progShape = r.progPoints = r.progUI = r.progText = 0;
    if (g_fontTex) { gl.glDeleteTextures(1, &g_fontTex); g_fontTex = 0; }
}

void RendererBeginFrame(Renderer& r, int winW, int winH) {
    r.cam.aspect = (winW > 0 && winH > 0) ? float(winW) / float(winH) : 1.6f;

    glClearColor(0.08f, 0.09f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    if (r.showWire) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void RendererEndFrame(Renderer& r) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_CULL_FACE);
}

static void SetShapeUniforms(GLuint prog, const ShapeInstance& inst, const ShapeInstance* other, const Renderer& r) {
    Mat4 model = inst.GetModel();
    Mat4 vp = Mul(r.cam.GetProj(), r.cam.GetView());
    Mat4 mvp = Mul(vp, model);

    GLint loc;
    loc = gl.glGetUniformLocation(prog, "uModel"); if (loc >= 0) gl.glUniformMatrix4fv(loc, 1, GL_FALSE, model.m);
    loc = gl.glGetUniformLocation(prog, "uViewProj"); if (loc >= 0) gl.glUniformMatrix4fv(loc, 1, GL_FALSE, vp.m);

    loc = gl.glGetUniformLocation(prog, "uColor"); if (loc >= 0) gl.glUniform3fv(loc, 1, &inst.color.x);
    loc = gl.glGetUniformLocation(prog, "uAlpha"); if (loc >= 0) gl.glUniform1f(loc, inst.alpha);
    loc = gl.glGetUniformLocation(prog, "uLightDir"); if (loc >= 0) gl.glUniform3fv(loc, 1, &r.lightDir.x);

    // Shape description for analytic + planes
    ShapeGPUData gd; memset(&gd, 0, sizeof(gd));
    ShapeMeshData dm; GenerateUnitMesh(inst.type, dm, gd);

    loc = gl.glGetUniformLocation(prog, "uShapeType"); if (loc>=0) gl.glUniform1i(loc, (int)inst.type);
    loc = gl.glGetUniformLocation(prog, "uNumPlanes"); if (loc>=0) gl.glUniform1i(loc, gd.numPlanes);
    if (gd.numPlanes > 0) {
        loc = gl.glGetUniformLocation(prog, "uPlanes");
        if (loc >= 0) gl.glUniform4fv(loc, gd.numPlanes, (float*)gd.planes);
    }
    Vec3 ex(gd.extra[0], gd.extra[1], gd.extra[2]);
    loc = gl.glGetUniformLocation(prog, "uExtra"); if (loc>=0) gl.glUniform3fv(loc, 1, &ex.x);

    // Other for intersection test in shader (powerful part!)
    if (other) {
        ShapeGPUData od; ShapeMeshData odum; GenerateUnitMesh(other->type, odum, od);
        loc = gl.glGetUniformLocation(prog, "uOtherType"); if (loc>=0) gl.glUniform1i(loc, (int)other->type);
        loc = gl.glGetUniformLocation(prog, "uOtherNumPlanes"); if (loc>=0) gl.glUniform1i(loc, od.numPlanes);
        if (od.numPlanes > 0) {
            loc = gl.glGetUniformLocation(prog, "uOtherPlanes");
            if (loc>=0) gl.glUniform4fv(loc, od.numPlanes, (float*)od.planes);
        }
        Vec3 oex(od.extra[0], od.extra[1], od.extra[2]);
        loc = gl.glGetUniformLocation(prog, "uOtherExtra"); if (loc>=0) gl.glUniform3fv(loc, 1, &oex.x);

        // Pass actual other transform so shader can do accurate World->OtherLocal test
        loc = gl.glGetUniformLocation(prog, "uOtherPos"); if (loc>=0) gl.glUniform3fv(loc, 1, &other->pos.x);
        loc = gl.glGetUniformLocation(prog, "uOtherEuler"); if (loc>=0) gl.glUniform3fv(loc, 1, &other->euler.x);
        loc = gl.glGetUniformLocation(prog, "uOtherScale"); if (loc>=0) gl.glUniform3fv(loc, 1, &other->scale.x);
    } else {
        loc = gl.glGetUniformLocation(prog, "uOtherType"); if (loc>=0) gl.glUniform1i(loc, -1);
    }
}

void DrawShape(const Renderer& r, const ShapeInstance& inst, const ShapeInstance* otherForTest) {
    if (!inst.visible || !r.progShape) return;
    gl.glUseProgram(r.progShape);
    SetShapeUniforms(r.progShape, inst, otherForTest, r);

    // We need the mesh for this shape type. For simplicity we generate + upload every time (small cost, or cache later)
    static GLMesh cache[SHAPE_COUNT];
    static bool inited[SHAPE_COUNT] = {0};

    int idx = (int)inst.type;
    if (!inited[idx]) {
        ShapeMeshData md; ShapeGPUData gd;
        GenerateUnitMesh(inst.type, md, gd);
        // stride 8 floats
        CreateMesh(cache[idx], md.vertices.data(), (int)md.vertices.size() / 8, 8,
                   md.indices.data(), (int)md.indices.size());
        inited[idx] = true;
    }

    BindMesh(cache[idx]);
    DrawMesh(cache[idx]);
    gl.glBindVertexArray(0);
    gl.glUseProgram(0);
}

void DrawAxes(const Renderer& r) {
    if (!r.showAxes || !r.progUI) return;
    glDisable(GL_DEPTH_TEST);
    gl.glUseProgram(r.progUI);

    Mat4 vp = Mul(r.cam.GetProj(), r.cam.GetView());
    GLint loc = gl.glGetUniformLocation(r.progUI, "uOrtho"); // reuse as viewproj for 3d lines hack
    // We will draw 3 lines using immediate style with a temp VBO or glBegin no, use a small static mesh or draw with points/lines via bufferless.

    // Simple: use glDrawArrays with client arrays disabled in core. Create tiny line mesh once.
    static GLuint axesVao = 0, axesVbo = 0;
    if (!axesVao) {
        float data[] = {
            0,0,0,  220,0,0,
            0,0,0,  0,220,0,
            0,0,0,  0,0,220
        };
        gl.glGenVertexArrays(1, &axesVao);
        gl.glGenBuffers(1, &axesVbo);
        gl.glBindVertexArray(axesVao);
        gl.glBindBuffer(GL_ARRAY_BUFFER, axesVbo);
        gl.glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
        gl.glEnableVertexAttribArray(0);
        gl.glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), 0);
        gl.glBindVertexArray(0);
    }
    gl.glBindVertexArray(axesVao);
    // We abuse the UI program a bit; better would be a simple color line shader but reuse ok for axes.
    // Set a identity ortho? Hack: set uOrtho to vp and draw in 3D positions.
    Mat4 id; id.Identity();
    loc = gl.glGetUniformLocation(r.progUI, "uOrtho");
    if (loc>=0) gl.glUniformMatrix4fv(loc, 1, GL_FALSE, vp.m);
    GLint cloc = gl.glGetUniformLocation(r.progUI, "uRectColor");

    // X red
    if (cloc>=0) gl.glUniform4f(cloc, 0.9f,0.2f,0.2f,1.0f);
    glDrawArrays(GL_LINES, 0, 2);
    // Y green
    if (cloc>=0) gl.glUniform4f(cloc, 0.2f,0.85f,0.3f,1.0f);
    glDrawArrays(GL_LINES, 2, 2);
    // Z blue
    if (cloc>=0) gl.glUniform4f(cloc, 0.2f,0.5f,0.95f,1.0f);
    glDrawArrays(GL_LINES, 4, 2);

    gl.glBindVertexArray(0);
    gl.glUseProgram(0);
    glEnable(GL_DEPTH_TEST);
}

void DrawIntersectionPoints(const Renderer& r, const std::vector<Vec3>& pts, Vec3 color, float size) {
    if (pts.empty() || !r.progPoints || !r.showPoints) return;
    gl.glUseProgram(r.progPoints);
    Mat4 vp = Mul(r.cam.GetProj(), r.cam.GetView());
    GLint loc = gl.glGetUniformLocation(r.progPoints, "uViewProj");
    if (loc>=0) gl.glUniformMatrix4fv(loc, 1, GL_FALSE, vp.m);
    loc = gl.glGetUniformLocation(r.progPoints, "uColor"); if (loc>=0) gl.glUniform3fv(loc, 1, &color.x);
    loc = gl.glGetUniformLocation(r.progPoints, "uPointSize"); if (loc>=0) gl.glUniform1f(loc, size);

    static GLuint ptVao=0, ptVbo=0;
    gl.glBindVertexArray(ptVao ? ptVao : (gl.glGenVertexArrays(1,&ptVao), ptVao));
    gl.glBindBuffer(GL_ARRAY_BUFFER, ptVbo ? ptVbo : (gl.glGenBuffers(1,&ptVbo), ptVbo));
    gl.glBufferData(GL_ARRAY_BUFFER, pts.size() * sizeof(Vec3), pts.data(), GL_DYNAMIC_DRAW);
    gl.glEnableVertexAttribArray(0);
    gl.glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3), 0);

    glEnable(0x8642); // GL_PROGRAM_POINT_SIZE
    glDrawArrays(GL_POINTS, 0, (GLsizei)pts.size());
    glDisable(0x8642);

    gl.glBindVertexArray(0);
    gl.glUseProgram(0);
}

void DrawRect2D(float x, float y, float w, float h, Vec3 color, float alpha) {
    // Build screen-space verts every time (simple, no model matrix needed). Callers ensure correct program + ortho + color setup.
    gl.glUseProgram(g_rendererForUI ? g_rendererForUI->progUI : 0);
    float verts[12] = {
        x,   y,   x+w, y,   x+w, y+h,
        x,   y,   x+w, y+h, x,   y+h
    };
    GLuint tmpVao=0, tmpVbo=0;
    gl.glGenVertexArrays(1, &tmpVao);
    gl.glGenBuffers(1, &tmpVbo);
    gl.glBindVertexArray(tmpVao);
    gl.glBindBuffer(GL_ARRAY_BUFFER, tmpVbo);
    gl.glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
    gl.glEnableVertexAttribArray(0);
    gl.glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    GLint loc = gl.glGetUniformLocation(g_rendererForUI ? g_rendererForUI->progUI : 0, "uRectColor");
    if (loc >= 0) gl.glUniform4f(loc, color.x, color.y, color.z, alpha);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    gl.glDeleteVertexArrays(1, &tmpVao);
    gl.glDeleteBuffers(1, &tmpVbo);
    gl.glBindVertexArray(0);
}

Renderer* g_rendererForUI = nullptr; // exported for gui.cpp

void SetUIRenderer(Renderer* r) { g_rendererForUI = r; }

void DrawLine2D(float x0, float y0, float x1, float y1, Vec3 color, float alpha, float thick) {
    // similar temp
    float verts[4] = {x0,y0, x1,y1};
    GLuint vao=0,vbo=0;
    gl.glGenVertexArrays(1,&vao); gl.glGenBuffers(1,&vbo);
    gl.glBindVertexArray(vao);
    gl.glBindBuffer(GL_ARRAY_BUFFER, vbo);
    gl.glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
    gl.glEnableVertexAttribArray(0);
    gl.glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    GLint loc = gl.glGetUniformLocation(g_rendererForUI?g_rendererForUI->progUI:0, "uRectColor");
    if (loc>=0) gl.glUniform4f(loc, color.x,color.y,color.z,alpha);
    glLineWidth(thick);
    glDrawArrays(GL_LINES, 0, 2);
    glLineWidth(1.0f);
    gl.glDeleteVertexArrays(1,&vao); gl.glDeleteBuffers(1,&vbo);
    gl.glBindVertexArray(0);
}

void DrawText2D(float x, float y, const char* text, Vec3 color, float scale) {
    if (!g_fontTex || !g_rendererForUI || !g_rendererForUI->progText || !text || !text[0]) return;

    size_t len = strlen(text);
    if (len == 0) return;

    std::vector<float> verts;
    verts.reserve(len * 6 * 4);

    float lineH = 18.0f * scale;
    float penX = x;

    for (size_t i = 0; i < len; ++i) {
        unsigned char c = (unsigned char)text[i];
        if (c < 32 || c > 126) c = '?';

        float u0 = g_glyphU0[c], v0 = g_glyphV0[c];
        float u1 = g_glyphU1[c], v1 = g_glyphV1[c];
        if (u1 <= u0 || v1 <= v0) {
            penX += 6.0f * scale;
            continue;
        }

        float adv = g_glyphAdvance[c] * scale;
        float physW = adv;
        float physH = lineH;

        float x0 = penX;
        float y0 = y;
        float x1 = penX + physW;
        float y1 = y + physH;

        auto push = [&](float px, float py, float u, float v) {
            verts.push_back(px); verts.push_back(py);
            verts.push_back(u);  verts.push_back(v);
        };

        push(x0, y0, u0, v0);
        push(x1, y0, u1, v0);
        push(x1, y1, u1, v1);
        push(x0, y0, u0, v0);
        push(x1, y1, u1, v1);
        push(x0, y1, u0, v1);

        penX += adv;
    }

    if (verts.empty()) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    gl.glUseProgram(g_rendererForUI->progText);
    GUISetOrthoForProgram(g_rendererForUI->progText);

    gl.glActiveTexture(GL_TEXTURE0);
    gl.glBindTexture(GL_TEXTURE_2D, g_fontTex);
    GLint loc = gl.glGetUniformLocation(g_rendererForUI->progText, "uFont");
    if (loc >= 0) gl.glUniform1i(loc, 0);
    loc = gl.glGetUniformLocation(g_rendererForUI->progText, "uColor");
    if (loc >= 0) gl.glUniform3fv(loc, 1, &color.x);

    GLuint vao = 0, vbo = 0;
    gl.glGenVertexArrays(1, &vao);
    gl.glGenBuffers(1, &vbo);
    gl.glBindVertexArray(vao);
    gl.glBindBuffer(GL_ARRAY_BUFFER, vbo);
    gl.glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_DYNAMIC_DRAW);
    gl.glEnableVertexAttribArray(0);
    gl.glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    gl.glEnableVertexAttribArray(1);
    gl.glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(verts.size() / 4));
    gl.glBindVertexArray(0);
    gl.glDeleteVertexArrays(1, &vao);
    gl.glDeleteBuffers(1, &vbo);

    gl.glUseProgram(0);
    gl.glBindTexture(GL_TEXTURE_2D, 0);
}

void DrawLineStrip3D(const Renderer& r, const std::vector<Vec3>& pts, Vec3 color, float alpha, float lineWidth) {
    if (pts.size() < 2 || !r.progUI) return;
    glDisable(GL_DEPTH_TEST);
    gl.glUseProgram(r.progUI);
    Mat4 vp = Mul(r.cam.GetProj(), r.cam.GetView());
    GLint loc = gl.glGetUniformLocation(r.progUI, "uOrtho");
    if (loc >= 0) gl.glUniformMatrix4fv(loc, 1, GL_FALSE, vp.m);
    loc = gl.glGetUniformLocation(r.progUI, "uRectColor");
    if (loc >= 0) gl.glUniform4f(loc, color.x, color.y, color.z, alpha);

    GLuint vao = 0, vbo = 0;
    gl.glGenVertexArrays(1, &vao);
    gl.glGenBuffers(1, &vbo);
    gl.glBindVertexArray(vao);
    gl.glBindBuffer(GL_ARRAY_BUFFER, vbo);
    gl.glBufferData(GL_ARRAY_BUFFER, pts.size() * sizeof(Vec3), pts.data(), GL_DYNAMIC_DRAW);
    gl.glEnableVertexAttribArray(0);
    gl.glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3), 0);
    glLineWidth(lineWidth);
    glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)pts.size());
    glLineWidth(1.0f);
    gl.glBindVertexArray(0);
    gl.glDeleteVertexArrays(1, &vao);
    gl.glDeleteBuffers(1, &vbo);
    gl.glUseProgram(0);
    glEnable(GL_DEPTH_TEST);
}

void DrawIntersectionCurve3D(const Renderer& r, const std::vector<Vec3>& curve) {
    if (!r.showCurve || curve.size() < 2) return;
    DrawLineStrip3D(r, curve, Vec3(1.0f, 0.85f, 0.2f), 0.95f, 2.8f);
}

void DrawPolyline2D(const std::vector<Vec2>& pts, Vec3 color, float alpha, bool closed) {
    if (pts.size() < 2) return;
    for (size_t i = 0; i + 1 < pts.size(); ++i)
        DrawLine2D(pts[i].x, pts[i].y, pts[i + 1].x, pts[i + 1].y, color, alpha, 1.2f);
    if (closed && pts.size() > 2)
        DrawLine2D(pts.back().x, pts.back().y, pts.front().x, pts.front().y, color, alpha, 1.2f);
}

void DrawGrid2D(Rect2D area, int divisions, Vec3 color, float alpha) {
    if (divisions < 1) divisions = 1;
    float dx = area.w / divisions;
    float dy = area.h / divisions;
    for (int i = 0; i <= divisions; ++i) {
        float x = area.x + dx * i;
        DrawLine2D(x, area.y, x, area.y + area.h, color, alpha, 1.0f);
        float y = area.y + dy * i;
        DrawLine2D(area.x, y, area.x + area.w, y, color, alpha, 1.0f);
    }
}

static Vec2 MapSheetPoint(Vec2 modelPt, const ProjectionSheet& sheet, float cx, float cy, float halfW, float halfH) {
    Vec2 d = modelPt - sheet.origin;
    float k = sheet.scale * 0.42f;
    return Vec2(cx + d.x * k * halfW, cy + d.y * k * halfH);
}

static void DrawProjectedView(const ProjectionSheet& sheet, const ProjectedCurve2D& curve,
    const ProjectedCurve2D& outlineA, const ProjectedCurve2D& outlineB,
    Rect2D viewArea, const char* label, bool showOutlines)
{
    DrawRect2D(viewArea.x, viewArea.y, viewArea.w, viewArea.h, Vec3(0.95f, 0.96f, 0.98f), 0.98f);
    DrawGrid2D(viewArea, 8, Vec3(0.82f, 0.84f, 0.88f), 0.55f);
    DrawRect2D(viewArea.x, viewArea.y, viewArea.w, viewArea.h, Vec3(0.35f, 0.38f, 0.42f), 0.35f);

    float cx = viewArea.x + viewArea.w * 0.5f;
    float cy = viewArea.y + viewArea.h * 0.5f;
    float halfW = viewArea.w * 0.5f;
    float halfH = viewArea.h * 0.5f;

    if (showOutlines && outlineA.points.size() > 2) {
        std::vector<Vec2> scr;
        scr.reserve(outlineA.points.size());
        for (auto& p : outlineA.points) scr.push_back(MapSheetPoint(p, sheet, cx, cy, halfW, halfH));
        DrawPolyline2D(scr, Vec3(0.25f, 0.55f, 0.95f), 0.35f, false);
    }
    if (showOutlines && outlineB.points.size() > 2) {
        std::vector<Vec2> scr;
        scr.reserve(outlineB.points.size());
        for (auto& p : outlineB.points) scr.push_back(MapSheetPoint(p, sheet, cx, cy, halfW, halfH));
        DrawPolyline2D(scr, Vec3(0.95f, 0.45f, 0.15f), 0.35f, false);
    }
    if (curve.points.size() > 1) {
        std::vector<Vec2> scr;
        scr.reserve(curve.points.size());
        for (auto& p : curve.points) scr.push_back(MapSheetPoint(p, sheet, cx, cy, halfW, halfH));
        DrawPolyline2D(scr, Vec3(1.0f, 0.75f, 0.1f), 0.95f, true);
    }

    Label(label, viewArea.x + 6, viewArea.y + viewArea.h - 16, Vec3(0.2f, 0.22f, 0.28f), 0.85f);
}

void DrawProjectionSheet2D(const ProjectionSheet& sheet, Rect2D area, bool showOutlines) {
    DrawRect2D(area.x, area.y, area.w, area.h, Vec3(0.88f, 0.9f, 0.93f), 0.96f);
    DrawRect2D(area.x + 2, area.y + 2, area.w - 4, area.h - 4, Vec3(0.2f, 0.22f, 0.28f), 0.5f);

    float pad = 8.0f;
    float vw = (area.w - pad * 4.0f) / 3.0f;
    float vh = area.h - pad * 2.0f - 20.0f;
    float y0 = area.y + pad;

    Rect2D front(area.x + pad, y0, vw, vh);
    Rect2D top(area.x + pad * 2 + vw, y0, vw, vh);
    Rect2D right(area.x + pad * 3 + vw * 2, y0, vw, vh);

    DrawProjectedView(sheet, sheet.curveFront, sheet.outlineAFront, sheet.outlineBFront, front, "FRONT", showOutlines);
    DrawProjectedView(sheet, sheet.curveTop, sheet.outlineATop, sheet.outlineBTop, top, "TOP", showOutlines);
    DrawProjectedView(sheet, sheet.curveRight, sheet.outlineARight, sheet.outlineBRight, right, "RIGHT", showOutlines);

    Label("DRAWING SHEET", area.x + 10, area.y + area.h - 14, Vec3(0.75f, 0.78f, 0.85f), 0.9f);
}

bool ReloadShaders(Renderer& r) {
    RendererShutdown(r);
    return RendererInit(r);
}
