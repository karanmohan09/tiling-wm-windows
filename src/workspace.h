#pragma once
#include <windows.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include "window_utils.h"
#include "highlight.h"
class Workspace {
public:
    
    struct ManagedWindow {
        HWND hwnd;
        bool floating = false;
    };

    std::vector<ManagedWindow> windows;

    LayoutType layout = LayoutType::MasterStack;

    int focusedIndex = 0;

    void add(HWND hwnd) {
        auto it = std::find_if(windows.begin(), windows.end(),
            [hwnd](const ManagedWindow& w) { return w.hwnd == hwnd; });
        if (it == windows.end()) {
            windows.push_back({ hwnd });
        }
    }

    float masterRatio = 0.5f;

    void clear() {
        windows.clear();
        focusedIndex = 0;
        hide_highlight();
    }

    void tile() {
    std::vector<HWND> tiling;
    for (auto& win : windows) {
        if (!win.floating && IsWindow(win.hwnd) && IsWindowVisible(win.hwnd) && !IsIconic(win.hwnd)) 
            tiling.push_back(win.hwnd);
    }

    std::cout << "[tile] Tiling " << tiling.size() << " windows using layout: ";
    switch (layout) {
        case LayoutType::MasterStack: std::cout << "MasterStack\n"; break;
        case LayoutType::Grid: std::cout << "Grid\n"; break;
        case LayoutType::Monocle: std::cout << "Monocle\n"; break;
        case LayoutType::EvenSplit: std::cout << "EvenSplit\n"; break;
    }

     LayoutType effectiveLayout = layout;
    if (layout == LayoutType::EvenSplit && tiling.size() > 4) {
        std::cout << "[*] Too many windows for EvenSplit. Falling back to Grid.\n";
        effectiveLayout = LayoutType::Grid;
    }

    tile_windows(tiling, effectiveLayout, masterRatio);
    if(focusedIndex >=0 &&focusedIndex < (int)windows.size()) {
        show_highlight(windows[focusedIndex].hwnd);
    }
}


    void cycle_layout() {
        layout = static_cast<LayoutType>((static_cast<int>(layout) + 1) % 4);
        std::cout << "[*] Switched to layout: ";
        switch (layout) {
        case LayoutType::MasterStack: std::cout << "MasterStack\n"; break;
        case LayoutType::Grid: std::cout << "Grid\n"; break;
        case LayoutType::Monocle: std::cout << "Monocle\n"; break;
        case LayoutType::EvenSplit: std::cout << "EvenSplit\n"; break;
        }
    }

    void focus_next() {
        if (windows.empty()) return;
        focusedIndex = (focusedIndex + 1) % windows.size();
        SetForegroundWindow(windows[focusedIndex].hwnd);
        std::cout << "[*] Focused window " << focusedIndex << "\n";
    }

    void focus_prev() {
        if (windows.empty()) return;
        focusedIndex = (focusedIndex - 1 + windows.size()) % windows.size();
        SetForegroundWindow(windows[focusedIndex].hwnd);
        std::cout << "[*] Focused window " << focusedIndex << "\n";
    }

    void promote_focused() {
        if (windows.empty() || focusedIndex == 0) return;
        std::swap(windows[0], windows[focusedIndex]);
        focusedIndex = 0;
        std::cout << "[*] Promoted window to master\n";
    }

    void toggle_floating() {
        if (windows.empty()) return;
        auto& win = windows[focusedIndex];
        win.floating = !win.floating;
        std::cout << "[*] " << (win.floating ? "Floating" : "Tiling") << " window " << focusedIndex << "\n";
    }

    void increase_master() {
        if (masterRatio < 0.9f) {
            masterRatio += 0.05f;
            std::cout << "[*] Increased master area to " << masterRatio << "\n";
        }
    }

    void decrease_master() {
        if (masterRatio > 0.1f) {
            masterRatio -= 0.05f;
            std::cout << "[*] Decreased master area to " << masterRatio << "\n";
        }
    }
};
