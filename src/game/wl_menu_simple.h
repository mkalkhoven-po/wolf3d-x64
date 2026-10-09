#pragma once

/*
 * Interactive title + main menu (pre-game), until P5 ports WL_MENU.C.
 * The menu does not know the engine: it calls these back, with its window
 * closed. Each returns 0 to come back to the menu, a negative value to quit
 * normally (the window was closed in the game), anything else to quit with
 * that error code.
 */
typedef struct {
    int (*new_game)(void);
    int (*load_game)(void); /* the quick save slot (F8) */
} wolf_menu_actions_t;

/* Returns 0 on quit. */
int Wolf_RunIntroMenu(const wolf_menu_actions_t *actions);
