#pragma once
#include <fmt/core.h>

template <class T, class... Args>
void log(T format, Args... args) {
    std::cout << fmt::format(format, args...);
    std::cout.flush();
}

template <class T, class... Args>
void logln(T format, Args... args) {
    std::cout << fmt::format(format, args...) << std::endl;
}