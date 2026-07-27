#pragma once

#include "wl_types.h"

#define WOLF_PATH_MAX 512
#define WOLF_EXT_MAX 8

typedef struct {
    char path[WOLF_PATH_MAX];
    char extension[WOLF_EXT_MAX]; /* e.g. ".WL6" including dot */
} wolf_data_dir_t;

/* Resolve data directory: --data > WOLF3D_DATA > optional defaults.
 * On failure prints diagnostics and returns false.
 */
boolean Wolf_ResolveDataDir(int argc, char **argv, wolf_data_dir_t *out);

/* Verify required pack files exist for out->extension. Lists missing; false if any. */
boolean Wolf_CheckRequiredFiles(const wolf_data_dir_t *dir);

/* Build path: dir + basename + extension into dst (dst_sz). */
void Wolf_MakeDataPath(const wolf_data_dir_t *dir, const char *basename,
                       char *dst, size_t dst_sz);

/* True if argv contains --smoke-only (load test, skip window). */
boolean Wolf_ArgSmokeOnly(int argc, char **argv);

/* Print short usage for data flags. */
void Wolf_PrintDataUsage(void);
