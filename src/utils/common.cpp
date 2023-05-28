#include "utils/common.h"

namespace logger = spdlog;
#ifdef WIN32
#include <windows.h>

std::optional<std::filesystem::path> executable_location() {
    char location[512];
    DWORD rc = GetModuleFileNameA(nullptr, location, 512);
    DWORD error = GetLastError();
    if(rc == 512 && error == ERROR_INSUFFICIENT_BUFFER || rc == 0) {
        logger::error("Failed to get executable location: error code {}", error);
        return {};
    }
    return std::filesystem::path(location);
}
#else
std::optional<std::filesystem::path> executable_location() {
    return std::filesystem::canonical("/proc/self/exe");
}
#endif