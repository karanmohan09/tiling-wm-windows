#define _WIN32_DCOM
#define _WIN32_WINNT 0x0A00  // Target Windows 10
#include <windows.h>
#include "window_utils.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <shobjidl.h>   // IVirtualDesktopManager
#include <combaseapi.h> // CoInitializeEx, CoCreateInstance
#include <comdef.h>    // _com_error

const IID IID_IVirtualDesktopManager = {0xa5cd92ff, 0x29be, 0x454c, {0x8d, 0x04, 0xd8, 0x0b, 0xf9, 0x64, 0x1c, 0xa9}};
const CLSID CLSID_VirtualDesktopManager = {0xaa509086, 0x5ca9, 0x4c25, {0x8f, 0x95, 0x58, 0x9d, 0x3c, 0x07, 0xb4, 0x8a}};

bool is_window_on_current_virtual_desktop(HWND hwnd) {
    static IVirtualDesktopManager* vdm = nullptr;

    if (!vdm) {
        HRESULT hr = CoCreateInstance(
            CLSID_VirtualDesktopManager,
            nullptr,
            CLSCTX_ALL,
            IID_PPV_ARGS(&vdm)
        );
        if (FAILED(hr)) {
            std::cerr << "[!] Failed to create VirtualDesktopManager.\n";
            return true; // fallback: assume true
        }
    }

    BOOL onCurrent = FALSE;
    vdm->IsWindowOnCurrentVirtualDesktop(hwnd, &onCurrent);
    return onCurrent == TRUE;
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    std::vector<HWND>* windows = reinterpret_cast<std::vector<HWND>*>(lParam);
    if (!IsWindowVisible(hwnd) || IsIconic(hwnd)) return TRUE;
    if (!IsWindow(hwnd) || !IsWindowVisible(hwnd)) return TRUE;
    if (!is_window_on_current_virtual_desktop(hwnd)) return TRUE; // Skip windows not on current virtual desktop
    if (GetWindowLong(hwnd, GWL_STYLE) & WS_DISABLED) return TRUE; // Skip disabled windows
    if (GetWindowLong(hwnd, GWL_STYLE) & WS_POPUP) return TRUE; // Skip popups
    if (IsIconic(hwnd)) return TRUE; // Skip minimized
    if (GetAncestor(hwnd, GA_ROOT) != hwnd) return TRUE; // Skip owned/child windows
    if (hwnd == GetConsoleWindow()) return true;
    // Skip tool windows
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOOLWINDOW) return TRUE;

    // Skip this app's own window
    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == GetCurrentProcessId()) return TRUE;

    // Skip titleless windows
    char title[256];
    GetWindowTextA(hwnd, title, sizeof(title));
    std::string windowTitle = std::string(title);
    if (windowTitle.empty() || windowTitle.length() < 3) return TRUE;

    // Skip known non-user windows
    if (windowTitle == "Program Manager") return TRUE;
    if (windowTitle.find("Windows Default Lock Screen") != std::string::npos) return TRUE;
    if (windowTitle.find("Cortana") != std::string::npos) return TRUE;

    std::cout << "[+] Captured window: " << hwnd << " | Title: \"" << title << "\"\n";

    windows->push_back(hwnd);
    return TRUE;
}


std::vector<HWND> get_visible_windows() {
    std::vector<HWND> windows;
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&windows));
    return windows;
}

void tile_windows(const std::vector<HWND>& windows, LayoutType layout , float masterRatio) {
    if (windows.empty()) return;

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    size_t count = windows.size();

    switch (layout) {
    case LayoutType::MasterStack: {
        int masterW = static_cast<int>(screenWidth * masterRatio);
        SetWindowPos(windows[0], HWND_TOP, 0, 0, masterW, screenHeight, SWP_NOZORDER | SWP_NOACTIVATE |SWP_SHOWWINDOW);
        int stackCount = windows.size() - 1;
        int x = masterW;
        int w = screenWidth - masterW;
        int h = stackCount > 0 ? screenHeight / stackCount : screenHeight;
        int y = 0;
        for (size_t i = 1; i < count; ++i) {
            SetWindowPos(windows[i], HWND_TOP, x, y, w, h,SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
            y += h;
        }
        break;
    }
    case LayoutType::Grid: {
        int cols = std::ceil(std::sqrt(count));
        int rows = std::ceil(static_cast<float>(count) / cols);
        int cellW = screenWidth / cols;
        int cellH = screenHeight / rows;

        for (size_t i = 0; i < count; ++i) {
            int row = i / cols;
            int col = i % cols;
            SetWindowPos(windows[i], HWND_TOP,
                col * cellW, row * cellH,
                cellW, cellH, SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        }
        break;
    }
    case LayoutType::Monocle: {
        for (auto hwnd : windows) {
            SetWindowPos(hwnd, HWND_TOP, 0, 0, screenWidth, screenHeight, SWP_NOZORDER | SWP_NOACTIVATE |SWP_SHOWWINDOW);
        }
        break;
    }
    
    case LayoutType::EvenSplit: {
    int c = static_cast<int>(count);
    std::cout << "[EvenSplit] Number of windows: " << c << "\n";

    if (c == 1) {
        SetWindowPos(windows[0], HWND_TOP, 0, 0, screenWidth, screenHeight, SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }
    else if (c == 2) {
        // Side by side
        for (int i = 0; i < 2; ++i) {
            SetWindowPos(windows[i], HWND_TOP,
                         i * (screenWidth / 2), 0,
                         screenWidth / 2, screenHeight, SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        }
    }
    else if (c == 3) {
        // One on left half, two stacked on right half
        SetWindowPos(windows[0], HWND_TOP,
                     0, 0,
                     screenWidth / 2, screenHeight,SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);

        for (int i = 1; i < 3; ++i) {
            SetWindowPos(windows[i], HWND_TOP,
                         screenWidth / 2,
                         (i - 1) * (screenHeight / 2),
                         screenWidth / 2,
                         screenHeight / 2,
                         SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        }
    }
    else if (c == 4) {
        // 2x2 grid
        for (int i = 0; i < 4; ++i) {
            int row = i / 2;
            int col = i % 2;
            SetWindowPos(windows[i], HWND_TOP,
                         col * (screenWidth / 2),
                         row * (screenHeight / 2),
                         screenWidth / 2,
                         screenHeight / 2,
                         SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        }
    }
    else {
        std::cout << "[!] EvenSplit supports max 4 windows. Falling back to Grid.\n";
        int cols = std::ceil(std::sqrt(c));
        int rows = std::ceil(static_cast<float>(c) / cols);
        int cellW = screenWidth / cols;
        int cellH = screenHeight / rows;

        for (int i = 0; i < c; ++i) {
            int row = i / cols;
            int col = i % cols;
            SetWindowPos(windows[i], HWND_TOP,
                         col * cellW, row * cellH, cellW, cellH,SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        }
    }
    break;
}
    }
}
