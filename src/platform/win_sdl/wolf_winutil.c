#include "wolf_winutil.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
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
#else
    (void)out;
    (void)out_sz;
    return 0;
#endif
}

void Wolf_ShowDataError(const char *message)
{
#ifdef _WIN32
    char buf[2048];
    snprintf(buf, sizeof(buf),
             "%s\n\n"
             "Fix one of:\n"
             "  - Put your WL6/WL1 folder next to wolf3d.exe as Wolf3dgame\\\n"
             "  - Or one folder up as ..\\Wolf3dgame\\ (repo layout)\n"
             "  - Or run:  wolf3d.exe --data \"D:\\path\\to\\pack\"\n"
             "  - Or set WOLF3D_DATA to that folder\n\n"
             "See docs\\player\\QUICKSTART.md",
             message && message[0] ? message
                                   : "Could not find Wolfenstein 3D game data.");
    MessageBoxA(NULL, buf, "Wolf3D Win64 - missing game data",
                MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
#else
    fprintf(stderr, "%s\n", message ? message : "missing game data");
#endif
}
