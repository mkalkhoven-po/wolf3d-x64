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

/* registered 1.4 (GFXV_WL6.H) */
const gfx_layout_t gfx_layout_wl6 = {"WL6", 149, 132, 135, 136, 0, 6};

/* Apogee 1.4 shareware: 12 more help pictures, then the WL6 layout; 8 externs */
const gfx_layout_t gfx_layout_wl1 = {"WL1", 156, 144, 147, 148, 12, 17};

const gfx_layout_t *g_gfx = &gfx_layout_wl6;

static FILE *maphandle;
static int32_t headeroffsets[MAX_MAP_OFFSETS];
static wolf_data_dir_t g_map_dir;

/* Graphics (VGAGRAPH) */
pictabletype *pictable;
byte *grsegs[NUMCHUNKS_MAX];
static size_t grsizes[NUMCHUNKS_MAX]; /* expanded length of each cached chunk */
static FILE *grhandle;
static int32_t grstarts[NUMCHUNKS_MAX + 1];
typedef struct {
    uint16_t bit0, bit1;
} huffnode;
static huffnode grhuffman[256];

/* ---- little-endian helpers ---- */

static uint16_t rd_u16(const byte *p)
{
    return (uint16_t)(p[0] | (p[1] << 8));
}

static int32_t rd_i32(const byte *p)
{
    return (int32_t)((uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
                     ((uint32_t)p[3] << 24));
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

/* Near/far operands are byte-packed, so the stream is read bytewise (no unaligned loads). */
boolean CAL_CarmackExpand(const byte *source, size_t src_bytes, uint16_t *dest,
                          unsigned length_bytes)
{
    const unsigned total = length_bytes / 2u;
    unsigned out = 0;
    size_t in = 0;

    while (out < total) {
        uint16_t ch;
        unsigned chhigh;
        unsigned count;

        if (in + 2 > src_bytes) {
            return false;
        }
        ch = rd_u16(source + in);
        in += 2;
        chhigh = ch >> 8;
        count = ch & 0xffu;

        if ((chhigh != NEARTAG && chhigh != FARTAG) || count == 0) {
            if (chhigh == NEARTAG || chhigh == FARTAG) {
                /* Escaped literal: low byte of the word comes from the stream. */
                if (in + 1 > src_bytes) {
                    return false;
                }
                ch = (uint16_t)(ch | source[in++]);
            }
            dest[out++] = ch;
        } else {
            unsigned offset;
            if (chhigh == NEARTAG) {
                if (in + 1 > src_bytes) {
                    return false;
                }
                offset = source[in++];
                if (offset == 0 || offset > out) {
                    return false;
                }
                offset = out - offset;
            } else {
                if (in + 2 > src_bytes) {
                    return false;
                }
                offset = rd_u16(source + in);
                in += 2;
                if (offset >= out) {
                    return false;
                }
            }
            if (count > total - out) {
                return false;
            }
            while (count--) {
                dest[out++] = dest[offset++];
            }
        }
    }
    return true;
}

boolean CA_RLEWexpand(const uint16_t *source, size_t src_words, uint16_t *dest,
                      long length_bytes, uint16_t rlewtag)
{
    const size_t total = (size_t)(length_bytes / 2);
    size_t out = 0;
    size_t in = 0;

    while (out < total) {
        uint16_t value;
        if (in >= src_words) {
            return false;
        }
        value = source[in++];
        if (value != rlewtag) {
            dest[out++] = value;
        } else {
            uint16_t count;
            if (in + 2 > src_words) {
                return false;
            }
            count = source[in++];
            value = source[in++];
            if (count > total - out) {
                return false;
            }
            while (count--) {
                dest[out++] = value;
            }
        }
    }
    return true;
}

/* ---- lifecycle ---- */

void CA_Startup(void)
{
    /* Real open happens via CA_OpenMapFiles after data dir is known. */
}

void CA_Shutdown(void)
{
    int i;
    CA_FreeMaps();
    if (maphandle) {
        fclose(maphandle);
        maphandle = NULL;
    }
    if (grhandle) {
        fclose(grhandle);
        grhandle = NULL;
    }
    for (i = 0; i < NUMCHUNKS; ++i) {
        CA_UnCacheGrChunk(i);
    }
    if (pictable) {
        free(pictable);
        pictable = NULL;
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

        if (pos <= 0 || compressed < 4) {
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
        if (expanded < 2) {
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
        /* RLEW stream starts after its own expanded-length word. */
        if (!CAL_CarmackExpand(source + 2, (size_t)compressed - 2, buffer2, expanded) ||
            !CA_RLEWexpand(buffer2 + 1, (size_t)(expanded / 2u) - 1, mapsegs[plane],
                           size, rlew_tag)) {
            fprintf(stderr, "Map %d plane %d corrupt\n", mapnum, plane);
            free(buffer2);
            free(source);
            return false;
        }

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
    /* Full-screen chunky when expanded size is 320*200. */
    CA_CacheGrChunk(chunk);
    if (grsegs[chunk]) {
        /* CA_CacheScreen in original expanded with screenhack — we use DrawPic path. */
    }
}

/*
 * Pure-C Huffman expander for VGA graph chunks.
 * Nodes 0..255 are leaves (=byte), 256..510 are internal nodes index+256.
 * Head is node 254.
 */
static void CAL_HuffExpand(const byte *source, byte *dest, long length,
                           const huffnode *hufftable)
{
    const huffnode *headptr = &hufftable[254];
    const huffnode *node = headptr;
    const byte *src = source;
    byte *out = dest;
    byte *end = dest + length;
    unsigned mask = 1;
    byte b = *src++;

    while (out < end) {
        uint16_t next = (b & mask) ? node->bit1 : node->bit0;
        mask <<= 1;
        if (mask == 256) {
            b = *src++;
            mask = 1;
        }
        if (next < 256) {
            *out++ = (byte)next;
            node = headptr;
        } else {
            node = &hufftable[next - 256];
        }
    }
}

static int32_t GRFILEPOS(int chunk)
{
    if (chunk < 0 || chunk > NUMCHUNKS) {
        return -1;
    }
    return grstarts[chunk];
}

static void VL_UnmungeToChunky(const byte *planar, byte *chunky, int width, int height)
{
    int plane, y, x;
    int pwidth = width / 4;
    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x) {
            plane = x & 3;
            chunky[y * width + x] =
                planar[plane * pwidth * height + y * pwidth + (x >> 2)];
        }
    }
}

boolean CA_OpenGraphFiles(const wolf_data_dir_t *dir)
{
    char path[WOLF_PATH_MAX];
    FILE *f;
    byte headbuf[(NUMCHUNKS_MAX + 1) * 3];
    int i;
    size_t n;
    byte *compressed = NULL;
    long compr_len;
    long expanded;

    if (grhandle) {
        fclose(grhandle);
        grhandle = NULL;
    }
    for (i = 0; i < NUMCHUNKS; ++i) {
        CA_UnCacheGrChunk(i);
    }
    if (pictable) {
        free(pictable);
        pictable = NULL;
    }
    g_gfx = (strcmp(dir->extension, ".WL1") == 0) ? &gfx_layout_wl1 : &gfx_layout_wl6;

    /* Huffman dictionary — 255 nodes * 4 bytes, file is 1024 */
    Wolf_MakeDataPath(dir, "VGADICT", path, sizeof(path));
    f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Cannot open %s\n", path);
        return false;
    }
    memset(grhuffman, 0, sizeof(grhuffman));
    if (fread(grhuffman, 1, 255 * sizeof(huffnode), f) != 255 * sizeof(huffnode)) {
        fclose(f);
        return false;
    }
    fclose(f);

    /* 3-byte offsets */
    Wolf_MakeDataPath(dir, "VGAHEAD", path, sizeof(path));
    f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Cannot open %s\n", path);
        return false;
    }
    n = fread(headbuf, 1, sizeof(headbuf), f);
    fclose(f);
    if (n != (size_t)(NUMCHUNKS + 1) * 3) {
        fprintf(stderr, "VGAHEAD size %zu does not match the %s layout (%d bytes): "
                        "unsupported pack version\n",
                n, g_gfx->name, (NUMCHUNKS + 1) * 3);
        return false;
    }
    for (i = 0; i <= NUMCHUNKS; ++i) {
        const byte *p = headbuf + i * 3;
        int32_t v = (int32_t)(p[0] | (p[1] << 8) | (p[2] << 16));
        if (v == 0xffffff) {
            v = -1;
        }
        grstarts[i] = v;
    }

    Wolf_MakeDataPath(dir, "VGAGRAPH", path, sizeof(path));
    grhandle = fopen(path, "rb");
    if (!grhandle) {
        fprintf(stderr, "Cannot open %s\n", path);
        return false;
    }

    /* STRUCTPIC (chunk 0) → pictable */
    {
        int32_t pos = GRFILEPOS(STRUCTPIC);
        int next = STRUCTPIC + 1;
        while (next <= NUMCHUNKS && GRFILEPOS(next) == -1) {
            next++;
        }
        if (pos < 0 || next > NUMCHUNKS) {
            fprintf(stderr, "Bad STRUCTPIC positions\n");
            return false;
        }
        compr_len = GRFILEPOS(next) - pos;
        if (compr_len <= 0 || compr_len > 1024 * 1024) {
            return false;
        }
        if (fseek(grhandle, pos, SEEK_SET) != 0) {
            return false;
        }
        compressed = (byte *)malloc((size_t)compr_len);
        if (!compressed ||
            fread(compressed, 1, (size_t)compr_len, grhandle) != (size_t)compr_len) {
            free(compressed);
            return false;
        }
        expanded = (long)rd_i32(compressed);
        if (expanded != (long)(NUMPICS * (int)sizeof(pictabletype))) {
            fprintf(stderr, "pictable expanded size %ld (expected %d for %s)\n",
                    expanded, (int)(NUMPICS * sizeof(pictabletype)), g_gfx->name);
            free(compressed);
            return false;
        }
        if (expanded <= 0 || expanded > 64 * 1024) {
            free(compressed);
            return false;
        }
        pictable = (pictabletype *)malloc((size_t)expanded);
        if (!pictable) {
            free(compressed);
            return false;
        }
        CAL_HuffExpand(compressed + 4, (byte *)pictable, expanded, grhuffman);
        free(compressed);
        /* endian fix pictable entries */
        for (i = 0; i < NUMPICS; ++i) {
            byte *raw = (byte *)&pictable[i];
            int16_t w = (int16_t)(raw[0] | (raw[1] << 8));
            int16_t h = (int16_t)(raw[2] | (raw[3] << 8));
            pictable[i].width = w;
            pictable[i].height = h;
        }
    }

    printf("VGAGRAPH open: %s layout, pics=%d (TITLEPIC=%d size %dx%d)\n",
           g_gfx->name, NUMPICS, TITLEPIC,
           pictable[TITLEPIC - STARTPICS].width,
           pictable[TITLEPIC - STARTPICS].height);
    return true;
}

void CA_CacheGrChunk(int chunk)
{
    int32_t pos;
    int next;
    long compressed;
    byte *source = NULL;
    long expanded;
    byte *destination = NULL;
    byte *source_data;

    if (chunk < 0 || chunk >= NUMCHUNKS) {
        return;
    }
    if (grsegs[chunk]) {
        return;
    }
    if (!grhandle || !pictable) {
        return;
    }

    pos = GRFILEPOS(chunk);
    if (pos < 0) {
        return; /* sparse */
    }
    next = chunk + 1;
    while (next <= NUMCHUNKS && GRFILEPOS(next) == -1) {
        next++;
    }
    if (next > NUMCHUNKS) {
        return;
    }
    compressed = GRFILEPOS(next) - pos;
    if (compressed <= 0) {
        return;
    }
    if (fseek(grhandle, pos, SEEK_SET) != 0) {
        return;
    }
    source = (byte *)malloc((size_t)compressed);
    if (!source || fread(source, 1, (size_t)compressed, grhandle) != (size_t)compressed) {
        free(source);
        return;
    }

    source_data = source;
    if (chunk >= STARTTILE8 && chunk < STARTEXTERNS) {
        /* not needed for E4 menus */
        free(source);
        return;
    }

    expanded = (long)rd_i32(source);
    source_data = source + 4;
    if (expanded <= 0 || expanded > 320 * 200 * 4) {
        free(source);
        return;
    }

    destination = (byte *)malloc((size_t)expanded);
    if (!destination) {
        free(source);
        return;
    }
    CAL_HuffExpand(source_data, destination, expanded, grhuffman);
    free(source);

    /* Convert planar pics to chunky for easy blitting. */
    if (chunk >= STARTPICS && chunk < STARTPICS + NUMPICS) {
        int pic = chunk - STARTPICS;
        int w = pictable[pic].width;
        int h = pictable[pic].height;
        if (w > 0 && h > 0 && (w * h) == (int)expanded && (w & 3) == 0) {
            byte *chunky = (byte *)malloc((size_t)(w * h));
            if (chunky) {
                VL_UnmungeToChunky(destination, chunky, w, h);
                free(destination);
                destination = chunky;
            }
        }
    }

    grsegs[chunk] = destination;
    grsizes[chunk] = (size_t)expanded;
}

size_t CA_GrChunkSize(int chunk)
{
    return chunk >= 0 && chunk < NUMCHUNKS && grsegs[chunk] ? grsizes[chunk] : 0;
}

void CA_UnCacheGrChunk(int chunk)
{
    if (chunk < 0 || chunk >= NUMCHUNKS) {
        return;
    }
    if (grsegs[chunk]) {
        free(grsegs[chunk]);
        grsegs[chunk] = NULL;
    }
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
