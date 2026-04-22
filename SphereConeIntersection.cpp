// SphereConeIntersection.cpp
// Win32 GDI Application demonstrating sphere-cone intersection
// Build with Visual Studio

#include <windows.h>
#include <math.h>
#include <stdio.h>

#define PI 3.14159265358979323846

// Global variables
HWND g_hWnd = NULL;
HDC g_hdcMemory = NULL;
HBITMAP g_hBitmap = NULL;
HBITMAP g_hOldBitmap = NULL;

// Animation parameters
double g_conePosition = -300.0;
double g_coneSpeed = 2.0;
bool g_bMovingForward = true;

// Sphere parameters (center and radius)
const double SPHERE_CENTER_X = 400.0;
const double SPHERE_CENTER_Y = 300.0;
const double SPHERE_RADIUS = 100.0;

// Cone parameters
const double CONE_BASE_RADIUS = 80.0;
const double CONE_HEIGHT = 200.0;
const double CONE ApexAngle = atan(CONE_BASE_RADIUS / CONE_HEIGHT);

// Timer ID
#define IDT_TIMER1 1

// Function prototypes
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void DrawScene(HDC hdc);
void DrawSphere(HDC hdc, double centerX, double centerY, double radius);
void DrawCone(HDC hdc, double apexX, double apexY, double baseRadius, double height);
void DrawIntersection(HDC hdc, double coneApexX, double coneApexY);
void CalculateIntersection(double coneApexX, double coneApexY, double* intersectionPoints, int& pointCount);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSEX wc = {0};
    MSG msg;
    
    // Register window class
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = L"SphereConeIntersection";
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    
    if (!RegisterClassEx(&wc))
    {
        MessageBox(NULL, L"Window Registration Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    // Create window
    g_hWnd = CreateWindowEx(
        0,
        L"SphereConeIntersection",
        L"Sphere-Cone Intersection (Win32 GDI)",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        NULL, NULL, hInstance, NULL
    );
    
    if (!g_hWnd)
    {
        MessageBox(NULL, L"Window Creation Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);
    
    // Message loop
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:
            // Create off-screen bitmap for double buffering
            HDC hdc = GetDC(hwnd);
            g_hdcMemory = CreateCompatibleDC(hdc);
            g_hBitmap = CreateCompatibleBitmap(hdc, 800, 600);
            g_hOldBitmap = (HBITMAP)SelectObject(g_hdcMemory, g_hBitmap);
            ReleaseDC(hwnd, hdc);
            
            // Start timer for animation
            SetTimer(hwnd, IDT_TIMER1, 30, NULL);
            return 0;
            
        case WM_TIMER:
            if (wParam == IDT_TIMER1)
            {
                // Update cone position
                if (g_bMovingForward)
                {
                    g_conePosition += g_coneSpeed;
                    if (g_conePosition > 500.0)
                        g_bMovingForward = false;
                }
                else
                {
                    g_conePosition -= g_coneSpeed;
                    if (g_conePosition < -300.0)
                        g_bMovingForward = true;
                }
                
                // Redraw
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
            
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            // Draw to off-screen buffer
            DrawScene(g_hdcMemory);
            
            // Copy to screen
            BitBlt(hdc, 0, 0, 800, 600, g_hdcMemory, 0, 0, SRCCOPY);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_DESTROY:
            KillTimer(hwnd, IDT_TIMER1);
            SelectObject(g_hdcMemory, g_hOldBitmap);
            DeleteObject(g_hBitmap);
            DeleteDC(g_hdcMemory);
            PostQuitMessage(0);
            return 0;
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void DrawScene(HDC hdc)
{
    // Clear background
    HBRUSH hBrushWhite = CreateSolidBrush(RGB(255, 255, 255));
    RECT rect = {0, 0, 800, 600};
    FillRect(hdc, &rect, hBrushWhite);
    DeleteObject(hBrushWhite);
    
    // Draw title
    SetBkMode(hdc, TRANSPARENT);
    HFONT hFont = CreateFont(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    SetTextColor(hdc, RGB(0, 0, 0));
    TextOut(hdc, 200, 20, L"Sphere-Cone Intersection", 24);
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
    
    // Draw sphere (fixed position)
    DrawSphere(hdc, SPHERE_CENTER_X, SPHERE_CENTER_Y, SPHERE_RADIUS);
    
    // Draw cone (moving)
    double coneApexX = SPHERE_CENTER_X + g_conePosition;
    double coneApexY = SPHERE_CENTER_Y;
    DrawCone(hdc, coneApexX, coneApexY, CONE_BASE_RADIUS, CONE_HEIGHT);
    
    // Draw intersection curve
    DrawIntersection(hdc, coneApexX, coneApexY);
    
    // Draw info text
    wchar_t infoText[256];
    swprintf(infoText, 256, L"Cone Position: %.1f", g_conePosition);
    TextOut(hdc, 20, 550, infoText, wcslen(infoText));
    
    swprintf(infoText, 256, L"Distance to Sphere: %.1f", fabs(g_conePosition));
    TextOut(hdc, 20, 575, infoText, wcslen(infoText));
}

void DrawSphere(HDC hdc, double centerX, double centerY, double radius)
{
    // Draw sphere as a circle with shading
    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(0, 100, 200));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    
    HBRUSH hBrush = CreateHatchBrush(HS_DIAGCROSS, RGB(200, 230, 255));
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
    
    Ellipse(hdc, (int)(centerX - radius), (int)(centerY - radius),
            (int)(centerX + radius), (int)(centerY + radius));
    
    SelectObject(hdc, hOldPen);
    SelectObject(hdc, hOldBrush);
    DeleteObject(hPen);
    DeleteObject(hBrush);
    
    // Draw center point
    SetPixel(hdc, (int)centerX, (int)centerY, RGB(255, 0, 0));
}

void DrawCone(HDC hdc, double apexX, double apexY, double baseRadius, double height)
{
    // Cone apex is at (apexX, apexY), base is to the left
    double baseX = apexX - height;
    double baseTopY = apexY - baseRadius;
    double baseBottomY = apexY + baseRadius;
    
    // Draw cone outline
    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(200, 100, 0));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    
    HBRUSH hBrush = CreateHatchBrush(HS_BDIAGONAL, RGB(255, 230, 200));
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
    
    // Create polygon for cone
    POINT points[4];
    points[0].x = (int)apexX;
    points[0].y = (int)apexY;
    points[1].x = (int)baseX;
    points[1].y = (int)baseTopY;
    points[2].x = (int)baseX;
    points[2].y = (int)baseBottomY;
    points[3].x = (int)apexX;
    points[3].y = (int)apexY;
    
    Polygon(hdc, points, 4);
    
    // Draw base ellipse
    Ellipse(hdc, (int)(baseX - 5), (int)(baseTopY),
            (int)(baseX + 5), (int)(baseBottomY));
    
    SelectObject(hdc, hOldPen);
    SelectObject(hdc, hOldBrush);
    DeleteObject(hPen);
    DeleteObject(hBrush);
    
    // Draw apex point
    SetPixel(hdc, (int)apexX, (int)apexY, RGB(255, 0, 0));
}

void CalculateIntersection(double coneApexX, double coneApexY, double* intersectionPoints, int& pointCount)
{
    pointCount = 0;
    
    // Simple 2D intersection calculation
    // For each angle around the sphere, check if the point is inside the cone
    
    double dx = coneApexX - SPHERE_CENTER_X;
    double distance = sqrt(dx * dx); // Only horizontal distance matters in 2D
    
    // If cone is too far, no intersection
    if (distance > SPHERE_RADIUS + CONE_HEIGHT)
        return;
    
    // Generate intersection points by sampling around the sphere
    int numSamples = 100;
    for (int i = 0; i < numSamples; i++)
    {
        double angle = 2.0 * PI * i / numSamples;
        double px = SPHERE_CENTER_X + SPHERE_RADIUS * cos(angle);
        double py = SPHERE_CENTER_Y + SPHERE_RADIUS * sin(angle);
        
        // Check if point is inside cone
        // Cone equation: point is inside if distance from axis <= radius at that height
        double distFromApexX = coneApexX - px;
        
        if (distFromApexX > 0 && distFromApexX < CONE_HEIGHT)
        {
            double coneRadiusAtPoint = (distFromApexX / CONE_HEIGHT) * CONE_BASE_RADIUS;
            double distFromAxis = fabs(py - coneApexY);
            
            if (distFromAxis <= coneRadiusAtPoint)
            {
                intersectionPoints[pointCount * 2] = px;
                intersectionPoints[pointCount * 2 + 1] = py;
                pointCount++;
                
                if (pointCount >= 50) break; // Limit points
            }
        }
    }
}

void DrawIntersection(HDC hdc, double coneApexX, double coneApexY)
{
    double intersectionPoints[100];
    int pointCount = 0;
    
    CalculateIntersection(coneApexX, coneApexY, intersectionPoints, pointCount);
    
    if (pointCount < 2)
        return;
    
    // Draw intersection curve
    HPEN hPen = CreatePen(PS_SOLID, 3, RGB(255, 0, 255));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    
    // Draw lines between consecutive points
    MoveToEx(hdc, (int)intersectionPoints[0], (int)intersectionPoints[1], NULL);
    
    for (int i = 1; i < pointCount; i++)
    {
        LineTo(hdc, (int)intersectionPoints[i * 2], (int)intersectionPoints[i * 2 + 1]);
    }
    
    // Close the curve
    LineTo(hdc, (int)intersectionPoints[0], (int)intersectionPoints[1]);
    
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
    
    // Draw label
    SetBkMode(hdc, TRANSPARENT);
    HFONT hFont = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    SetTextColor(hdc, RGB(255, 0, 255));
    TextOut(hdc, (int)coneApexX - 50, (int)coneApexY - 100, L"Intersection Curve", 18);
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}
