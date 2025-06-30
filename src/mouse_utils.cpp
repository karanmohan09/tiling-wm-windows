#include "mouse_utils.h"
#include "workspace.h"
#include "window_utils.h"
#include <windows.h>
#include <iostream>
#include <unordered_map>
extern std::unordered_map<int, Workspace> workspaces;
extern int currentWorkspaceId;

static HHOOK mouseHook = nullptr;
static bool dragging = false;
static HWND dragWindow = nullptr;
static HWND currentTarget = nullptr;

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        MSLLHOOKSTRUCT* mouse = (MSLLHOOKSTRUCT*)lParam;

        if (wParam == WM_LBUTTONDOWN) {
            HWND rawHwnd = WindowFromPoint(mouse->pt);
            HWND hwnd = GetAncestor(rawHwnd, GA_ROOT);

            auto& ws = workspaces[currentWorkspaceId];
            for (auto& win : ws.windows) {
                if (win.hwnd == hwnd) {
                    dragWindow = hwnd;
                    dragging = true;
                    currentTarget = nullptr;
                    std::cout << "[mouse] Start drag: " << hwnd << "\n";
                    break;
                }
            }
        }

        else if (wParam == WM_MOUSEMOVE && dragging) {
            HWND rawHwnd = WindowFromPoint(mouse->pt);
            HWND hovered = GetAncestor(rawHwnd, GA_ROOT);
            if (hovered != dragWindow) {
                currentTarget = hovered;
            }
        }

        else if (wParam == WM_LBUTTONUP && dragging) {
            dragging = false;

            if (!dragWindow || !currentTarget) {
                std::cout << "[mouse] Invalid drag or target.\n";
                dragWindow = nullptr;
                currentTarget = nullptr;
                return CallNextHookEx(mouseHook, nCode, wParam, lParam);
            }

            auto& ws = workspaces[currentWorkspaceId];
            int dragIndex = -1, dropIndex = -1;

            for (int i = 0; i < ws.windows.size(); ++i) {
                if (ws.windows[i].hwnd == dragWindow) dragIndex = i;
                if (ws.windows[i].hwnd == currentTarget) dropIndex = i;
            }

            if (dragIndex != -1 && dropIndex != -1 && dragIndex != dropIndex) {
                std::cout << "[mouse] Swapping index " << dragIndex << " <-> " << dropIndex << "\n";
                std::swap(ws.windows[dragIndex], ws.windows[dropIndex]);
                ws.focusedIndex = dropIndex;
                ws.tile();
                SetForegroundWindow(ws.windows[ws.focusedIndex].hwnd);
                show_highlight(ws.windows[ws.focusedIndex].hwnd);
            }
            else {
                std::cout << "[mouse] Swap failed or redundant.\n";
            }
            dragWindow = nullptr;
            currentTarget = nullptr;
        }
    }

    return CallNextHookEx(mouseHook, nCode, wParam, lParam);
}

void start_mouse_tracking() {
    if (!mouseHook) {
        mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, NULL, 0);
        std::cout << "[mouse] Mouse tracking started.\n";
    }
}

void stop_mouse_tracking() {
    if (mouseHook) {
        UnhookWindowsHookEx(mouseHook);
        mouseHook = nullptr;
        std::cout << "[mouse] Mouse tracking stopped.\n";
    }
}
