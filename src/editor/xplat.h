#ifndef EDITOR_XPLAT_H
#define EDITOR_XPLAT_H

#include <string>
#include <vector>

struct FileType { std::string type, extension; };
std::string ChooseFile(const std::vector<FileType> & types, bool mustExist);

#endif