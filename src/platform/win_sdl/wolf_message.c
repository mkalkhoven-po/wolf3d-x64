#include "wolf_message.h"

#include <stdio.h>

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL.h>

#ifdef _WIN32
#define EXE "wolf3d.exe"
#define DATA_ARG "\"D:\\path\\to\\your\\files\""
#else
#define EXE "wolf3d"
#define DATA_ARG "~/path/to/your/files"
#endif

void Wolf_ShowDataError(const char *message)
{
    char buf[2048];

    snprintf(buf, sizeof(buf),
             "%s\n\n"
             "Fix one of:\n"
             "  - Put your Wolfenstein 3-D game files (the shareware or the full\n"
             "    version) in a folder named Wolf3dgame next to " EXE "\n"
             "  - Or run:  " EXE " --data " DATA_ARG "\n"
             "  - Or set WOLF3D_DATA to that folder\n\n"
             "See docs/player/QUICKSTART.md",
             message && message[0] ? message : "Could not find the Wolfenstein 3-D game files.");
    fprintf(stderr, "%s\n", buf);
    /* SDL's message box works before SDL_Init; without a display it just fails */
    (void)SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Wolf3D x64 - missing game files", buf,
                                   NULL);
}
