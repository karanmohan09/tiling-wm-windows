#pragma once
#include <windows.h>
#include <vector>

enum class LayoutType {
    MasterStack,
    Grid,
    Monocle,
    EvenSplit
};

std::vector<HWND> get_visible_windows();
void tile_windows(const std::vector<HWND>& windows,
    LayoutType layout = LayoutType::MasterStack,float masterRatio = 0.5f);
