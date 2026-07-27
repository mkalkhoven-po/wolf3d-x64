#pragma once

#include "wl_data.h"
#include "wl_types.h"

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

void CA_Startup(void);
void CA_Shutdown(void);

/* Open MAPHEAD/GAMEMAPS for dir; load headers. */
boolean CA_OpenMapFiles(const wolf_data_dir_t *dir);

/* Decompress map planes into mapsegs (allocates). */
boolean CA_CacheMap(int mapnum);

void CA_FreeMaps(void);

/* Return count of maps with valid offsets. */
int CA_CountValidMaps(void);

/* Dump simple wall stats for map plane 0 (walls). */
void CA_PrintMapSmoke(int mapnum);
