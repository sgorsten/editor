#ifndef ENGINE_UTF8_H
#define ENGINE_UTF8_H

#include <cstdint>
#include <array>

namespace utf8
{
    const char * prev(const char * units); // Assumes units points just past the end of a valid utf-8 sequence of code units
    const char * next(const char * units); // Assumes units points to the start of a valid utf-8 sequence of code units
    uint32_t code(const char * units); // Assumes units points to the start of a valid utf-8 sequence of code units
    std::array<char,5> units(uint32_t code); // Assumes code < 0x110000
    bool is_valid(const char * units, size_t count); // Return true if the given sequence of code units is valid utf-8
}

#endif