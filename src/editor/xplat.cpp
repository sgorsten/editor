#include "xplat.h"
#include <cassert>
#include <sstream>

#ifdef WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Commdlg.h>

std::wstring UtfToWin(const std::string & utf)
{
    if(utf.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf.data(), utf.size(), nullptr, 0);
    if(len == 0) throw std::runtime_error("Invalid UTF-8 encoded string: " + utf);
    std::wstring win(len, ' ');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf.data(), utf.size(), &win[0], win.size());
    return win;
}

std::string ChooseFile(const std::vector<FileType> & types, bool mustExist)
{
    std::ostringstream ss;
    for(auto & type : types) { ss << type.type << " (*." << type.extension << ")" << '\0' << "*." << type.extension << '\0'; }
    auto filter = UtfToWin(ss.str());

    wchar_t buffer[MAX_PATH] = {};

    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = buffer;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = filter.c_str();
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST; // OFN_ALLOWMULTISELECT
    if(mustExist) ofn.Flags |= OFN_FILEMUSTEXIST; 

    if ((mustExist ? GetOpenFileName : GetSaveFileName)(&ofn) == TRUE)
    {
        size_t len = wcstombs(nullptr, buffer, 0);
        if(len == size_t(-1)) throw std::runtime_error("Invalid path.");
        std::string result(len,' ');
        len = wcstombs(&result[0], buffer, result.size());
        assert(len == result.size());
        return result;
    }
    else return {};
}

#endif