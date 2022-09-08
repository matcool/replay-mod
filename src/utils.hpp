#pragma once
#include "includes.h"
#include <vector>

template <typename T, typename R>
T cast(R const v) { return reinterpret_cast<T>(v); }

inline void patch(void* loc, std::vector<std::uint8_t> bytes) {
    auto size = bytes.size();
    DWORD old_prot;
    VirtualProtect(loc, size, PAGE_EXECUTE_READWRITE, &old_prot);
    memcpy(loc, bytes.data(), size);
    VirtualProtect(loc, size, old_prot, &old_prot);
}

template <typename R, typename U>
R& from_offset(U base, int offset) {
    return *cast<R*>(cast<intptr_t>(base) + offset);
}

// only use this when necessary
template <typename T, typename U>
T union_cast(U value) {
    union {
        T a;
        U b;
    } u;
    u.b = value;
    return u.a;
}
