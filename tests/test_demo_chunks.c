/*
 * Needs a user-supplied pack (--data DIR). Checks that T_DEMO0..T_DEMO3 of the
 * selected graphics layout hold demo lumps as PlayDemo (WL_GAME.C) reads them.
 */
#include <stdio.h>
#include <string.h>

#include "id_ca.h"
#include "wl_data.h"

int main(int argc, char **argv)
{
    wolf_data_dir_t dir;
    int failures = 0;
    int i;

    memset(&dir, 0, sizeof(dir));
    if (!Wolf_ResolveDataDir(argc, argv, &dir)) {
        return 1;
    }
    CA_Startup();
    if (!CA_OpenGraphFiles(&dir)) {
        fprintf(stderr, "FAIL: could not open graphics for %s\n", dir.path);
        return 1;
    }

    for (i = 0; i < NUMDEMOS; i++) {
        const int chunk = T_DEMO0 + i;
        const byte *demo;
        unsigned map, length;

        CA_CacheGrChunk(chunk);
        demo = grsegs[chunk];
        if (!demo) {
            fprintf(stderr, "FAIL: %s demo %d (chunk %d) did not load\n", g_gfx->name, i,
                    chunk);
            failures++;
            continue;
        }
        map = demo[0];
        length = (unsigned)demo[1] | ((unsigned)demo[2] << 8);
        printf("%s demo %d: chunk %d, map %u, %u bytes, %u tics\n", g_gfx->name, i, chunk,
               map, length, length >= 4 ? (length - 4) / 3 : 0);
        if (map >= NUMMAPS || length < 4 || (length - 4) % 3 != 0) {
            fprintf(stderr, "FAIL: %s demo %d has a bad header\n", g_gfx->name, i);
            failures++;
        }
    }

    CA_Shutdown();
    if (failures) {
        return 1;
    }
    printf("test_demo_chunks: all checks passed\n");
    return 0;
}
