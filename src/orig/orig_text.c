/*
 * WL_TEXT.C: the text articles. A layout is a text chunk of VGAGRAPH with
 * formatting commands; ShowArticle word-wraps it page by page around the
 * pictures it names. HelpScreens shows T_HELPART (Read This!, F1);
 * EndText shows T_ENDART1 + episode after a victory.
 *
 * TEXT FORMATTING COMMANDS
 * ^C<hex digit>          Change text color
 * ^E[enter]              End of layout (all pages)
 * ^G<y>,<x>,<pic>[enter] Draw a graphic and push margins
 * ^P[enter]              start new page, must be the first chars in a layout
 * ^L<x>,<y>[ENTER]       Locate to a specific spot, x in pixels, y in lines
 *
 * The original trusted the text to end in ^E; here the chunk is copied with
 * a terminating zero and every scan stops there too.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "id_ca.h"
#include "orig.h"

#define BACKCOLOR 0x11

#define WORDLIMIT 80
#define FONTHEIGHT 10
#define TOPMARGIN 16
#define BOTTOMMARGIN 32
#define LEFTMARGIN 16
#define RIGHTMARGIN 16
#define PICMARGIN 8
#define TEXTROWS ((200 - TOPMARGIN - BOTTOMMARGIN) / FONTHEIGHT)
#define SPACEWIDTH 7
#define SCREENPIXWIDTH 320
#define SCREENMID (SCREENPIXWIDTH / 2)

static int pagenum, numpages;

static unsigned leftmargin[TEXTROWS], rightmargin[TEXTROWS];
static const char *text, *textstart;
static unsigned rowon;

static int picx, picy, picnum, picdelay;
static boolean layoutdone;

static void RipToEOL(void)
{
    while (*text && *text++ != '\n') { /* scan to end of line */
    }
}

static int ParseNumber(void)
{
    char ch;
    char num[80], *numptr;

    /* scan until a number is found */
    ch = *text;
    while (ch && (ch < '0' || ch > '9')) {
        ch = *++text;
    }

    /* copy the number out */
    numptr = num;
    while (ch >= '0' && ch <= '9' && numptr < num + sizeof(num) - 1) {
        *numptr++ = ch;
        ch = *++text;
    }
    *numptr = 0;

    return atoi(num);
}

/* Call with text pointing just after a ^G; upon exit text points to the start of next line */
static void ParsePicCommand(void)
{
    picy = ParseNumber();
    picx = ParseNumber();
    picnum = ParseNumber();
    RipToEOL();
}

static void ParseTimedCommand(void)
{
    picy = ParseNumber();
    picx = ParseNumber();
    picnum = ParseNumber();
    picdelay = ParseNumber();
    RipToEOL();
}

/* ^T: update the screen, wait the delay, then draw the picture */
static void TimedPicCommand(void)
{
    ParseTimedCommand();

    VW_UpdateScreen();

    SetTimeCount(0);
    while (GetTimeCount() < picdelay && !orig_quit) {
        IN_Pump();
    }

    VWB_DrawPic(picx & ~7, picy, picnum);
}

static boolean PicSize(int chunk, int *width, int *height)
{
    if (!pictable || chunk < STARTPICS || chunk >= STARTPICS + NUMPICS) {
        return false;
    }
    *width = pictable[chunk - STARTPICS].width;
    *height = pictable[chunk - STARTPICS].height;
    return true;
}

static void HandleCommand(void)
{
    int i, margin, top, bottom;
    int picwidth, picheight, picmid;

    switch (toupper((byte)*++text)) {
    case 'B':
        picy = ParseNumber();
        picx = ParseNumber();
        picwidth = ParseNumber();
        picheight = ParseNumber();
        VWB_Bar(picx, picy, picwidth, picheight, BACKCOLOR);
        RipToEOL();
        break;
    case ';': /* comment */
        RipToEOL();
        break;
    case 'P': /* ^P is start of next page, ^E is end of file */
    case 'E':
        layoutdone = true;
        text--; /* back up to the '^' */
        break;

    case 'C': /* ^c<hex digit> changes text color */
        i = toupper((byte)*++text);
        if (i >= '0' && i <= '9') {
            fontcolor = (byte)(i - '0');
        } else if (i >= 'A' && i <= 'F') {
            fontcolor = (byte)(i - 'A' + 10);
        }

        fontcolor = (byte)(fontcolor * 16);
        i = toupper((byte)*++text);
        if (i >= '0' && i <= '9') {
            fontcolor = (byte)(fontcolor + i - '0');
        } else if (i >= 'A' && i <= 'F') {
            fontcolor = (byte)(fontcolor + i - 'A' + 10);
        }
        if (*text) {
            text++;
        }
        break;

    case '>':
        px = 160;
        text++;
        break;

    case 'L':
        py = (int16_t)ParseNumber();
        rowon = (unsigned)(py - TOPMARGIN) / FONTHEIGHT;
        if (rowon >= TEXTROWS) {
            rowon = TEXTROWS - 1; /* not in the original, which trusted the text */
        }
        py = (int16_t)(TOPMARGIN + rowon * FONTHEIGHT);
        px = (int16_t)ParseNumber();
        RipToEOL();
        break;

    case 'T': /* ^Tyyy,xxx,ppp,ttt waits ttt tics, then draws pic */
        TimedPicCommand();
        break;

    case 'G': /* ^Gyyy,xxx,ppp draws graphic */
        ParsePicCommand();
        VWB_DrawPic(picx & ~7, picy, picnum);
        if (!PicSize(picnum, &picwidth, &picheight)) {
            break;
        }

        /* adjust margins */
        picmid = picx + picwidth / 2;
        if (picmid > SCREENMID) {
            margin = picx - PICMARGIN; /* new right margin */
        } else {
            margin = picx + picwidth + PICMARGIN; /* new left margin */
        }

        top = (picy - TOPMARGIN) / FONTHEIGHT;
        if (top < 0) {
            top = 0;
        }
        bottom = (picy + picheight - TOPMARGIN) / FONTHEIGHT;
        if (bottom >= TEXTROWS) {
            bottom = TEXTROWS - 1;
        }

        for (i = top; i <= bottom; i++) {
            if (picmid > SCREENMID) {
                rightmargin[i] = (unsigned)margin;
            } else {
                leftmargin[i] = (unsigned)margin;
            }
        }

        /* adjust this line if needed */
        if ((unsigned)px < leftmargin[rowon]) {
            px = (int16_t)leftmargin[rowon];
        }
        break;

    case '\0': /* not in the original: the text ends without ^E */
        layoutdone = true;
        break;
    }
}

static void NewLine(void)
{
    char ch;

    if (++rowon == TEXTROWS) {
        /* overflowed the page, so skip until next page break */
        layoutdone = true;
        while (*text) {
            if (*text == '^') {
                ch = (char)toupper((byte)*(text + 1));
                if (ch == 'E' || ch == 'P') {
                    return;
                }
            }
            text++;
        }
        return;
    }
    px = (int16_t)leftmargin[rowon];
    py = (int16_t)(py + FONTHEIGHT);
}

static void HandleCtrls(void)
{
    const char ch = *text++; /* get the character and advance */

    if (ch == '\n') {
        NewLine();
    }
}

static void HandleWord(void)
{
    char word[WORDLIMIT];
    int wordindex;
    uint16_t wwidth, wheight;
    int newpos;

    /* copy the next word into [word] */
    word[0] = *text++;
    wordindex = 1;
    while ((byte)*text > 32 && wordindex < WORDLIMIT - 1) { /* the original quit past the limit */
        word[wordindex++] = *text++;
    }
    word[wordindex] = 0;

    /* see if it fits on this line */
    VW_MeasurePropString(word, &wwidth, &wheight);

    while ((unsigned)px + wwidth > rightmargin[rowon]) {
        NewLine();
        if (layoutdone) {
            return; /* overflowed page */
        }
    }

    /* print it */
    newpos = px + wwidth;
    VWB_DrawPropString(word);
    px = (int16_t)newpos;

    /* suck up any extra spaces */
    while (*text == ' ') {
        px += SPACEWIDTH;
        text++;
    }
}

/* Clears the screen, draws the pics on the page, and word wraps the text. */
static void PageLayout(boolean shownumber)
{
    int i;
    byte oldfontcolor;
    char ch, str[40];

    oldfontcolor = fontcolor;

    fontcolor = 0;

    /* clear the screen */
    VWB_Bar(0, 0, 320, 200, BACKCOLOR);
    VWB_DrawPic(0, 0, H_TOPWINDOWPIC);
    VWB_DrawPic(0, 8, H_LEFTWINDOWPIC);
    VWB_DrawPic(312, 8, H_RIGHTWINDOWPIC);
    VWB_DrawPic(8, 176, H_BOTTOMINFOPIC);

    for (i = 0; i < TEXTROWS; i++) {
        leftmargin[i] = LEFTMARGIN;
        rightmargin[i] = SCREENPIXWIDTH - RIGHTMARGIN;
    }

    px = LEFTMARGIN;
    py = TOPMARGIN;
    rowon = 0;
    layoutdone = false;

    /* make sure we are starting layout text (^P first command) */
    while (*text && (byte)*text <= 32) {
        text++;
    }

    if (*text != '^' || toupper((byte)*++text) != 'P') {
        fprintf(stderr, "PageLayout: Text not headed with ^P\n");
        fontcolor = oldfontcolor;
        return;
    }

    RipToEOL();

    /* process text stream */
    do {
        ch = *text;

        if (!ch) {
            layoutdone = true; /* not in the original: no ^E at the end */
        } else if (ch == '^') {
            HandleCommand();
        } else if (ch == 9) {
            px = (int16_t)((px + 8) & 0xf8);
            text++;
        } else if ((byte)ch <= 32) {
            HandleCtrls();
        } else {
            HandleWord();
        }

    } while (!layoutdone);

    pagenum++;

    if (shownumber) {
        snprintf(str, sizeof(str), "pg %d of %d", pagenum, numpages);
        py = 183;
        px = 213;
        fontcolor = 0x4f; /* 12^BACKCOLOR */

        VWB_DrawPropString(str);
    }

    fontcolor = oldfontcolor;
}

/* Scans for a previous ^P */
static void BackPage(void)
{
    pagenum--;
    while (text > textstart) {
        text--;
        if (*text == '^' && toupper((byte)*(text + 1)) == 'P') {
            return;
        }
    }
}

/* Scans the layout until a ^E, counting pages (the original cached the graphics too) */
static void CacheLayoutGraphics(void)
{
    char ch;

    numpages = pagenum = 0;

    while (*text) {
        if (*text == '^') {
            ch = (char)toupper((byte)*++text);
            if (ch == 'P') { /* start of a page */
                numpages++;
            }
            if (ch == 'E') { /* end of file */
                break;
            }
            if (ch == 'G') {
                ParsePicCommand();
            }
            if (ch == 'T') {
                ParseTimedCommand();
            }
        } else {
            text++;
        }
    }
    text = textstart;
}

/* not in the original: the pad turns pages, A and B as Enter and Esc */
static byte PadScan(void)
{
    static int held;
    int dx, dy, jb = 0, now = 0, pressed;

    if (!orig_hooks.pad_menu || !orig_hooks.pad_menu(&dx, &dy, &jb)) {
        held = 0;
        return 0;
    }
    now = jb & 3;
    if (dx < -60 || dy < -60) {
        now |= 4;
    } else if (dx > 60 || dy > 60) {
        now |= 8;
    }
    pressed = now & ~held;
    held = now;
    if (pressed & 1) {
        return sc_Enter;
    }
    if (pressed & 2) {
        return sc_Escape;
    }
    if (pressed & 4) {
        return sc_LeftArrow;
    }
    if (pressed & 8) {
        return sc_RightArrow;
    }
    return 0;
}

static void ShowArticle(const char *article)
{
    int16_t oldfontnumber;
    boolean newpage, firstpage;

    text = textstart = article;
    oldfontnumber = fontnumber;
    fontnumber = 0;
    VWB_Bar(0, 0, 320, 200, BACKCOLOR);
    CacheLayoutGraphics();

    newpage = true;
    firstpage = true;

    do {
        if (newpage) {
            newpage = false;
            PageLayout(true);
            VW_UpdateScreen();
            if (firstpage) {
                VL_FadeIn(0, 255, gamepal, 10);
                firstpage = false;
            }
        }

        orig_lastscan = 0;
        (void)PadScan(); /* the button that opened this is still down */
        while (!orig_lastscan && !orig_quit) {
            IN_Pump();
            if (!orig_lastscan) {
                orig_lastscan = PadScan();
            }
        }
        if (orig_quit) {
            break;
        }

        switch (orig_lastscan) {
        case sc_UpArrow:
        case sc_PgUp:
        case sc_LeftArrow:
            if (pagenum > 1) {
                BackPage();
                BackPage();
                newpage = true;
            }
            break;

        case sc_Enter:
        case sc_DownArrow:
        case sc_PgDn:
        case sc_RightArrow: /* the text already points at next page */
            if (pagenum < numpages) {
                newpage = true;
            }
            break;
        }

    } while (orig_lastscan != sc_Escape);

    IN_ClearKeysDown();
    fontnumber = oldfontnumber;
}

/* a text chunk with a terminating zero */
static char *LoadArticle(int chunk)
{
    static char article[64 * 1024];
    size_t len;

    CA_CacheGrChunk(chunk);
    len = CA_GrChunkSize(chunk);
    if (!grsegs[chunk] || !len) {
        return NULL;
    }
    if (len > sizeof(article) - 1) {
        len = sizeof(article) - 1;
    }
    memcpy(article, grsegs[chunk], len);
    article[len] = '\0';
    return article;
}

void HelpScreens(void)
{
    const char *article = LoadArticle(T_HELPART);

    if (article) {
        ShowArticle(article);
    }

    VW_FadeOut();
}

/* END ARTICLES */
void EndText(void)
{
    const char *article = LoadArticle(T_ENDART1 + gamestate.episode);

    if (article) {
        ShowArticle(article);
    }

    VW_FadeOut();
    SETFONTCOLOR(0, 15);
    IN_ClearKeysDown();
    if (orig_hooks.mouse_delta) {
        int dx, dy;
        orig_hooks.mouse_delta(&dx, &dy); /* Clear accumulated mouse movement */
    }
}
