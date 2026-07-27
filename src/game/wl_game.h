#pragma once

/* In-game 3D loop; returns 0 on quit back to OS, non-zero on error.
 * Esc returns to caller with 0 after shutdown of local loop resources.
 */
int Wolf_RunGame(void);
