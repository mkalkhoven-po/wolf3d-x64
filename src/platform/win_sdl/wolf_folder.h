#pragma once

#include <stddef.h>

/*
 * The first-run question when no game files are found: a message box
 * ("Choose folder..." / "Quit"), then the system's folder dialog (Windows;
 * zenity or kdialog on Linux), until check accepts the folder. 1 with the
 * folder in out; 0 when the player quits; -1 when this system has no
 * message box or folder dialog (the caller then explains --data). Plain
 * int, not boolean: the Windows COM headers define their own boolean.
 */
int Wolf_ChooseDataFolder(int (*check)(const char *folder), char *out, size_t out_sz);
