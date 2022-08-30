#pragma once
#include <chrono>
#include <ctime>

struct date_info {
    unsigned int year, month, day;
    unsigned short hour, minutes, seconds;

    template <class T>
    static date_info from_point(std::chrono::time_point<T> point) {
        auto time = std::chrono::system_clock::to_time_t(point);
#pragma warning(disable:4996)
        auto local = *std::localtime(&time);
#pragma warning(default:4996)
        return {
            static_cast<unsigned int>(local.tm_year) + 1900,
            static_cast<unsigned int>(local.tm_mon) + 1,
            static_cast<unsigned int>(local.tm_mday),
            static_cast<unsigned short>(local.tm_hour),
            static_cast<unsigned short>(local.tm_min),
            static_cast<unsigned short>(local.tm_sec)
        };
    }
};
