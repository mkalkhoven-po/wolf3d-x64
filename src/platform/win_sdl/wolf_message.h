#pragma once

/*
 * The missing-data message: an error box (SDL's, on Windows and Linux) with
 * the ways to point the game at its files, and the same text on stderr.
 */
void Wolf_ShowDataError(const char *message);
