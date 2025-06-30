#include <windows.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include "workspace.h"
#include "window_utils.h"
#include "logger.h"
#include <fstream>
#include <sstream>
#include "mouse_utils.h"
#include <set>
// This is a simple tiling window manager for Windows, allowing users to manage multiple workspaces and tile windows efficiently.
// Define 9 workspaces (1-based)
std::unordered_map<int, Workspace> workspaces;
int currentWorkspaceId = 1;

void register_hotkeys() {
    RegisterHotKey(NULL, 1, MOD_ALT, 'T');    // manual tile
    RegisterHotKey(NULL, 2, MOD_ALT, 'L');    // cycle layout
    RegisterHotKey(NULL, 3, MOD_ALT, 'J');    // focus next
    RegisterHotKey(NULL, 4, MOD_ALT, 'K');    // focus prev
    RegisterHotKey(NULL, 5, MOD_ALT, VK_RETURN); // promote
    RegisterHotKey(NULL, 6, MOD_ALT, 'F');    // toggle floating
    RegisterHotKey(NULL, 7, MOD_ALT, VK_OEM_PLUS);  // increase master
    RegisterHotKey(NULL, 8, MOD_ALT, VK_OEM_MINUS); // decrease master
    RegisterHotKey(NULL, 999, MOD_ALT, 'Q');  // quit
    for (int i = 1; i <= 9; ++i)
        RegisterHotKey(NULL, 100 + i, MOD_ALT, 0x30 + i); // workspaces 1-9
}

void switch_workspace(int newId) {
    if (newId == currentWorkspaceId) return;
    currentWorkspaceId = newId;

    if(workspaces.find(newId)== workspaces.end()) {
        std::cout << "[*] Workspace " << newId << " does not exist. Creating new workspace.\n";
        workspaces[newId] = Workspace();
    }

    workspaces[currentWorkspaceId].tile();
}

void load_config(float& ratio) {
    std::ifstream file("config.txt");
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key, val;
        if (std::getline(iss, key, '=') && std::getline(iss, val)) {
            if (key == "master_ratio") ratio = std::stof(val);
            else if (key == "log_enabled") LOGGING_ENABLED = (val == "true");
        }
    }
    log("Config loaded. Master ratio: " + std::to_string(ratio));
}

int main() {
    ShowWindow(GetConsoleWindow(), SW_HIDE);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    register_hotkeys();
    start_mouse_tracking();

    std::cout << "Tiling WM.Auto\n";

    // Load config for workspace 1
    float ratio = 0.5f;
    load_config(ratio);
    workspaces[1].masterRatio = ratio;
    workspaces[1].layout = LayoutType::EvenSplit;

    // Initial tile
    {
        auto visible = get_visible_windows();
        auto& ws = workspaces[currentWorkspaceId];
        for (auto hwnd : visible) ws.add(hwnd);
        ws.tile();
    }

    DWORD lastWindowScan = GetTickCount();
    DWORD lastHighlightCheck = GetTickCount();
    HWND lastForeground = GetForegroundWindow();
    bool running = true;

    while (running) {
        // Process all pending Windows messages
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
                break;
            }
            if (msg.message == WM_HOTKEY) {
                int id = msg.wParam;
                auto& ws = workspaces[currentWorkspaceId];
                switch (id) {
                    case 1: // Alt+T
                        {
                        ws.clear();
                        auto vis = get_visible_windows();
                        for (auto h : vis) ws.add(h);
                        ws.tile();
                        }
                        break;
                    case 2: ws.cycle_layout(); ws.tile(); break;
                    case 3: ws.focus_next(); break;
                    case 4: ws.focus_prev(); break;
                    case 5: ws.promote_focused(); ws.tile(); break;
                    case 6: ws.toggle_floating(); ws.tile(); break;
                    case 7: ws.increase_master(); ws.tile(); break;
                    case 8: ws.decrease_master(); ws.tile(); break;
                    case 999: // Alt+Q
                        running = false;
                        break;
                    default:
                        if (id >= 101 && id <= 109) switch_workspace(id - 100);
                }
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Auto-hide highlight (250ms)
        if (GetTickCount() - lastHighlightCheck > 250) {
            HWND current = GetForegroundWindow();
            auto& ws = workspaces[currentWorkspaceId];
            if (!ws.windows.empty()) {
                HWND focused = ws.windows[ws.focusedIndex].hwnd;
                if (current != lastForeground) {
                    if (current != focused) hide_highlight();
                    else show_highlight(focused);
                    lastForeground = current;
                }
            }
            lastHighlightCheck = GetTickCount();
        }

        // Auto-tile check (500ms)
        if (GetTickCount() - lastWindowScan > 500) {
            auto visible = get_visible_windows();
            auto& ws = workspaces[currentWorkspaceId];
            std::set<HWND> curr, vis;
            for (auto& w : ws.windows) if (IsWindow(w.hwnd)) curr.insert(w.hwnd);
            for (auto h : visible) if (IsWindow(h)) vis.insert(h);
            if (curr != vis) {
                std::cout << "[auto-tile] Change detected. Retiling...\n";
                ws.clear();
                for (auto h : visible) ws.add(h);
                ws.tile();
            }
            lastWindowScan = GetTickCount();
        }

        Sleep(10); // reduce CPU usage
    }

    stop_mouse_tracking();
    CoUninitialize();
    return 0;
}
