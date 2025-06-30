@echo off
echo [*] Compiling project...

g++ src\main.cpp src\window_utils.cpp src\highlight.cpp src\mouse_utils.cpp -o wm.exe -lgdi32 -lole32 -luuid -std=c++17 -static 

echo [*] Build complete. Output: wm.exe
pause
