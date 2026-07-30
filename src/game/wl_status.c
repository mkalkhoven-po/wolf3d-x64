#include "wl_status.h"

#include <stdio.h>
#include <string.h>

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL.h>

#include "id/gfxv_wl6.h"
#include "id/id_ca.h"
#include "id/id_vh.h"
#include "id/id_vl.h"
#include "wl_play.h"

/*
 * LatchDrawPic uses x in character columns (×8 pixels). StatusDrawPic places
 * all status latch art relative to status origin y = 200 - STATUSLINES.
 */
static void status_draw_pic(unsigned col, unsigned y, unsigned picnum)
{
    int px = (int)col * 8;
    int py = (SCREENHEIGHT - STATUSLINES) + (int)y;
    VWB_DrawPic(px, py, (int)picnum);
}

static void latch_number(int col, int y, int width, long number)
{
    char str[24];
    unsigned length;
    unsigned c;

    snprintf(str, sizeof(str), "%ld", number);
    length = (unsigned)strlen(str);

    while ((int)length < width) {
        status_draw_pic((unsigned)col, (unsigned)y, N_BLANKPIC);
        col++;
        width--;
    }

    c = length <= (unsigned)width ? 0 : length - (unsigned)width;
    while (c < length) {
        status_draw_pic((unsigned)col, (unsigned)y,
                        (unsigned)(str[c] - '0' + N_0PIC));
        col++;
        c++;
    }
}

static void draw_face(void)
{
    player_t *p = &g_level.player;
    int frame;

    if (p->dead || p->health <= 0) {
        status_draw_pic(17, 4, FACE8APIC);
        return;
    }
    /* FACE1A + 3*((100-health)/16) — use faceframe 0 (A). */
    frame = 3 * ((100 - p->health) / 16);
    if (frame < 0) {
        frame = 0;
    }
    if (frame > 7 * 3) {
        frame = 7 * 3;
    }
    status_draw_pic(17, 4, (unsigned)(FACE1APIC + frame));
}

static void draw_weapon_icon(void)
{
    player_t *p = &g_level.player;
    int w = p->weapon;
    if (w < 0) {
        w = 0;
    }
    if (w > 3) {
        w = 3;
    }
    status_draw_pic(32, 8, (unsigned)(KNIFEPIC + w));
}

static void draw_keys(void)
{
    player_t *p = &g_level.player;
    if (p->keys & 1) {
        status_draw_pic(30, 4, GOLDKEYPIC);
    } else {
        status_draw_pic(30, 4, NOKEYPIC);
    }
    if (p->keys & 2) {
        status_draw_pic(30, 20, SILVERKEYPIC);
    } else {
        status_draw_pic(30, 20, NOKEYPIC);
    }
}

void Wolf_DrawStatusBar(void)
{
    player_t *p = &g_level.player;
    int floor_num;

    if (!pictable) {
        return;
    }

    /* Full strip first */
    VWB_DrawPic(0, SCREENHEIGHT - STATUSLINES, STATUSBARPIC);

    floor_num = g_level.mapnum + 1;
    if (floor_num < 1) {
        floor_num = 1;
    }
    latch_number(2, 16, 2, floor_num);
    latch_number(6, 16, 6, p->score);
    latch_number(14, 16, 1, p->lives >= 0 ? p->lives : 0);
    latch_number(21, 16, 3, p->health > 0 ? p->health : 0);
    latch_number(27, 16, 2, p->ammo >= 0 ? p->ammo : 0);

    draw_face();
    draw_weapon_icon();
    draw_keys();
}

boolean Wolf_ShowGetPsyched(wolf_present_t *pr, boolean short_mode)
{
    int i;
    int steps;
    int win_x, win_y, win_w, win_h;
    int bar_y;
    int max_w;

    if (!pictable || !pr) {
        return false;
    }

    CA_CacheGrChunk(GETPSYCHEDPIC);
    CA_CacheGrChunk(STATUSBARPIC);
    if (!grsegs[GETPSYCHEDPIC]) {
        fprintf(stderr, "Get Psyched: GETPSYCHEDPIC missing\n");
        return false;
    }

    /* Teal playfield (original 127), leave status strip solid for STATUSBAR. */
    VWB_Bar(0, 0, SCREENWIDTH, SCREENHEIGHT - STATUSLINES, 127);
    VWB_DrawPic(0, SCREENHEIGHT - STATUSLINES, STATUSBARPIC);

    /*
     * LatchDrawPic(20-14, 80-3*8, GETPSYCHEDPIC) → pixel (48, 56).
     * Progress window sits on the pic (original Window*).
     */
    VWB_DrawPic(48, 56, GETPSYCHEDPIC);

    win_x = 160 - 14 * 8; /* 48 */
    win_y = 80 - 3 * 8;   /* 56 */
    win_w = 28 * 8;       /* 224 */
    win_h = 48;
    bar_y = win_y + win_h - 3;
    max_w = win_w - 10;

    steps = short_mode ? 8 : 24;
    for (i = 0; i <= steps; ++i) {
        int w = (int)(((long)max_w * i) / steps);
        VWB_Bar(win_x + 5, bar_y, max_w, 2, 0); /* black track */
        if (w > 0) {
            /* Original PreloadUpdate reds (letter-matched). */
            VWB_Bar(win_x + 5, bar_y, w, 2, 0x37);
            if (w > 1) {
                VWB_Bar(win_x + 5, bar_y, w - 1, 1, 0x32);
            }
        }
        Wolf_PresentFrame(pr);
        SDL_Delay(short_mode ? 16 : 40);
        {
            SDL_Event ev;
            while (SDL_PollEvent(&ev)) {
                if (ev.type == SDL_QUIT) {
                    return true;
                }
            }
        }
    }

    if (!short_mode) {
        SDL_Delay(200);
    }
    printf("Get Psyched: interstitial shown (short=%d)\n", (int)short_mode);
    return true;
}

boolean Wolf_StatusSmokeVerify(void)
{
    int sw, sh, pw, ph;

    if (!pictable) {
        fprintf(stderr, "Status smoke: no pictable\n");
        return false;
    }
    CA_CacheGrChunk(STATUSBARPIC);
    CA_CacheGrChunk(GETPSYCHEDPIC);
    CA_CacheGrChunk(N_0PIC);
    CA_CacheGrChunk(FACE1APIC);
    CA_CacheGrChunk(KNIFEPIC);

    sw = pictable[STATUSBARPIC - STARTPICS].width;
    sh = pictable[STATUSBARPIC - STARTPICS].height;
    pw = pictable[GETPSYCHEDPIC - STARTPICS].width;
    ph = pictable[GETPSYCHEDPIC - STARTPICS].height;

    printf("Status smoke: STATUSBARPIC=%dx%d GETPSYCHEDPIC=%dx%d "
           "chunks_ok=%d\n",
           sw, sh, pw, ph,
           grsegs[STATUSBARPIC] && grsegs[GETPSYCHEDPIC] && grsegs[N_0PIC] &&
               grsegs[FACE1APIC] && grsegs[KNIFEPIC]);

    if (sw < 300 || sh < 30 || pw < 80 || ph < 20) {
        fprintf(stderr, "Status smoke: unexpected pic sizes\n");
        return false;
    }
    if (!grsegs[STATUSBARPIC] || !grsegs[GETPSYCHEDPIC]) {
        return false;
    }
    return true;
}
