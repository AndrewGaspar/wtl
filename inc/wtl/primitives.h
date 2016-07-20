#pragma once

#include <chrono>
#include <cstdint>

namespace wtl
{
    using dword_milliseconds = std::chrono::duration<std::uint32_t, std::milli>;
    constexpr auto infinite = dword_milliseconds(INFINITE);
}