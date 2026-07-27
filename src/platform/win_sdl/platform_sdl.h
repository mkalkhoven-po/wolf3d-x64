#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Runs the platform event/present loop until the user closes the window.
 * Returns 0 on clean quit, non-zero on fatal init failure.
 */
int wolf_platform_run(void);

#ifdef __cplusplus
}
#endif
