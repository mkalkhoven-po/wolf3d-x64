#include "wolf_winutil.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

int Wolf_GetExecutableDir(char *out, size_t out_sz)
{
    if (!out || out_sz < 4) {
        return 0;
    }
    out[0] = '\0';
#ifdef _WIN32
    {
        char path[MAX_PATH];
        DWORD n = GetModuleFileNameA(NULL, path, MAX_PATH);
        char *slash;
        if (n == 0 || n >= MAX_PATH) {
            return 0;
        }
        slash = strrchr(path, '\\');
        if (!slash) {
            slash = strrchr(path, '/');
        }
        if (!slash) {
            return 0;
        }
        *slash = '\0';
        if (strlen(path) + 1 > out_sz) {
            return 0;
        }
        memcpy(out, path, strlen(path) + 1);
        return 1;
    }
#elif defined(__linux__)
    {
        /* /proc/self/exe links to the running binary */
        char path[4096];
        const ssize_t n = readlink("/proc/self/exe", path, sizeof(path) - 1);
        char *slash;

        if (n <= 0) {
            return 0;
        }
        path[n] = '\0';
        slash = strrchr(path, '/');
        if (!slash || (size_t)(slash - path) + 1 > out_sz) {
            return 0;
        }
        *slash = '\0';
        memcpy(out, path, strlen(path) + 1);
        return 1;
    }
#else
    (void)out;
    (void)out_sz;
    return 0;
#endif
}
