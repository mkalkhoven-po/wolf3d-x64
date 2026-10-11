#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Directory containing the running executable (no trailing slash). 0 on fail. */
int Wolf_GetExecutableDir(char *out, size_t out_sz);

#ifdef __cplusplus
}
#endif
