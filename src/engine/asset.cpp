#include "asset.h"

std::string LoadTextFile(const std::string & filename)
{
    std::string contents;
    FILE * f = fopen(filename.c_str(), "rb");
    if(f)
    {
        fseek(f, 0, SEEK_END);
        auto len = ftell(f);
        contents.resize(len);
        fseek(f, 0, SEEK_SET);
        fread(&contents[0], 1, contents.size(), f);
        fclose(f);
    }
    return contents;
}