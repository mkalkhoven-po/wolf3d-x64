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


void FB_DrawString(int x, int y, const char *text, int color)
{
    int cx = x;
    const char *p;

    for (p = text; *p; ++p) {
        char ch = *p;
        int row, col;
        uint8_t glyph[8];
        memset(glyph, 0, sizeof(glyph));

        if (ch >= 'a' && ch <= 'z') {
            ch = (char)(ch - 'a' + 'A');
        }

        switch (ch) {
        case ' ':
            break;
        case 'N':
            glyph[1] = 0xC6; glyph[2] = 0xE6; glyph[3] = 0xF6;
            glyph[4] = 0xDE; glyph[5] = 0xCE; glyph[6] = 0xC6;
            break;
        case 'E':
            glyph[1] = 0xFE; glyph[2] = 0xC0; glyph[3] = 0xFC;
            glyph[4] = 0xC0; glyph[5] = 0xC0; glyph[6] = 0xFE;
            break;
        case 'W':
            glyph[1] = 0xC6; glyph[2] = 0xC6; glyph[3] = 0xD6;
            glyph[4] = 0xD6; glyph[5] = 0xFE; glyph[6] = 0x6C;
            break;
        case 'G':
            glyph[1] = 0x7C; glyph[2] = 0xC6; glyph[3] = 0xC0;
            glyph[4] = 0xCE; glyph[5] = 0xC6; glyph[6] = 0x7C;
            break;
        case 'A':
            glyph[1] = 0x7C; glyph[2] = 0xC6; glyph[3] = 0xC6;
            glyph[4] = 0xFE; glyph[5] = 0xC6; glyph[6] = 0xC6;
            break;
        case 'M':
            glyph[1] = 0xC6; glyph[2] = 0xEE; glyph[3] = 0xFE;
            glyph[4] = 0xD6; glyph[5] = 0xC6; glyph[6] = 0xC6;
            break;
        case 'O':
            glyph[1] = 0x7C; glyph[2] = 0xC6; glyph[3] = 0xC6;
            glyph[4] = 0xC6; glyph[5] = 0xC6; glyph[6] = 0x7C;
            break;
        case 'P':
            glyph[1] = 0xFC; glyph[2] = 0xC6; glyph[3] = 0xC6;
            glyph[4] = 0xFC; glyph[5] = 0xC0; glyph[6] = 0xC0;
            break;
        case 'T':
            glyph[1] = 0xFE; glyph[2] = 0x18; glyph[3] = 0x18;
            glyph[4] = 0x18; glyph[5] = 0x18; glyph[6] = 0x18;
            break;
        case 'I':
            glyph[1] = 0x7E; glyph[2] = 0x18; glyph[3] = 0x18;
            glyph[4] = 0x18; glyph[5] = 0x18; glyph[6] = 0x7E;
            break;
        case 'S':
            glyph[1] = 0x7C; glyph[2] = 0xC0; glyph[3] = 0x7C;
            glyph[4] = 0x06; glyph[5] = 0x06; glyph[6] = 0x7C;
            break;
        case 'Q':
            glyph[1] = 0x7C; glyph[2] = 0xC6; glyph[3] = 0xC6;
            glyph[4] = 0xD6; glyph[5] = 0xCC; glyph[6] = 0x76;
            break;
        case 'U':
            glyph[1] = 0xC6; glyph[2] = 0xC6; glyph[3] = 0xC6;
            glyph[4] = 0xC6; glyph[5] = 0xC6; glyph[6] = 0x7C;
            break;
        case 'L':
            glyph[1] = 0xC0; glyph[2] = 0xC0; glyph[3] = 0xC0;
            glyph[4] = 0xC0; glyph[5] = 0xC0; glyph[6] = 0xFE;
            break;
        case 'D':
            glyph[1] = 0xF8; glyph[2] = 0xCC; glyph[3] = 0xC6;
            glyph[4] = 0xC6; glyph[5] = 0xCC; glyph[6] = 0xF8;
            break;
        case 'R':
            glyph[1] = 0xFC; glyph[2] = 0xC6; glyph[3] = 0xC6;
            glyph[4] = 0xFC; glyph[5] = 0xD8; glyph[6] = 0xCE;
            break;
        case 'Y':
            glyph[1] = 0xC6; glyph[2] = 0xC6; glyph[3] = 0x7C;
            glyph[4] = 0x18; glyph[5] = 0x18; glyph[6] = 0x18;
            break;
        case 'V':
            glyph[1] = 0xC6; glyph[2] = 0xC6; glyph[3] = 0xC6;
            glyph[4] = 0xC6; glyph[5] = 0x6C; glyph[6] = 0x38;
            break;
        case 'H':
            glyph[1] = 0xC6; glyph[2] = 0xC6; glyph[3] = 0xFE;
            glyph[4] = 0xC6; glyph[5] = 0xC6; glyph[6] = 0xC6;
            break;
        case 'C':
            glyph[1] = 0x7C; glyph[2] = 0xC6; glyph[3] = 0xC0;
            glyph[4] = 0xC0; glyph[5] = 0xC6; glyph[6] = 0x7C;
            break;
        case 'F':
            glyph[1] = 0xFE; glyph[2] = 0xC0; glyph[3] = 0xFC;
            glyph[4] = 0xC0; glyph[5] = 0xC0; glyph[6] = 0xC0;
            break;
        case 'B':
            glyph[1] = 0xFC; glyph[2] = 0xC6; glyph[3] = 0xFC;
            glyph[4] = 0xC6; glyph[5] = 0xC6; glyph[6] = 0xFC;
            break;
        case 'K':
                    glyph[1] = 0xCE; glyph[2] = 0xDC; glyph[3] = 0xF8;
                    glyph[4] = 0xF8; glyph[5] = 0xDC; glyph[6] = 0xCE;
                    break;
                case 'X':
                    glyph[1] = 0xC6; glyph[2] = 0x6C; glyph[3] = 0x38;
                    glyph[4] = 0x38; glyph[5] = 0x6C; glyph[6] = 0xC6;
                    break;
                case 'Z':
                    glyph[1] = 0xFE; glyph[2] = 0x0C; glyph[3] = 0x18;
                    glyph[4] = 0x30; glyph[5] = 0x60; glyph[6] = 0xFE;
                    break;
                case 'J':
                    glyph[1] = 0x1E; glyph[2] = 0x0C; glyph[3] = 0x0C;
                    glyph[4] = 0x0C; glyph[5] = 0xCC; glyph[6] = 0x78;
                    break;
                case '0':
                    glyph[1] = 0x7C; glyph[2] = 0xC6; glyph[3] = 0xCE;
                    glyph[4] = 0xD6; glyph[5] = 0xE6; glyph[6] = 0x7C;
                    break;
                case '1':
                    glyph[1] = 0x18; glyph[2] = 0x38; glyph[3] = 0x18;
                    glyph[4] = 0x18; glyph[5] = 0x18; glyph[6] = 0x7E;
                    break;
                case '2':
                    glyph[1] = 0x7C; glyph[2] = 0xC6; glyph[3] = 0x0C;
                    glyph[4] = 0x38; glyph[5] = 0x60; glyph[6] = 0xFE;
                    break;
                case '3':
                    glyph[1] = 0x7C; glyph[2] = 0xC6; glyph[3] = 0x1C;
                    glyph[4] = 0x06; glyph[5] = 0xC6; glyph[6] = 0x7C;
                    break;
                case '4':
                    glyph[1] = 0x0C; glyph[2] = 0x1C; glyph[3] = 0x3C;
                    glyph[4] = 0x6C; glyph[5] = 0xFE; glyph[6] = 0x0C;
                    break;
                case '5':
                    glyph[1] = 0xFE; glyph[2] = 0xC0; glyph[3] = 0xFC;
                    glyph[4] = 0x06; glyph[5] = 0xC6; glyph[6] = 0x7C;
                    break;
                case '6':
                    glyph[1] = 0x3C; glyph[2] = 0x60; glyph[3] = 0xFC;
                    glyph[4] = 0xC6; glyph[5] = 0xC6; glyph[6] = 0x7C;
                    break;
                case '7':
                    glyph[1] = 0xFE; glyph[2] = 0xC6; glyph[3] = 0x0C;
                    glyph[4] = 0x18; glyph[5] = 0x30; glyph[6] = 0x30;
                    break;
                case '8':
                    glyph[1] = 0x7C; glyph[2] = 0xC6; glyph[3] = 0x7C;
                    glyph[4] = 0xC6; glyph[5] = 0xC6; glyph[6] = 0x7C;
                    break;
                case '9':
                    glyph[1] = 0x7C; glyph[2] = 0xC6; glyph[3] = 0xC6;
                    glyph[4] = 0x7E; glyph[5] = 0x0C; glyph[6] = 0x78;
                    break;
                case '-':
                            glyph[3] = 0x7E; glyph[4] = 0x7E;
                            break;
                        case '=':
                            glyph[2] = 0x7E; glyph[4] = 0x7E;
                            break;
                        case '+':
                            glyph[2] = 0x18; glyph[3] = 0x7E; glyph[4] = 0x18;
                            break;
                case '/':
                    glyph[1] = 0x06; glyph[2] = 0x0C; glyph[3] = 0x18;
                    glyph[4] = 0x30; glyph[5] = 0x60; glyph[6] = 0xC0;
                    break;
                case ':':
                    glyph[2] = 0x18; glyph[5] = 0x18;
                    break;
                case '!':
                    glyph[1] = 0x18; glyph[2] = 0x18; glyph[3] = 0x18;
                    glyph[4] = 0x18; glyph[5] = 0x00; glyph[6] = 0x18;
                    break;
                case '.':
                    glyph[5] = 0x18; glyph[6] = 0x18;
                    break;
                default:
                    /* Unknown glyph: small box, not a fake zero/"O". */
                    glyph[1] = 0x7E; glyph[2] = 0x42; glyph[3] = 0x42;
                    glyph[4] = 0x42; glyph[5] = 0x42; glyph[6] = 0x7E;
                    break;
                }

        for (row = 0; row < 8; ++row) {
            for (col = 0; col < 8; ++col) {
                if (glyph[row] & (0x80 >> col)) {
                    FB_Plot(cx + col, y + row, color);
                }
            }
        }
        cx += 9;
    }
}
