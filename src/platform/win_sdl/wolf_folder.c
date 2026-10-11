#include "wolf_folder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL.h>

#ifdef _WIN32
#ifndef COBJMACROS
#define COBJMACROS
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shobjidl.h>
#endif

#ifdef _WIN32
/* Windows always has its folder dialog */
static int picker_available(void)
{
    return 1;
}

/* the Windows folder dialog (IFileOpenDialog, pick folders) */
static int pick_folder(const char *title, char *out, size_t out_sz)
{
    IFileOpenDialog *dlg = NULL;
    IShellItem *item = NULL;
    PWSTR wpath = NULL;
    FILEOPENDIALOGOPTIONS opts = 0;
    WCHAR wtitle[128];
    const HRESULT init = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    int ok = -1;

    if (FAILED(CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
                                &IID_IFileOpenDialog, (void **)&dlg))) {
        goto done;
    }
    MultiByteToWideChar(CP_UTF8, 0, title, -1, wtitle, (int)(sizeof(wtitle) / sizeof(wtitle[0])));
    IFileOpenDialog_SetTitle(dlg, wtitle);
    IFileOpenDialog_GetOptions(dlg, &opts);
    IFileOpenDialog_SetOptions(dlg, opts | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
    ok = 0; /* the dialog is there: cancelling is "no folder", not "no dialog" */
    if (SUCCEEDED(IFileOpenDialog_Show(dlg, NULL)) &&
        SUCCEEDED(IFileOpenDialog_GetResult(dlg, &item)) &&
        SUCCEEDED(IShellItem_GetDisplayName(item, SIGDN_FILESYSPATH, &wpath))) {
        /* the rest of the game opens files with the ANSI functions */
        ok = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, wpath, -1, out, (int)out_sz, NULL,
                                 NULL) > 0;
    }

done:
    if (wpath) {
        CoTaskMemFree(wpath);
    }
    if (item) {
        IShellItem_Release(item);
    }
    if (dlg) {
        IFileOpenDialog_Release(dlg);
    }
    if (SUCCEEDED(init)) {
        CoUninitialize();
    }
    return ok;
}
#else
/* zenity (GNOME and others) or kdialog (KDE) installed */
static int picker_available(void)
{
    return system("command -v zenity >/dev/null 2>&1 || command -v kdialog >/dev/null 2>&1") == 0;
}

/* the desktop's folder dialog through zenity (GNOME and others) or kdialog (KDE) */
static int pick_folder(const char *title, char *out, size_t out_sz)
{
    static const char *const tools[] = {
        "zenity --file-selection --directory --title=\"%s\" 2>/dev/null",
        "kdialog --getexistingdirectory \"$HOME\" --title \"%s\" 2>/dev/null",
    };
    size_t t;

    for (t = 0; t < sizeof(tools) / sizeof(tools[0]); t++) {
        char cmd[512];
        char which[64];
        FILE *p;
        size_t n;

        snprintf(which, sizeof(which), "command -v %.*s >/dev/null 2>&1",
                 (int)strcspn(tools[t], " "), tools[t]);
        if (system(which) != 0) {
            continue; /* not installed */
        }
        snprintf(cmd, sizeof(cmd), tools[t], title);
        p = popen(cmd, "r");
        if (!p) {
            continue;
        }
        out[0] = '\0';
        if (!fgets(out, (int)out_sz, p)) {
            out[0] = '\0';
        }
        pclose(p);
        n = strlen(out);
        while (n && (out[n - 1] == '\n' || out[n - 1] == '\r')) {
            out[--n] = '\0';
        }
        return n > 0;
    }
    return -1;
}
#endif

/* "Choose folder..." or "Quit"; 1 for choose */
static int ask(const char *text, const char *choose)
{
    const SDL_MessageBoxButtonData buttons[] = {
        {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Quit"},
        {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, choose},
    };
    const SDL_MessageBoxData box = {SDL_MESSAGEBOX_INFORMATION, NULL, "Wolf3D x64", text,
                                    (int)(sizeof(buttons) / sizeof(buttons[0])), buttons, NULL};
    int pressed = 0;

    if (SDL_ShowMessageBox(&box, &pressed) != 0) {
        return -1; /* no message box (no display) */
    }
    return pressed == 1;
}

int Wolf_ChooseDataFolder(int (*check)(const char *folder), char *out, size_t out_sz)
{
    const char *text =
        "Wolf3D x64 needs the Wolfenstein 3-D game files: the free shareware version "
        "or the full version.\n\n"
        "Choose the folder that holds them (VSWAP.WL1 and the other .WL1 files for the "
        "shareware, or the same names ending .WL6 for the full version).";
    const char *choose = "Choose folder...";

    if (!picker_available()) {
        return -1; /* nothing to choose with: the caller explains --data */
    }
    for (;;) {
        int r = ask(text, choose);

        if (r != 1) {
            return r; /* Quit (0), or no message box at all (-1) */
        }
        r = pick_folder("Choose the folder with the Wolfenstein 3-D game files", out, out_sz);
        if (r < 0) {
            return -1; /* no folder dialog on this system: the caller explains --data */
        }
        if (r > 0 && check(out)) {
            return 1;
        }
        if (r > 0) {
            text = "That folder has no Wolfenstein 3-D game files.\n\n"
                   "Choose the folder with VSWAP.WL1 and the other .WL1 files (the shareware) "
                   "or the same names ending .WL6 (the full version).";
            choose = "Choose another...";
        }
        /* cancelled: ask again */
    }
}
