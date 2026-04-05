#include "pch.h"
#include "global.h"
#include <ShlObj.h>
#include <filesystem>

OPTIONS Options{};

std::wstring GetAiriDocumentsDirW()
{
    PWSTR docs = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_CREATE, nullptr, &docs)))
        return L"";

    std::filesystem::path dir(docs);
    CoTaskMemFree(docs);
    dir /= L"Airi";

    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir.wstring();
}

std::string GetAiriDocumentsDirA()
{
    const std::wstring wide = GetAiriDocumentsDirW();
    if (wide.empty())
        return {};

    const int bytes = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (bytes <= 1)
        return {};

    std::string result(static_cast<size_t>(bytes), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, result.data(), bytes, nullptr, nullptr);
    if (!result.empty() && result.back() == '\0')
        result.pop_back();
    return result;
}

std::wstring GetAiriSubdirW(const wchar_t* subdir)
{
    std::filesystem::path dir(GetAiriDocumentsDirW());
    if (subdir && *subdir)
        dir /= subdir;

    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir.wstring();
}

std::string GetAiriSubdirA(const char* subdir)
{
    std::filesystem::path dir(GetAiriDocumentsDirA());
    if (subdir && *subdir)
        dir /= subdir;

    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir.string();
}
