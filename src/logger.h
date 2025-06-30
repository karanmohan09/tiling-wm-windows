#pragma once
#include <iostream>
#include <string>

inline bool LOGGING_ENABLED = true;

inline void log(const std::string& msg) {
    if (LOGGING_ENABLED)
        std::cout << "[LOG] " << msg << "\n";
}
