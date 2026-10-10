#include "id_vh.h"

#include <string.h>

#include "id_ca.h"
#include "id_vl.h"
#include "gfxv.h"

void VH_Startup(void)
{
}

void VH_Shutdown(void)
{
}

void VH_UpdateScreen(void)
{
}


void FB_DrawPic(int x, int y, int chunknum)
{
    int picnum;
    int width, height;
    const byte *src;
    int yy, xx;

    if (!pictable || chunknum < STARTPICS) {
        return;
    }
    picnum = chunknum - STARTPICS;
    if (picnum < 0 || picnum >= NUMPICS) {
        return;
    }

    CA_CacheGrChunk(chunknum);
    src = grsegs[chunknum];
    if (!src) {
        return;
    }
    width = pictable[picnum].width;
    height = pictable[picnum].height;
    if (width <= 0 || height <= 0 || !framebuffer) {
        return;
    }

    for (yy = 0; yy < height; ++yy) {
        int dy = y + yy;
        if ((unsigned)dy >= SCREENHEIGHT) {
            continue;
        }
        for (xx = 0; xx < width; ++xx) {
            int dx = x + xx;
            if ((unsigned)dx >= SCREENWIDTH) {
                continue;
            }
            framebuffer[dy * SCREENWIDTH + dx] = src[yy * width + xx];
        }
    }
}
