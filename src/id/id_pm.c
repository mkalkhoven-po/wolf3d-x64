#include "id_pm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "id_us.h"

word ChunksInFile;
word PMSpriteStart;
word PMSoundStart;
pm_page_info_t *PMPages;

static FILE *page_file;
static byte **page_cache;

static uint16_t rd_u16_le(FILE *f, boolean *ok)
{
    byte b[2];
    if (fread(b, 1, 2, f) != 2) {
        *ok = false;
        return 0;
    }
    return (uint16_t)(b[0] | (b[1] << 8));
}

static uint32_t rd_u32_le(FILE *f, boolean *ok)
{
    byte b[4];
    if (fread(b, 1, 4, f) != 4) {
        *ok = false;
        return 0;
    }
    return (uint32_t)(b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24));
}

void PM_Startup(void)
{
}

void PM_Shutdown(void)
{
    word i;
    if (page_cache) {
        for (i = 0; i < ChunksInFile; ++i) {
            free(page_cache[i]);
        }
        free(page_cache);
        page_cache = NULL;
    }
    if (page_file) {
        fclose(page_file);
        page_file = NULL;
    }
    if (PMPages) {
        free(PMPages);
        PMPages = NULL;
    }
    ChunksInFile = 0;
    PMSpriteStart = 0;
    PMSoundStart = 0;
}

void PM_Reset(void)
{
}

void PM_NextFrame(void)
{
}

boolean PM_OpenPageFile(const wolf_data_dir_t *dir)
{
    char path[WOLF_PATH_MAX];
    boolean ok = true;
    word i;
    longword *offsets = NULL;
    word *lengths = NULL;

    PM_Shutdown();

    Wolf_MakeDataPath(dir, "VSWAP", path, sizeof(path));
    page_file = fopen(path, "rb");
    if (!page_file) {
        fprintf(stderr, "Cannot open %s\n", path);
        return false;
    }

    ChunksInFile = rd_u16_le(page_file, &ok);
    PMSpriteStart = rd_u16_le(page_file, &ok);
    PMSoundStart = rd_u16_le(page_file, &ok);
    if (!ok || ChunksInFile == 0 || ChunksInFile > 10000) {
        fprintf(stderr, "Bad VSWAP header (chunks=%u)\n", ChunksInFile);
        PM_Shutdown();
        return false;
    }

    PMPages = (pm_page_info_t *)calloc(ChunksInFile, sizeof(pm_page_info_t));
    page_cache = (byte **)calloc(ChunksInFile, sizeof(byte *));
    offsets = (longword *)calloc(ChunksInFile, sizeof(longword));
    lengths = (word *)calloc(ChunksInFile, sizeof(word));
    if (!PMPages || !page_cache || !offsets || !lengths) {
        free(offsets);
        free(lengths);
        PM_Shutdown();
        return false;
    }

    for (i = 0; i < ChunksInFile; ++i) {
        offsets[i] = rd_u32_le(page_file, &ok);
    }
    for (i = 0; i < ChunksInFile; ++i) {
        lengths[i] = rd_u16_le(page_file, &ok);
    }
    if (!ok) {
        free(offsets);
        free(lengths);
        PM_Shutdown();
        fprintf(stderr, "VSWAP directory read failed\n");
        return false;
    }

    for (i = 0; i < ChunksInFile; ++i) {
        PMPages[i].offset = offsets[i];
        PMPages[i].length = lengths[i];
    }
    free(offsets);
    free(lengths);

    printf("VSWAP: %s\n", path);
    printf("VSWAP chunks=%u spriteStart=%u soundStart=%u\n", ChunksInFile,
           PMSpriteStart, PMSoundStart);
    return true;
}

void PM_PrintSmoke(void)
{
    word i;
    word nonempty = 0;
    longword total = 0;
    longword max_len = 0;

    if (!PMPages || !ChunksInFile) {
        fprintf(stderr, "PM_PrintSmoke: page file not open\n");
        return;
    }
    for (i = 0; i < ChunksInFile; ++i) {
        if (PMPages[i].length) {
            nonempty++;
            total += PMPages[i].length;
            if (PMPages[i].length > max_len) {
                max_len = PMPages[i].length;
            }
        }
    }
    printf("VSWAP smoke: nonempty_pages=%u total_bytes~=%lu max_page_len=%lu\n",
           nonempty, (unsigned long)total, (unsigned long)max_len);
    printf("VSWAP wall pages [0..%u), sprites [%u..%u), sounds [%u..%u)\n",
           PMSpriteStart, PMSpriteStart, PMSoundStart, PMSoundStart, ChunksInFile);
}

memptr PM_GetPage(int pagenum)
{
    word len;
    byte *buf;

    if (pagenum < 0 || (word)pagenum >= ChunksInFile || !page_file || !PMPages) {
        return NULL;
    }
    if (page_cache && page_cache[pagenum]) {
        return page_cache[pagenum];
    }

    len = PMPages[pagenum].length;
    if (!len || !PMPages[pagenum].offset) {
        return NULL;
    }
    if (fseek(page_file, (long)PMPages[pagenum].offset, SEEK_SET) != 0) {
        return NULL;
    }
    buf = (byte *)malloc(len);
    if (!buf) {
        return NULL;
    }
    if (fread(buf, 1, len, page_file) != len) {
        free(buf);
        return NULL;
    }
    if (page_cache) {
        page_cache[pagenum] = buf;
    }
    return buf;
}

memptr PM_GetPageAddress(int pagenum)
{
    return PM_GetPage(pagenum);
}
