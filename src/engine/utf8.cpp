#include "utf8.h"

namespace utf8
{
    int get_code_length(uint8_t byte)
    { 
        if(byte < 0x80) return 1;
        if(byte < 0xC0) return 0;
        if(byte < 0xE0) return 2;
        if(byte < 0xF0) return 3;
        if(byte < 0xF8) return 4;
        return 0;
    }

    bool is_continuation_byte(uint8_t byte)
    { 
        return byte >= 0x80 && byte < 0xC0; 
    }

    const char * prev(const char * units)
    {
        do { --units; } while(is_continuation_byte(*units));
        return units;
    }

    const char * next(const char * units)
    {
        return units + get_code_length(*units);
    }

    uint32_t code(const char * units)
    {
        static const uint8_t masks[] = {0, 0x7F, 0x1F, 0x0F, 0x07};
        auto length = get_code_length(*units);
        uint32_t codepoint = units[0] & masks[length];
        for(int i=1; i<length; ++i)
        {
            codepoint = (codepoint << 6) | (units[i] & 0x3F);
        }
        return codepoint;
    }

    std::array<char,5> units(uint32_t code)
    {
        if(code < 0x80) return {{code}};
        if(code < 0x800) return {{0xC0|((code>>6)&0x1F), 0x80|(code&0x3F)}};
        if(code < 0x10000) return {{0xE0|((code>>12)&0x0F), 0x80|((code>>6)&0x3F), 0x80|(code&0x3F)}};
        return {{0xF0|((code>>18)&0x07), 0x80|((code>>12)&0x3F), 0x80|((code>>6)&0x3F), 0x80|(code&0x3F)}};
    }

    bool is_valid(const char * units, size_t count)
    {
        auto end = units + count;
        while(units != end)
        {
            auto length = get_code_length(*units++);
            if(length == 0) return false;
            for(int i=1; i<length; ++i)
            {
                if(units == end) return false;
                if(!is_continuation_byte(*units++)) return false;
            }
        }
        return true;
    }
}