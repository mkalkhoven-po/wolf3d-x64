# Original ID Engine → modern port mapping (E2)

| Original (reference/original) | Modern path | Status (E2) |
|-------------------------------|-------------|-------------|
| `ID_HEADS.H` / `ID_HEAD.H` | `src/id/id_heads.h` + `wl_types.h` | Stub umbrella + sized types |
| `ID_MM.C/.H` | `src/id/id_mm.c/.h` | malloc wrappers, no EMS/XMS |
| `ID_PM.C/.H` | `src/id/id_pm.c/.h` | Stub (real VSWAP in E3) |
| `ID_CA.C/.H` | `src/id/id_ca.c/.h` | Stub codecs (E3) |
| `ID_VL.C/.H` + ASM | `src/id/id_vl.c/.h` | Palette stubs; present still SDL shell |
| `ID_VH.C/.H` + ASM | `src/id/id_vh.c/.h` | Pic blits stub (E4) |
| `ID_IN.C/.H` | `src/id/id_in.c/.h` | Stub keys; SDL ticks |
| `ID_SD.C/.H` + ASM | `src/id/id_sd.c/.h` | Silent stub (E7) |
| `ID_US*.C/.H` / `Quit` | `src/id/id_us.c/.h` | Quit to stderr/exit |
| Startup DOS / `C0.ASM` | CRT + `src/main.c` | Deleted concept |
| `WL_DR_A.ASM` etc. | *(none yet)* | Not in build; C later |

**Rules for E2+**

- No `dos.h`, `far` pointers, or ASM objects in the CMake target.
- `far` / `near` / `_seg` are empty macros in `wl_types.h` for transitional includes only — prefer plain pointers in new code.
- Real window present remains `src/platform/win_sdl/` until VL/VH own a chunky buffer (E4).
