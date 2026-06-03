#include "gui.h"
#include <string.h>
#include <stdio.h>

GUIState g_gui;

static int s_nextId = 1;
static int GenId() { return s_nextId++; }

static float s_orthoM[16];

void GUIBegin(int winW, int winH, int mx, int my, bool lmb) {
    g_gui.winW = winW; g_gui.winH = winH;
    g_gui.mouseX = mx; g_gui.mouseY = my;
    g_gui.mouseWasDown = g_gui.mouseDown;
    g_gui.mouseDown = lmb;
    if (!g_gui.mouseDown) g_gui.activeItem = 0;
    s_nextId = 1;
    GUISetOrtho((float)winW, (float)winH);
}

void GUIEnd() {
    if (!g_gui.mouseDown) {
        g_gui.hotItem = 0;
        g_gui.activeItem = 0;
    }
}

void GUISetOrtho(float w, float h) {
    // ortho: 0,0 top-left? We use bottom-left like GL NDC but flip y in shader? 
    // Simple pixel ortho: x right, y up from bottom
    memset(s_orthoM, 0, sizeof(s_orthoM));
    s_orthoM[0] = 2.0f / w;
    s_orthoM[5] = 2.0f / h;
    s_orthoM[10] = 1.0f;
    s_orthoM[12] = -1.0f;
    s_orthoM[13] = -1.0f;
    s_orthoM[15] = 1.0f;
}

static void ApplyOrtho(GLuint prog) {
    GLint loc = gl.glGetUniformLocation(prog, "uOrtho");
    if (loc >= 0) gl.glUniformMatrix4fv(loc, 1, GL_FALSE, s_orthoM);
}

bool GUIRectHit(float x, float y, float w, float h) {
    int mx = g_gui.mouseX, my = g_gui.winH - g_gui.mouseY; // flip to y-up
    return mx >= x && mx <= (x + w) && my >= y && my <= (y + h);
}

static void DrawPanelBG(float x, float y, float w, float h, float alpha) {
    if (!g_rendererForUI) return;
    gl.glUseProgram(g_rendererForUI->progUI);
    ApplyOrtho(g_rendererForUI->progUI);
    // dark panel
    DrawRect2D(x, y, w, h, Vec3(0.11f, 0.12f, 0.15f), alpha);
    // subtle border
    DrawRect2D(x, y, w, 1.5f, Vec3(0.25f,0.27f,0.32f), 1.0f);
    DrawRect2D(x, y+h-1.5f, w, 1.5f, Vec3(0.25f,0.27f,0.32f), 1.0f);
    DrawRect2D(x, y, 1.5f, h, Vec3(0.25f,0.27f,0.32f), 1.0f);
    DrawRect2D(x+w-1.5f, y, 1.5f, h, Vec3(0.25f,0.27f,0.32f), 1.0f);
    gl.glUseProgram(0);
}

void Panel(float x, float y, float w, float h, const char* title, float alpha) {
    DrawPanelBG(x, y, w, h, alpha);
    if (title) {
        Label(title, x + 10, y + h - 22, Vec3(0.75f, 0.78f, 0.85f), 1.05f);
        // separator line
        DrawLine2D(x+8, y + h - 28, x + w - 8, y + h - 28, Vec3(0.3f,0.32f,0.36f), 0.9f, 1.0f);
    }
}

bool Button(const char* label, float x, float y, float w, float h) {
    int id = GenId();
    bool hover = GUIRectHit(x, y, w, h);
    bool clicked = false;

    if (hover) g_gui.hotItem = id;
    bool isActive = (g_gui.activeItem == id);

    if (hover && g_gui.mouseDown && !g_gui.mouseWasDown) {
        g_gui.activeItem = id;
    }
    if (isActive && !g_gui.mouseDown && g_gui.mouseWasDown) {
        if (hover) clicked = true;
        g_gui.activeItem = 0;
    }

    Vec3 col = hover ? Vec3(0.22f, 0.28f, 0.38f) : Vec3(0.16f, 0.18f, 0.22f);
    if (isActive) col = Vec3(0.15f, 0.35f, 0.55f);

    if (g_rendererForUI) {
        gl.glUseProgram(g_rendererForUI->progUI);
        ApplyOrtho(g_rendererForUI->progUI);
        DrawRect2D(x, y, w, h, col, 0.95f);
        // Proper borders so buttons don't look like blobs / crooked
        Vec3 brd = hover ? Vec3(0.45f, 0.52f, 0.62f) : Vec3(0.32f, 0.35f, 0.40f);
        float bw = 1.0f;
        DrawRect2D(x, y, w, bw, brd, 0.9f);                 // top
        DrawRect2D(x, y + h - bw, w, bw, brd, 0.9f);        // bottom
        DrawRect2D(x, y, bw, h, brd, 0.9f);                 // left
        DrawRect2D(x + w - bw, y, bw, h, brd, 0.9f);        // right
        // subtle top highlight for pressed feel
        if (!isActive) {
            DrawRect2D(x + bw, y + h - bw*2, w - bw*2, bw, Vec3(0.38f,0.42f,0.48f), 0.5f);
        }
        gl.glUseProgram(0);
    }
    // Vertically center label text inside button (match larger DrawText2D phys for readability)
    float textScale = 1.0f;
    float textH = 16.0f * textScale;
    float textY = y + (h - textH) * 0.5f - 0.5f;
    Label(label, x + 5, textY, hover ? Vec3(0.98f,0.98f,1.0f) : Vec3(0.92f,0.93f,0.96f), textScale);
    return clicked;
}

bool Slider(const char* label, float x, float y, float w, float h, float& value, float vmin, float vmax, const char* fmt) {
    int id = GenId();
    float tw = 58.0f;
    float barX = x + tw + 4;
    float barW = w - tw - 8;
    float barH = 14.0f;
    float barY = y + (h - barH) * 0.5f + 1;

    bool changed = false;
    bool hover = GUIRectHit(barX - 4, barY - 3, barW + 8, barH + 6);

    if (hover) g_gui.hotItem = id;

    if (hover && g_gui.mouseDown) {
        g_gui.activeItem = id;
    }
    if (g_gui.activeItem == id) {
        float t = (g_gui.mouseX - barX) / barW;
        t = (t < 0 ? 0 : (t > 1 ? 1 : t));
        float nv = vmin + (vmax - vmin) * t;
        if (fabsf(nv - value) > 1e-4f) {
            value = nv;
            changed = true;
        }
    }

    // label + value
    char buf[128];
    sprintf_s(buf, "%s", label);
    Label(buf, x, y + 2, Vec3(0.82f,0.84f,0.88f), 0.92f);

    float t = (value - vmin) / (vmax - vmin + 1e-12f);
    t = (t < 0 ? 0 : (t > 1 ? 1 : t));

    // track
    if (g_rendererForUI) {
        gl.glUseProgram(g_rendererForUI->progUI);
        ApplyOrtho(g_rendererForUI->progUI);
        DrawRect2D(barX, barY, barW, barH, Vec3(0.12f,0.13f,0.16f), 1.0f);
        DrawRect2D(barX, barY, barW * t, barH, Vec3(0.25f,0.55f,0.85f), 0.95f);
        // knob
        float kx = barX + barW * t - 4;
        DrawRect2D(kx, barY - 1, 8, barH + 2, Vec3(0.85f,0.88f,0.95f), 1.0f);
        gl.glUseProgram(0);
    }

    // value text
    sprintf_s(buf, fmt ? fmt : "%.1f", value);
    Label(buf, barX + barW + 6, y + 2, Vec3(0.7f,0.85f,1.0f), 0.9f);
    return changed;
}

bool Checkbox(const char* label, float x, float y, bool& checked) {
    int id = GenId();
    float box = 14;
    bool hover = GUIRectHit(x, y, 200, box + 4);
    if (hover) g_gui.hotItem = id;

    if (hover && g_gui.mouseDown && !g_gui.mouseWasDown) {
        checked = !checked;
        return true;
    }
    if (g_rendererForUI) {
        gl.glUseProgram(g_rendererForUI->progUI);
        ApplyOrtho(g_rendererForUI->progUI);
        DrawRect2D(x, y, box, box, Vec3(0.1f,0.11f,0.13f), 1);
        if (checked) DrawRect2D(x+2, y+2, box-4, box-4, Vec3(0.3f,0.7f,0.95f), 1);
        gl.glUseProgram(0);
    }
    Label(label, x + box + 6, y + 1, Vec3(0.85f,0.87f,0.9f), 0.92f);
    return false;
}

int Combo(const char* label, float x, float y, float w, const char* const* items, int count, int& current) {
    int id = GenId();
    float h = 20;
    bool hover = GUIRectHit(x, y, w, h);
    if (hover) g_gui.hotItem = id;

    // draw as button + arrow
    if (g_rendererForUI) {
        gl.glUseProgram(g_rendererForUI->progUI);
        ApplyOrtho(g_rendererForUI->progUI);
        Vec3 c = hover ? Vec3(0.2f,0.25f,0.32f) : Vec3(0.15f,0.17f,0.21f);
        DrawRect2D(x, y, w, h, c, 0.98f);
        // small triangle
        float ax = x + w - 14, ay = y + 7;
        // draw 3 lines for arrow
        DrawLine2D(ax, ay+1, ax+6, ay+1, Vec3(0.7f,0.75f,0.8f),1,1.2f);
        DrawLine2D(ax+1, ay+3, ax+5, ay+3, Vec3(0.7f,0.75f,0.8f),1,1.2f);
        gl.glUseProgram(0);
    }
    if (current >= 0 && current < count) Label(items[current], x + 6, y + 3, Vec3(0.9f,0.92f,0.95f), 0.9f);

    if (hover && g_gui.mouseDown && !g_gui.mouseWasDown) {
        current = (current + 1) % count;
        return 1;
    }
    return 0;
}

void Label(const char* text, float x, float y, Vec3 col, float sc) {
    if (g_rendererForUI) {
        // use text shader
        DrawText2D(x, y, text, col, sc);
    }
}

int ShapeGrid(float x, float y, int current, float btnW, float btnH) {
    int clicked = -1;
    float pad = 4;
    for (int i = 0; i < SHAPE_COUNT; ++i) {
        float cx = x + (i % 5) * (btnW + pad);
        float cy = y - (i / 5) * (btnH + pad);
        bool sel = (i == current);
        if (g_rendererForUI) {
            gl.glUseProgram(g_rendererForUI->progUI);
            ApplyOrtho(g_rendererForUI->progUI);
            Vec3 c = sel ? Vec3(0.20f, 0.42f, 0.68f) : Vec3(0.12f,0.14f,0.17f);
            DrawRect2D(cx, cy, btnW, btnH, c, 0.97f);
            // full border for clear button shape
            Vec3 brd = sel ? Vec3(0.55f,0.75f,0.95f) : Vec3(0.30f,0.33f,0.38f);
            float bw = 1.0f;
            DrawRect2D(cx, cy, btnW, bw, brd, 0.85f);
            DrawRect2D(cx, cy + btnH - bw, btnW, bw, brd, 0.85f);
            DrawRect2D(cx, cy, bw, btnH, brd, 0.85f);
            DrawRect2D(cx + btnW - bw, cy, bw, btnH, brd, 0.85f);
            gl.glUseProgram(0);
        }
        // center text (larger for readability with bigger phys size)
        float tsc = 0.95f;
        float tH = 16.0f * tsc;
        float ty = cy + (btnH - tH) * 0.5f - 0.5f;
        Label(g_shapeInfos[i].shortName, cx + 4, ty, sel ? Vec3(1.0f,1.0f,1.0f) : Vec3(0.88f,0.89f,0.92f), tsc);

        if (GUIRectHit(cx, cy, btnW, btnH) && g_gui.mouseDown && !g_gui.mouseWasDown) {
            clicked = i;
        }
    }
    return clicked;
}

int PresetBar(float x, float y, int currentPreset) {
    const char* names[6] = {"Classic", "Box-Ico", "Dod-Tet", "Cyl-Tor", "Oct-Cone", "Mixed"};
    float bw = 72, bh = 20;  // taller for readable text
    int newP = currentPreset;
    for (int i=0; i<6; ++i) {
        float px = x + i * (bw + 3);
        bool sel = (i == currentPreset);
        if (Button(names[i], px, y, bw, bh)) newP = i;
    }
    return newP;
}

// ===================== Font atlas via GDI (shader rendered) =====================

GLuint g_fontTex = 0;
int g_fontTexW = 0, g_fontTexH = 0;
float g_glyphU[128], g_glyphV[128], g_glyphW[128];

bool CreateFontAtlas(HDC hdcRef) {
    const int cols = 16;
    const int rows = 6;
    const int cw = 24;   // even larger cells for reliable glyph rendering without clipping or padding issues
    const int ch = 28;
    const int texW = cols * cw;
    const int texH = rows * ch;

    HDC memDC = CreateCompatibleDC(hdcRef);
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = texW;
    bmi.bmiHeader.biHeight = -texH;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP hbmp = CreateDIBSection(memDC, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, hbmp);

    RECT rc = {0,0,texW,texH};
    HBRUSH br = CreateSolidBrush(RGB(0,0,0));
    FillRect(memDC, &rc, br);
    DeleteObject(br);

    HFONT font = CreateFontA(ch - 8, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Consolas");
    HFONT oldF = (HFONT)SelectObject(memDC, font);
    SetTextColor(memDC, RGB(255,255,255));
    SetBkMode(memDC, TRANSPARENT);

    for (int c = 32; c < 127; ++c) {
        int col = (c - 32) % cols;
        int row = (c - 32) / cols;
        if (row >= rows) break;
        char chs[2] = {(char)c, 0};
        TextOutA(memDC, col * cw + 3, row * ch + 2, chs, 1);  // more padding
    }

    GdiFlush();  // ensure all GDI drawing is flushed to the DIB before reading bits

    SelectObject(memDC, oldF);
    DeleteObject(font);

    unsigned char* alpha = new unsigned char[texW * texH];
    for (int i = 0; i < texW * texH; ++i) {
        unsigned char* px = (unsigned char*)bits + i * 4;
        // For ANTIALIASED_QUALITY we get grayscale; average channels for good coverage mask.
        // px[0]=B, [1]=G, [2]=R (typical 32bpp DIB)
        int val = (px[2] + px[1] + px[0]) / 3;
        alpha[i] = (unsigned char)(val > 255 ? 255 : val);
    }

    if (g_fontTex) gl.glDeleteTextures(1, &g_fontTex);
    gl.glGenTextures(1, &g_fontTex);
    gl.glBindTexture(GL_TEXTURE_2D, g_fontTex);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl.glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, texW, texH, 0, GL_RED, GL_UNSIGNED_BYTE, alpha);
    gl.glBindTexture(GL_TEXTURE_2D, 0);

    delete[] alpha;

    g_fontTexW = texW;
    g_fontTexH = texH;

    for (int c = 0; c < 128; ++c) {
        if (c < 32 || c > 126) { g_glyphU[c] = 0; g_glyphV[c] = 0; g_glyphW[c] = 0; continue; }
        int idx = c - 32;
        int col = idx % cols;
        int row = idx / cols;
        g_glyphU[c] = float(col * cw) / texW;
        // Standard: v = 0 at top of image data (visual top row for row=0). We will invert in DrawText2D quad to map visual top to high v in our y-up ortho.
        float rowH = 1.0f / rows;
        g_glyphV[c] = float(row) * rowH;  // v for the top of this visual row strip in image space
        g_glyphW[c] = float(cw) / texW;
    }

    SelectObject(memDC, oldBmp);
    DeleteObject(hbmp);
    DeleteDC(memDC);
    return g_fontTex != 0;
}

void DestroyFontAtlas() {
    if (g_fontTex) {
        gl.glDeleteTextures(1, &g_fontTex);
        g_fontTex = 0;
    }
}

// Helper so text shader (in renderer) can get the current ortho matrix set on its program
void GUISetOrthoForProgram(GLuint prog) {
    GLint loc = gl.glGetUniformLocation(prog, "uOrtho");
    if (loc >= 0) gl.glUniformMatrix4fv(loc, 1, GL_FALSE, s_orthoM);
}
