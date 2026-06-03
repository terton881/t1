#pragma once
// gl_core.h - Minimal modern OpenGL loader + context creation for Win32 (core 3.3+)
// No external dependencies. Loads only what we need.

#include <windows.h>
#include <GL/gl.h>

// --- Minimal GL 3.3+ constants and types (avoid glu, legacy) ---
#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_STATIC_DRAW                    0x88E4
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_INFO_LOG_LENGTH                0x8B84
#define GL_TEXTURE0                       0x84C0
#define GL_BGRA                           0x80E1
#define GL_UNSIGNED_INT_8_8_8_8_REV       0x8367
#define GL_MULTISAMPLE                    0x809D
#define GL_SAMPLE_BUFFERS                 0x80A8
#define GL_SAMPLES                        0x80A9
#define GL_PROGRAM_POINT_SIZE             0x8642
#define GL_R8                             0x8229

typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;
typedef char GLchar;

typedef void (APIENTRY *GLDEBUGPROC)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

// Function pointers
extern "C" {
    // WGL
    typedef HGLRC(WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC hDC, HGLRC hShareContext, const int* attribList);
    typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC)(int interval);
    typedef int (WINAPI* PFNWGLGETSWAPINTERVALEXTPROC)(void);

    // GL 1.5+
    typedef void (APIENTRY* PFNGLGENBUFFERSPROC)(GLsizei n, GLuint* buffers);
    typedef void (APIENTRY* PFNGLDELETEBUFFERSPROC)(GLsizei n, const GLuint* buffers);
    typedef void (APIENTRY* PFNGLBINDBUFFERPROC)(GLenum target, GLuint buffer);
    typedef void (APIENTRY* PFNGLBUFFERDATAPROC)(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
    typedef void (APIENTRY* PFNGLBUFFERSUBDATAPROC)(GLenum target, GLintptr offset, GLsizeiptr size, const void* data);
    typedef void* (APIENTRY* PFNGLMAPBUFFERPROC)(GLenum target, GLenum access);
    typedef GLboolean(APIENTRY* PFNGLUNMAPBUFFERPROC)(GLenum target);

    // GL 2.0+
    typedef GLuint(APIENTRY* PFNGLCREATESHADERPROC)(GLenum type);
    typedef void (APIENTRY* PFNGLDELETESHADERPROC)(GLuint shader);
    typedef void (APIENTRY* PFNGLSHADERSOURCEPROC)(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length);
    typedef void (APIENTRY* PFNGLCOMPILESHADERPROC)(GLuint shader);
    typedef void (APIENTRY* PFNGLGETSHADERIVPROC)(GLuint shader, GLenum pname, GLint* params);
    typedef void (APIENTRY* PFNGLGETSHADERINFOLOGPROC)(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
    typedef GLuint(APIENTRY* PFNGLCREATEPROGRAMPROC)(void);
    typedef void (APIENTRY* PFNGLDELETEPROGRAMPROC)(GLuint program);
    typedef void (APIENTRY* PFNGLATTACHSHADERPROC)(GLuint program, GLuint shader);
    typedef void (APIENTRY* PFNGLLINKPROGRAMPROC)(GLuint program);
    typedef void (APIENTRY* PFNGLUSEPROGRAMPROC)(GLuint program);
    typedef void (APIENTRY* PFNGLGETPROGRAMIVPROC)(GLuint program, GLenum pname, GLint* params);
    typedef void (APIENTRY* PFNGLGETPROGRAMINFOLOGPROC)(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
    typedef void (APIENTRY* PFNGLGETACTIVEUNIFORMPROC)(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name);
    typedef GLint(APIENTRY* PFNGLGETUNIFORMLOCATIONPROC)(GLuint program, const GLchar* name);
    typedef void (APIENTRY* PFNGLUNIFORM1IPROC)(GLint location, GLint v0);
    typedef void (APIENTRY* PFNGLUNIFORM1FPROC)(GLint location, GLfloat v0);
    typedef void (APIENTRY* PFNGLUNIFORM2FPROC)(GLint location, GLfloat v0, GLfloat v1);
    typedef void (APIENTRY* PFNGLUNIFORM3FPROC)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
    typedef void (APIENTRY* PFNGLUNIFORM4FPROC)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
    typedef void (APIENTRY* PFNGLUNIFORMMATRIX4FVPROC)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
    typedef void (APIENTRY* PFNGLUNIFORM3FVPROC)(GLint location, GLsizei count, const GLfloat* value);
    typedef void (APIENTRY* PFNGLUNIFORM4FVPROC)(GLint location, GLsizei count, const GLfloat* value);

    // VAO 3.0+
    typedef void (APIENTRY* PFNGLGENVERTEXARRAYSPROC)(GLsizei n, GLuint* arrays);
    typedef void (APIENTRY* PFNGLDELETEVERTEXARRAYSPROC)(GLsizei n, const GLuint* arrays);
    typedef void (APIENTRY* PFNGLBINDVERTEXARRAYPROC)(GLuint array);
    typedef void (APIENTRY* PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint index);
    typedef void (APIENTRY* PFNGLDISABLEVERTEXATTRIBARRAYPROC)(GLuint index);
    typedef void (APIENTRY* PFNGLVERTEXATTRIBPOINTERPROC)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);

    // Textures etc already in gl.h mostly, add a few
    typedef void (APIENTRY* PFNGLACTIVETEXTUREPROC)(GLenum texture);
    typedef void (APIENTRY* PFNGLGENERATEMIPMAPPROC)(GLenum target);
}

struct GLProcs {
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = nullptr;
    PFNWGLSWAPINTERVALEXTPROC         wglSwapIntervalEXT = nullptr;
    PFNWGLGETSWAPINTERVALEXTPROC      wglGetSwapIntervalEXT = nullptr;

    PFNGLGENBUFFERSPROC               glGenBuffers = nullptr;
    PFNGLDELETEBUFFERSPROC            glDeleteBuffers = nullptr;
    PFNGLBINDBUFFERPROC               glBindBuffer = nullptr;
    PFNGLBUFFERDATAPROC               glBufferData = nullptr;
    PFNGLBUFFERSUBDATAPROC            glBufferSubData = nullptr;

    PFNGLCREATESHADERPROC             glCreateShader = nullptr;
    PFNGLDELETESHADERPROC             glDeleteShader = nullptr;
    PFNGLSHADERSOURCEPROC             glShaderSource = nullptr;
    PFNGLCOMPILESHADERPROC            glCompileShader = nullptr;
    PFNGLGETSHADERIVPROC              glGetShaderiv = nullptr;
    PFNGLGETSHADERINFOLOGPROC         glGetShaderInfoLog = nullptr;
    PFNGLCREATEPROGRAMPROC            glCreateProgram = nullptr;
    PFNGLDELETEPROGRAMPROC            glDeleteProgram = nullptr;
    PFNGLATTACHSHADERPROC             glAttachShader = nullptr;
    PFNGLLINKPROGRAMPROC              glLinkProgram = nullptr;
    PFNGLUSEPROGRAMPROC               glUseProgram = nullptr;
    PFNGLGETPROGRAMIVPROC             glGetProgramiv = nullptr;
    PFNGLGETPROGRAMINFOLOGPROC        glGetProgramInfoLog = nullptr;
    PFNGLGETUNIFORMLOCATIONPROC       glGetUniformLocation = nullptr;
    PFNGLUNIFORM1IPROC                glUniform1i = nullptr;
    PFNGLUNIFORM1FPROC                glUniform1f = nullptr;
    PFNGLUNIFORM2FPROC                glUniform2f = nullptr;
    PFNGLUNIFORM3FPROC                glUniform3f = nullptr;
    PFNGLUNIFORM4FPROC                glUniform4f = nullptr;
    PFNGLUNIFORMMATRIX4FVPROC         glUniformMatrix4fv = nullptr;
    PFNGLUNIFORM3FVPROC               glUniform3fv = nullptr;
    PFNGLUNIFORM4FVPROC               glUniform4fv = nullptr;

    PFNGLGENVERTEXARRAYSPROC          glGenVertexArrays = nullptr;
    PFNGLDELETEVERTEXARRAYSPROC       glDeleteVertexArrays = nullptr;
    PFNGLBINDVERTEXARRAYPROC          glBindVertexArray = nullptr;
    PFNGLENABLEVERTEXATTRIBARRAYPROC  glEnableVertexAttribArray = nullptr;
    PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray = nullptr;
    PFNGLVERTEXATTRIBPOINTERPROC      glVertexAttribPointer = nullptr;

    PFNGLACTIVETEXTUREPROC            glActiveTexture = nullptr;
    PFNGLGENERATEMIPMAPPROC           glGenerateMipmap = nullptr;

    // Texture (core)
    void (APIENTRY* glBindTexture)(GLenum, GLuint) = nullptr;
    void (APIENTRY* glGenTextures)(GLsizei, GLuint*) = nullptr;
    void (APIENTRY* glDeleteTextures)(GLsizei, const GLuint*) = nullptr;
    void (APIENTRY* glTexParameteri)(GLenum, GLenum, GLint) = nullptr;
    void (APIENTRY* glTexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*) = nullptr;
};

extern GLProcs gl;

// Load all required function pointers. Requires a current (compat) GL context first.
bool LoadGLProcs(HDC hdc);

// Create modern core profile context. Call after temp context + LoadGLProcs.
HGLRC CreateModernContext(HDC hdc, int major, int minor, bool debug);

// Pixel format + MSAA helper
int SetupPixelFormatModern(HDC hdc, int msaaSamples);

// Convenience
void SetVSync(bool enabled);

// Shader helpers
GLuint CompileShader(GLenum type, const char* src);
GLuint LinkProgram(GLuint vs, GLuint fs);
void CheckShaderLog(GLuint obj, bool isProgram);

// Basic VAO helpers (user still manages VBO/EBO)
struct GLMesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    int    indexCount = 0;
    GLenum prim = GL_TRIANGLES;
};

void CreateMesh(GLMesh& mesh, const float* verts, int vertCount, int vertStrideFloats, const unsigned int* indices, int idxCount, GLenum usage = GL_STATIC_DRAW);
void DestroyMesh(GLMesh& mesh);
void BindMesh(const GLMesh& mesh);
void DrawMesh(const GLMesh& mesh);
