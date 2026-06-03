#include "gl_core.h"
#include <stdio.h>
#include <string.h>

GLProcs gl;

static void* GetAnyGLFuncAddress(const char* name) {
    void* p = (void*)wglGetProcAddress(name);
    if (p == 0 || (p == (void*)0x1) || (p == (void*)0x2) || (p == (void*)0x3) || (p == (void*)-1)) {
        HMODULE module = LoadLibraryA("opengl32.dll");
        if (module) p = (void*)GetProcAddress(module, name);
    }
    return p;
}

bool LoadGLProcs(HDC hdc) {
    // WGL extensions first (need temp context usually)
    gl.wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)GetAnyGLFuncAddress("wglCreateContextAttribsARB");
    gl.wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)GetAnyGLFuncAddress("wglSwapIntervalEXT");
    gl.wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXTPROC)GetAnyGLFuncAddress("wglGetSwapIntervalEXT");

    // Core
    gl.glGenBuffers = (PFNGLGENBUFFERSPROC)GetAnyGLFuncAddress("glGenBuffers");
    gl.glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)GetAnyGLFuncAddress("glDeleteBuffers");
    gl.glBindBuffer = (PFNGLBINDBUFFERPROC)GetAnyGLFuncAddress("glBindBuffer");
    gl.glBufferData = (PFNGLBUFFERDATAPROC)GetAnyGLFuncAddress("glBufferData");
    gl.glBufferSubData = (PFNGLBUFFERSUBDATAPROC)GetAnyGLFuncAddress("glBufferSubData");

    gl.glCreateShader = (PFNGLCREATESHADERPROC)GetAnyGLFuncAddress("glCreateShader");
    gl.glDeleteShader = (PFNGLDELETESHADERPROC)GetAnyGLFuncAddress("glDeleteShader");
    gl.glShaderSource = (PFNGLSHADERSOURCEPROC)GetAnyGLFuncAddress("glShaderSource");
    gl.glCompileShader = (PFNGLCOMPILESHADERPROC)GetAnyGLFuncAddress("glCompileShader");
    gl.glGetShaderiv = (PFNGLGETSHADERIVPROC)GetAnyGLFuncAddress("glGetShaderiv");
    gl.glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)GetAnyGLFuncAddress("glGetShaderInfoLog");

    gl.glCreateProgram = (PFNGLCREATEPROGRAMPROC)GetAnyGLFuncAddress("glCreateProgram");
    gl.glDeleteProgram = (PFNGLDELETEPROGRAMPROC)GetAnyGLFuncAddress("glDeleteProgram");
    gl.glAttachShader = (PFNGLATTACHSHADERPROC)GetAnyGLFuncAddress("glAttachShader");
    gl.glLinkProgram = (PFNGLLINKPROGRAMPROC)GetAnyGLFuncAddress("glLinkProgram");
    gl.glUseProgram = (PFNGLUSEPROGRAMPROC)GetAnyGLFuncAddress("glUseProgram");
    gl.glGetProgramiv = (PFNGLGETPROGRAMIVPROC)GetAnyGLFuncAddress("glGetProgramiv");
    gl.glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)GetAnyGLFuncAddress("glGetProgramInfoLog");
    gl.glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)GetAnyGLFuncAddress("glGetUniformLocation");

    gl.glUniform1i = (PFNGLUNIFORM1IPROC)GetAnyGLFuncAddress("glUniform1i");
    gl.glUniform1f = (PFNGLUNIFORM1FPROC)GetAnyGLFuncAddress("glUniform1f");
    gl.glUniform2f = (PFNGLUNIFORM2FPROC)GetAnyGLFuncAddress("glUniform2f");
    gl.glUniform3f = (PFNGLUNIFORM3FPROC)GetAnyGLFuncAddress("glUniform3f");
    gl.glUniform4f = (PFNGLUNIFORM4FPROC)GetAnyGLFuncAddress("glUniform4f");
    gl.glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)GetAnyGLFuncAddress("glUniformMatrix4fv");
    gl.glUniform3fv = (PFNGLUNIFORM3FVPROC)GetAnyGLFuncAddress("glUniform3fv");
    gl.glUniform4fv = (PFNGLUNIFORM4FVPROC)GetAnyGLFuncAddress("glUniform4fv");

    gl.glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)GetAnyGLFuncAddress("glGenVertexArrays");
    gl.glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)GetAnyGLFuncAddress("glDeleteVertexArrays");
    gl.glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)GetAnyGLFuncAddress("glBindVertexArray");
    gl.glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)GetAnyGLFuncAddress("glEnableVertexAttribArray");
    gl.glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)GetAnyGLFuncAddress("glDisableVertexAttribArray");
    gl.glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)GetAnyGLFuncAddress("glVertexAttribPointer");

    gl.glActiveTexture = (PFNGLACTIVETEXTUREPROC)GetAnyGLFuncAddress("glActiveTexture");
    gl.glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)GetAnyGLFuncAddress("glGenerateMipmap");

    gl.glBindTexture = (void (APIENTRY*)(GLenum, GLuint))GetAnyGLFuncAddress("glBindTexture");
    gl.glGenTextures = (void (APIENTRY*)(GLsizei, GLuint*))GetAnyGLFuncAddress("glGenTextures");
    gl.glDeleteTextures = (void (APIENTRY*)(GLsizei, const GLuint*))GetAnyGLFuncAddress("glDeleteTextures");
    gl.glTexParameteri = (void (APIENTRY*)(GLenum, GLenum, GLint))GetAnyGLFuncAddress("glTexParameteri");
    gl.glTexImage2D = (void (APIENTRY*)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*))GetAnyGLFuncAddress("glTexImage2D");

    // Verify critical
    if (!gl.glCreateProgram || !gl.glGenVertexArrays || !gl.glBindBuffer) {
        MessageBoxA(NULL, "Failed to load required OpenGL 3.3 functions.\nModern context creation will fail.", "GL Loader", MB_ICONERROR);
        return false;
    }
    return true;
}

HGLRC CreateModernContext(HDC hdc, int major, int minor, bool debug) {
    if (!gl.wglCreateContextAttribsARB) {
        // Fallback to old
        return wglCreateContext(hdc);
    }
    int attribs[] = {
        0x2091 /*WGL_CONTEXT_MAJOR_VERSION_ARB*/, major,
        0x2092 /*WGL_CONTEXT_MINOR_VERSION_ARB*/, minor,
        0x2094 /*WGL_CONTEXT_FLAGS_ARB*/, debug ? 0x0001 /*WGL_CONTEXT_DEBUG_BIT_ARB*/ : 0,
        0x2095 /*WGL_CONTEXT_PROFILE_MASK_ARB*/, 0x00000001 /*WGL_CONTEXT_CORE_PROFILE_BIT_ARB*/,
        0
    };
    HGLRC ctx = gl.wglCreateContextAttribsARB(hdc, 0, attribs);
    if (!ctx) {
        // Try without debug
        attribs[5] = 0;
        ctx = gl.wglCreateContextAttribsARB(hdc, 0, attribs);
    }
    return ctx;
}

int SetupPixelFormatModern(HDC hdc, int msaaSamples) {
    PIXELFORMATDESCRIPTOR pfd = { 0 };
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pf = ChoosePixelFormat(hdc, &pfd);
    if (!pf) pf = 1;
    SetPixelFormat(hdc, pf, &pfd);
    return pf;
}

void SetVSync(bool enabled) {
    if (gl.wglSwapIntervalEXT) {
        gl.wglSwapIntervalEXT(enabled ? 1 : 0);
    }
}

// ---- Shader utils ----
GLuint CompileShader(GLenum type, const char* src) {
    GLuint sh = gl.glCreateShader(type);
    gl.glShaderSource(sh, 1, &src, nullptr);
    gl.glCompileShader(sh);
    GLint ok = 0;
    gl.glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        GLsizei len = 0;
        gl.glGetShaderInfoLog(sh, sizeof(log) - 1, &len, log);
        log[len] = 0;
        char title[64];
        sprintf_s(title, "Shader Compile Error (%s)", (type == GL_VERTEX_SHADER ? "VS" : "FS"));
        MessageBoxA(NULL, log, title, MB_ICONERROR);
        gl.glDeleteShader(sh);
        return 0;
    }
    return sh;
}

GLuint LinkProgram(GLuint vs, GLuint fs) {
    GLuint prog = gl.glCreateProgram();
    if (vs) gl.glAttachShader(prog, vs);
    if (fs) gl.glAttachShader(prog, fs);
    gl.glLinkProgram(prog);
    GLint ok = 0;
    gl.glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        GLsizei len = 0;
        gl.glGetProgramInfoLog(prog, sizeof(log) - 1, &len, log);
        log[len] = 0;
        MessageBoxA(NULL, log, "Program Link Error", MB_ICONERROR);
        gl.glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

void CheckShaderLog(GLuint obj, bool isProgram) {
    // optional detailed
}

void CreateMesh(GLMesh& mesh, const float* verts, int vertCount, int vertStrideFloats, const unsigned int* indices, int idxCount, GLenum usage) {
    DestroyMesh(mesh);
    mesh.indexCount = idxCount;
    mesh.prim = GL_TRIANGLES;

    gl.glGenVertexArrays(1, &mesh.vao);
    gl.glGenBuffers(1, &mesh.vbo);
    if (indices && idxCount > 0) gl.glGenBuffers(1, &mesh.ebo);

    gl.glBindVertexArray(mesh.vao);

    gl.glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    gl.glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)vertCount * vertStrideFloats * sizeof(float), verts, usage);

    if (mesh.ebo) {
        gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
        gl.glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)idxCount * sizeof(unsigned int), indices, usage);
    }

    // Assume layout: location 0 = vec3 pos, 1 = vec3 normal, 2 = vec2 uv (optional)
    GLsizei stride = vertStrideFloats * sizeof(float);
    gl.glEnableVertexAttribArray(0);
    gl.glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    if (vertStrideFloats >= 6) {
        gl.glEnableVertexAttribArray(1);
        gl.glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    }
    if (vertStrideFloats >= 8) {
        gl.glEnableVertexAttribArray(2);
        gl.glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    }

    gl.glBindVertexArray(0);
    gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
    if (mesh.ebo) gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void DestroyMesh(GLMesh& mesh) {
    if (mesh.vao) gl.glDeleteVertexArrays(1, &mesh.vao);
    if (mesh.vbo) gl.glDeleteBuffers(1, &mesh.vbo);
    if (mesh.ebo) gl.glDeleteBuffers(1, &mesh.ebo);
    mesh.vao = mesh.vbo = mesh.ebo = 0;
    mesh.indexCount = 0;
}

void BindMesh(const GLMesh& mesh) {
    gl.glBindVertexArray(mesh.vao);
}

void DrawMesh(const GLMesh& mesh) {
    if (mesh.ebo && mesh.indexCount > 0) {
        glDrawElements(mesh.prim, mesh.indexCount, GL_UNSIGNED_INT, 0);
    } else {
        glDrawArrays(mesh.prim, 0, mesh.indexCount ? mesh.indexCount : 3);
    }
}
