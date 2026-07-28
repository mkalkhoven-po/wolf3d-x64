#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Directory containing the running executable (no trailing slash). 0 on fail. */
int Wolf_GetExecutableDir(char *out, size_t out_sz);

/* Blocking error box on Windows; stderr on other platforms. */
void Wolf_ShowDataError(const char *message);

#ifdef __cplusplus
}
#endif
