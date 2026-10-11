#pragma once

#include "wl_data.h"
#include "wl_types.h"
#include "gfxv.h"

#define NUMMAPS 60
#define MAPPLANES 2
#define MAPSIZE 64
#define MAPAREA (MAPSIZE * MAPSIZE)

#pragma pack(push, 1)
typedef struct {
    int32_t planestart[3];
    uint16_t planelength[3];
    uint16_t width, height;
    char name[16];
} maptype;
#pragma pack(pop)

extern uint16_t *mapsegs[MAPPLANES];
extern maptype *mapheaderseg[NUMMAPS];
extern int mapon;
extern uint16_t rlew_tag;

extern pictabletype *pictable;
extern byte *grsegs[NUMCHUNKS_MAX];

void CA_Startup(void);
void CA_Shutdown(void);

boolean CA_OpenMapFiles(const wolf_data_dir_t *dir);
boolean CA_OpenGraphFiles(const wolf_data_dir_t *dir);
boolean CA_CacheMap(int mapnum);

/* Both return false on corrupt or truncated input instead of overrunning. */
boolean CAL_CarmackExpand(const byte *source, size_t src_bytes, uint16_t *dest,
                          unsigned length_bytes);
boolean CA_RLEWexpand(const uint16_t *source, size_t src_words, uint16_t *dest,
                      long length_bytes, uint16_t rlewtag);
void CA_FreeMaps(void);
int CA_CountValidMaps(void);
void CA_PrintMapSmoke(int mapnum);

void CA_CacheGrChunk(int chunk);
void CA_UnCacheGrChunk(int chunk);
size_t CA_GrChunkSize(int chunk); /* the cached chunk's length in bytes, 0 if not cached */
void CA_CacheScreen(int chunk);
boolean CA_LoadFile(const char *filename, memptr *ptr);

/* Endian-safe two-byte file loads already elsewhere. */
