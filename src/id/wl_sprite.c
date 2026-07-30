#include "wl_sprite.h"

#include <stdio.h>
#include <string.h>

#include "id_pm.h"

boolean Wolf_SpriteExpand(int shapenum, byte out[64 * 64])
{
    const byte *shape;
    int max_shape;
    int page;
    word leftpix, rightpix;
    word page_len;
    int col;
    int n;

    if (!out || !PMPages || PMSoundStart <= PMSpriteStart) {
        return false;
    }
    memset(out, 0xFF, 64 * 64);
    max_shape = (int)PMSoundStart - (int)PMSpriteStart - 1;
    if (shapenum < 0 || shapenum > max_shape) {
        return false;
    }
    page = (int)PMSpriteStart + shapenum;
    if ((unsigned)page >= ChunksInFile) {
        return false;
    }
    page_len = PMPages[page].length;
    if (page_len < 8) {
        return false;
    }
    shape = (const byte *)PM_GetPage(page);
    if (!shape) {
        return false;
    }
    leftpix = (word)(shape[0] | (shape[1] << 8));
    rightpix = (word)(shape[2] | (shape[3] << 8));
    if (rightpix < leftpix || rightpix >= 64) {
        return false;
    }
    n = (int)rightpix - (int)leftpix + 1;
    if (4 + n * 2 > (int)page_len) {
        return false;
    }
    for (col = 0; col < n; ++col) {
        int x = (int)leftpix + col;
        const byte *cmd;
        word col_ofs;
        int cmd_limit;

        if ((unsigned)x >= 64u) {
            continue;
        }
        col_ofs = (word)(shape[4 + col * 2] | (shape[5 + col * 2] << 8));
        if ((int)col_ofs + 6 > (int)page_len) {
            continue;
        }
        cmd = shape + col_ofs;
        cmd_limit = (int)page_len;
        for (;;) {
            word endy, newstart, starty;
            unsigned j;
            int off = (int)(cmd - shape);

            if (off + 6 > cmd_limit) {
                break;
            }
            endy = (word)(cmd[0] | (cmd[1] << 8));
            cmd += 2;
            if (endy == 0) {
                break;
            }
            endy >>= 1;
            newstart = (word)(cmd[0] | (cmd[1] << 8));
            cmd += 2;
            starty = (word)(cmd[0] | (cmd[1] << 8));
            cmd += 2;
            starty >>= 1;
            if (endy > 64u) {
                endy = 64;
            }
            if (starty > 64u) {
                starty = 64;
            }
            if (starty >= endy) {
                continue;
            }
            if ((int)newstart + (int)endy > (int)page_len) {
                continue;
            }
            for (j = starty; j < endy; ++j) {
                out[j * 64 + x] = shape[newstart + j];
            }
        }
    }
    return true;
}

static int count_opaque(const byte *buf)
{
    int i, n = 0;
    for (i = 0; i < 64 * 64; ++i) {
        if (buf[i] != 0xFF) {
            n++;
        }
    }
    return n;
}

boolean Wolf_SpriteSmokeVerify(void)
{
    byte buf[64 * 64];
    int n;

    if (!Wolf_SpriteExpand(SPR_STAT_BASE + 3, buf)) {
        fprintf(stderr, "Sprite smoke: lamp expand failed\n");
        return false;
    }
    n = count_opaque(buf);
    printf("Sprite smoke: lamp filled=%d\n", n);
    if (n < 40) {
        return false;
    }
    if (!Wolf_SpriteExpand(SPR_GRD_S1, buf)) {
        fprintf(stderr, "Sprite smoke: guard expand failed\n");
        return false;
    }
    n = count_opaque(buf);
    printf("Sprite smoke: guard_s1 filled=%d\n", n);
    if (n < 40) {
        return false;
    }
    if (!Wolf_SpriteExpand(SPR_GRD_DEAD, buf)) {
        fprintf(stderr, "Sprite smoke: corpse expand failed\n");
        return false;
    }
    n = count_opaque(buf);
    printf("Sprite smoke: guard_dead filled=%d\n", n);
    if (n < 20) {
        return false;
    }
    if (!Wolf_SpriteExpand(SPR_STAT_BASE + 11, buf)) {
        fprintf(stderr, "Sprite smoke: plant expand failed\n");
        return false;
    }
    n = count_opaque(buf);
    printf("Sprite smoke: plant filled=%d\n", n);
    return n >= 20;
}
