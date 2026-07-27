#include "id_ca.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "id_mm.h"
#include "id_us.h"

#define NEARTAG 0xa7
#define FARTAG 0xa8
#define MAX_MAP_OFFSETS 100

uint16_t *mapsegs[MAPPLANES];
maptype *mapheaderseg[NUMMAPS];
int mapon = -1;
uint16_t rlew_tag;

static FILE *maphandle;
static int32_t headeroffsets[MAX_MAP_OFFSETS];
static wolf_data_dir_t g_map_dir;

/* ---- little-endian helpers ---- */

static uint16_t rd_u16(const byte *p)
{
    return (uint16_t)(p[0] | (p[1] << 8));
}

static int32_t rd_i32(const byte *p)
{
    return (int32_t)(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}

static size_t file_size(FILE *f)
{
    long pos = ftell(f);
    long end;
    if (fseek(f, 0, SEEK_END) != 0) {
        return 0;
    }
    end = ftell(f);
    fseek(f, pos, SEEK_SET);
    return end > 0 ? (size_t)end : 0;
}

/* ---- decompressors (ports of id Carmack + RLEW expand) ---- */

void CAL_CarmackExpand(const uint16_t *source, uint16_t *dest, unsigned length_bytes)
{
    unsigned length = length_bytes / 2u;
    const uint16_t *inptr = source;
    uint16_t *outptr = dest;

    while (length) {
        uint16_t ch = *inptr++;
        unsigned chhigh = ch >> 8;
        if (chhigh == NEARTAG) {
            unsigned count = ch & 0xffu;
            if (!count) {
                ch |= *(const uint8_t *)inptr;
                inptr = (const uint16_t *)((const uint8_t *)inptr + 1);
                *outptr++ = ch;
                length--;
            } else {
                unsigned offset = *(const uint8_t *)inptr;
                inptr = (const uint16_t *)((const uint8_t *)inptr + 1);
                {
                    const uint16_t *copyptr = outptr - offset;
                    length -= count;
                    while (count--) {
                        *outptr++ = *copyptr++;
                    }
                }
            }
        } else if (chhigh == FARTAG) {
            unsigned count = ch & 0xffu;
            if (!count) {
                ch |= *(const uint8_t *)inptr;
                inptr = (const uint16_t *)((const uint8_t *)inptr + 1);
                *outptr++ = ch;
                length--;
            } else {
                unsigned offset = *inptr++;
                {
                    const uint16_t *copyptr = dest + offset;
                    length -= count;
                    while (count--) {
                        *outptr++ = *copyptr++;
                    }
                }
            }
        } else {
            *outptr++ = ch;
            length--;
        }
    }
}

void CA_RLEWexpand(const uint16_t *source, uint16_t *dest, long length_bytes,
                   uint16_t rlewtag)
{
    uint16_t *end = dest + (length_bytes / 2);
    while (dest < end) {
        uint16_t value = *source++;
        if (value != rlewtag) {
            *dest++ = value;
        } else {
            uint16_t count = *source++;
            value = *source++;
            while (count--) {
                *dest++ = value;
            }
        }
    }
}

/* ---- lifecycle ---- */

void CA_Startup(void)
{
    /* Real open happens via CA_OpenMapFiles after data dir is known. */
}

void CA_Shutdown(void)
{
    CA_FreeMaps();
    if (maphandle) {
        fclose(maphandle);
        maphandle = NULL;
    }
}

void CA_FreeMaps(void)
{
    int i;
    for (i = 0; i < MAPPLANES; ++i) {
        if (mapsegs[i]) {
            free(mapsegs[i]);
            mapsegs[i] = NULL;
        }
    }
    for (i = 0; i < NUMMAPS; ++i) {
        if (mapheaderseg[i]) {
            free(mapheaderseg[i]);
            mapheaderseg[i] = NULL;
        }
    }
    mapon = -1;
}

boolean CA_OpenMapFiles(const wolf_data_dir_t *dir)
{
    char path[WOLF_PATH_MAX];
    byte *tinf = NULL;
    size_t length;
    size_t nread;
    int i;

    g_map_dir = *dir;
    CA_FreeMaps();
    if (maphandle) {
        fclose(maphandle);
        maphandle = NULL;
    }

    Wolf_MakeDataPath(dir, "MAPHEAD", path, sizeof(path));
    {
        FILE *hf = fopen(path, "rb");
        if (!hf) {
            fprintf(stderr, "Cannot open %s\n", path);
            return false;
        }
        length = file_size(hf);
        if (length < 2 + 4 || length > 1024 * 64) {
            fclose(hf);
            fprintf(stderr, "Bad MAPHEAD size: %zu\n", length);
            return false;
        }
        tinf = (byte *)malloc(length);
        if (!tinf) {
            fclose(hf);
            return false;
        }
        nread = fread(tinf, 1, length, hf);
        fclose(hf);
        if (nread != length) {
            free(tinf);
            return false;
        }
    }

    rlew_tag = rd_u16(tinf);
    memset(headeroffsets, 0, sizeof(headeroffsets));
    {
        size_t off_count = (length - 2) / 4;
        if (off_count > MAX_MAP_OFFSETS) {
            off_count = MAX_MAP_OFFSETS;
        }
        for (i = 0; i < (int)off_count; ++i) {
            headeroffsets[i] = rd_i32(tinf + 2 + (size_t)i * 4);
        }
    }
    free(tinf);

    Wolf_MakeDataPath(dir, "GAMEMAPS", path, sizeof(path));
    maphandle = fopen(path, "rb");
    if (!maphandle) {
        /* Some shareware trees used MAPTEMP — try as fallback. */
        Wolf_MakeDataPath(dir, "MAPTEMP", path, sizeof(path));
        maphandle = fopen(path, "rb");
    }
    if (!maphandle) {
        fprintf(stderr, "Cannot open GAMEMAPS/MAPTEMP with extension %s\n",
                dir->extension);
        return false;
    }

    for (i = 0; i < NUMMAPS; ++i) {
        int32_t pos = headeroffsets[i];
        maptype hdr;
        if (pos <= 0) {
            continue;
        }
        if (fseek(maphandle, pos, SEEK_SET) != 0) {
            continue;
        }
        if (fread(&hdr, 1, sizeof(hdr), maphandle) != sizeof(hdr)) {
            continue;
        }
        /* fix endianness of multi-byte fields */
        {
            int p;
            byte *raw = (byte *)&hdr;
            maptype fixed;
            for (p = 0; p < 3; ++p) {
                fixed.planestart[p] = rd_i32(raw + p * 4);
                fixed.planelength[p] = rd_u16(raw + 12 + p * 2);
            }
            fixed.width = rd_u16(raw + 18);
            fixed.height = rd_u16(raw + 20);
            memcpy(fixed.name, raw + 22, 16);
            hdr = fixed;
        }
        mapheaderseg[i] = (maptype *)malloc(sizeof(maptype));
        if (!mapheaderseg[i]) {
            Quit("CA_OpenMapFiles: OOM");
        }
        *mapheaderseg[i] = hdr;
    }

    printf("MAPHEAD RLEW tag: 0x%04x\n", rlew_tag);
    printf("Maps with headers: %d\n", CA_CountValidMaps());
    return true;
}

int CA_CountValidMaps(void)
{
    int i, n = 0;
    for (i = 0; i < NUMMAPS; ++i) {
        if (mapheaderseg[i]) {
            n++;
        }
    }
    return n;
}

boolean CA_CacheMap(int mapnum)
{
    int plane;
    const unsigned size = MAPAREA * 2; /* expanded plane bytes */

    if (mapnum < 0 || mapnum >= NUMMAPS || !mapheaderseg[mapnum]) {
        fprintf(stderr, "CA_CacheMap: invalid map %d\n", mapnum);
        return false;
    }
    if (!maphandle) {
        fprintf(stderr, "CA_CacheMap: maps not open\n");
        return false;
    }

    for (plane = 0; plane < MAPPLANES; ++plane) {
        if (mapsegs[plane]) {
            free(mapsegs[plane]);
            mapsegs[plane] = NULL;
        }
        mapsegs[plane] = (uint16_t *)calloc(MAPAREA, sizeof(uint16_t));
        if (!mapsegs[plane]) {
            Quit("CA_CacheMap: OOM plane");
        }
    }

    mapon = mapnum;

    for (plane = 0; plane < MAPPLANES; ++plane) {
        int32_t pos = mapheaderseg[mapnum]->planestart[plane];
        uint16_t compressed = mapheaderseg[mapnum]->planelength[plane];
        byte *source = NULL;
        uint16_t *src16;
        uint16_t expanded;
        uint16_t *buffer2 = NULL;

        if (pos <= 0 || compressed == 0) {
            fprintf(stderr, "Map %d plane %d missing\n", mapnum, plane);
            return false;
        }
        if (fseek(maphandle, pos, SEEK_SET) != 0) {
            return false;
        }
        source = (byte *)malloc(compressed);
        if (!source) {
            return false;
        }
        if (fread(source, 1, compressed, maphandle) != compressed) {
            free(source);
            return false;
        }

        /* Carmackized: first word = expanded size of Carmack output */
        src16 = (uint16_t *)source;
        expanded = src16[0];
        if (expanded < 2 || expanded > 64 * 1024) {
            fprintf(stderr, "Map %d plane %d bad expanded=%u\n", mapnum, plane,
                    expanded);
            free(source);
            return false;
        }
        buffer2 = (uint16_t *)malloc(expanded);
        if (!buffer2) {
            free(source);
            return false;
        }
        CAL_CarmackExpand(src16 + 1, buffer2, expanded);
        /* RLEW: skip expanded-length word in RLEW stream */
        CA_RLEWexpand(buffer2 + 1, mapsegs[plane], size, rlew_tag);

        free(buffer2);
        free(source);
    }

    return true;
}

void CA_PrintMapSmoke(int mapnum)
{
    unsigned i;
    unsigned solid = 0;
    unsigned empty = 0;
    uint16_t minv = 0xffff, maxv = 0;

    if (mapnum != mapon || !mapsegs[0]) {
        fprintf(stderr, "CA_PrintMapSmoke: map %d not cached\n", mapnum);
        return;
    }

    for (i = 0; i < MAPAREA; ++i) {
        uint16_t v = mapsegs[0][i];
        if (v < minv) {
            minv = v;
        }
        if (v > maxv) {
            maxv = v;
        }
        if (v == 0) {
            empty++;
        } else {
            solid++;
        }
    }

    printf("Map %d \"%.16s\" %ux%u plane0: empty=%u solid=%u tile range=%u..%u\n",
               mapnum,
               mapheaderseg[mapnum] ? mapheaderseg[mapnum]->name : "?",
               mapheaderseg[mapnum] ? mapheaderseg[mapnum]->width : 0,
               mapheaderseg[mapnum] ? mapheaderseg[mapnum]->height : 0,
               empty, solid, minv, maxv);

    /* Print a tiny 16x8 ASCII preview of northwest corner (wall if nonzero). */
    {
        int y, x;
        printf("NW preview (1=wall-ish tile):\n");
        for (y = 0; y < 8; ++y) {
            for (x = 0; x < 16; ++x) {
                uint16_t v = mapsegs[0][y * MAPSIZE + x];
                putchar(v ? '#' : '.');
            }
            putchar('\n');
        }
    }
}

void CA_CacheScreen(int chunk)
{
    (void)chunk;
}

void CA_CacheGrChunk(int chunk)
{
    (void)chunk;
}

void CA_UnCacheGrChunk(int chunk)
{
    (void)chunk;
}

boolean CA_LoadFile(const char *filename, memptr *ptr)
{
    FILE *f;
    size_t length;
    void *buf;

    if (!ptr) {
        return false;
    }
    *ptr = NULL;
    f = fopen(filename, "rb");
    if (!f) {
        return false;
    }
    length = file_size(f);
    buf = malloc(length ? length : 1);
    if (!buf) {
        fclose(f);
        return false;
    }
    if (length && fread(buf, 1, length, f) != length) {
        free(buf);
        fclose(f);
        return false;
    }
    fclose(f);
    *ptr = buf;
    return true;
}
