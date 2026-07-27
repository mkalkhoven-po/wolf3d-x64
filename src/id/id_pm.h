#pragma once

#include "wl_data.h"
#include "wl_types.h"

#define PMPageSize 4096

typedef struct {
    longword offset;
    word length;
} pm_page_info_t;

extern word ChunksInFile;
extern word PMSpriteStart;
extern word PMSoundStart;
extern pm_page_info_t *PMPages;

void PM_Startup(void);
void PM_Shutdown(void);
void PM_Reset(void);
void PM_NextFrame(void);

/* Open VSWAP for dir and read chunk directory (no full cache). */
boolean PM_OpenPageFile(const wolf_data_dir_t *dir);

void PM_PrintSmoke(void);

memptr PM_GetPage(int pagenum);
memptr PM_GetPageAddress(int pagenum);
