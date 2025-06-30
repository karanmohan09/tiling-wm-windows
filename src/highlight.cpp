#define _WIN32_WINNT 0x0601  // Windows 7+ API
#include "highlight.h"
#include <windows.h>
#undef min
#undef max
#include <stdlib.h>
#include <algorithm>
static HWND highlightWnd = nullptr;

LRESULT CALLBACK HighlightProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void create_highlight_window() {
    if (highlightWnd) return;

    const char* className = "WMHighlightOverlay";
    WNDCLASSA wc = {};
    wc.lpfnWndProc = HighlightProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = className;
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.style = CS_HREDRAW | CS_VREDRAW;

    RegisterClassA(&wc);

    highlightWnd = CreateWindowExA(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        className, "", WS_POPUP,
        0, 0, 0, 0,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );
}

void show_highlight(HWND target) {
    if (!target) return;

    create_highlight_window();

    RECT r;
    GetWindowRect(target, &r);

    const int border = 4;
    int width = (r.right - r.left) + 2 * border;
    int height = (r.bottom - r.top) + 2 * border;

    // Create 32-bit ARGB DIB
    HDC screenDC = GetDC(NULL);
    HDC memDC = CreateCompatibleDC(screenDC);
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP dib = CreateDIBSection(memDC, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    HGDIOBJ oldBmp = SelectObject(memDC, dib);


    DWORD* pixels = (DWORD*)bits;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int d = std::min(std::min(x, y), std::min(width - x - 1, height - y - 1));
            const int maxGlow = 4;
            if (d >= 0 && d <= maxGlow) {
                BYTE alpha = (BYTE)(255 * (1.0 - (float)d / maxGlow));  // Smooth gradient
                pixels[y * width + x] = (alpha << 24) | 0xFFFFFF;  // ARGB: translucent white
            }   
        }
    }

    POINT ptPos = { r.left - border, r.top - border };
    SIZE sizeWnd = { width, height };
    POINT ptSrc = { 0, 0 };
    BLENDFUNCTION blend = {};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;  // We rely on RGB (not per-pixel alpha)

    SetWindowPos(highlightWnd, target,
        ptPos.x, ptPos.y, width, height,
        SWP_NOACTIVATE | SWP_SHOWWINDOW);

    UpdateLayeredWindow(highlightWnd, screenDC, &ptPos, &sizeWnd,
                        memDC, &ptSrc, 0, &blend, ULW_ALPHA);

    SelectObject(memDC, oldBmp);
    DeleteObject(dib);
    DeleteDC(memDC);
    ReleaseDC(NULL, screenDC);
}

void hide_highlight() {
    if (highlightWnd) {
        ShowWindow(highlightWnd, SW_HIDE);
    }
}
