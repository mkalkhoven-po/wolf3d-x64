# Product exclusions

These are **out of scope** for the MVP and must not appear as acceptance criteria on MVP stories unless an epic explicitly re-opens them.

## Platform / toolchain

- [ ] DOSBox, PCem, or other full-system emulation as the ship vehicle
- [ ] Borland C++ 3.x / TASM build as a requirement
- [ ] 16-bit or WOW64 DOS executables
- [ ] Supporting non-Windows hosts in MVP (Linux/macOS later optional)
- [ ] Raw Direct3D/Vulkan engine rewrite of the raycaster

## Engine fidelity traps

- [ ] EMS / XMS page manager behaviour
- [ ] Real VGA Mode-X planar framebuffer and CRTC page flips
- [ ] Self-modifying / JIT wall scalers in executable memory
- [ ] Hardware Sound Blaster DMA / AdLib port I/O / PC speaker PIT hacks
- [ ] Keyboard interrupt vectors / timer ISR installation
- [ ] Byte-identical DOS `CONFIG.` / savegame compatibility (new format first)

## Product features (defer)

- [ ] Multiplayer / IPX / network
- [ ] Free look, jump, vertical motion, taller walls
- [ ] Modern physically based renderer
- [ ] Map editor / scripting / ZDoom-like mod platform
- [ ] Spear of Destiny, Japanese, Spanish SKUs (post-MVP)
- [ ] Controller support (nice-to-have after keyboard/mouse)
- [ ] Online leaderboards, achievements, cloud saves

## Legal / content

- [ ] Committing or redistributing commercial `VSWAP` / `VGAGRAPH` / map / audio data
- [ ] Claiming id trademark endorsement
- [ ] Mixing incompatible third-party GPL code into the tree without a documented license decision

## Process

- [ ] Big-bang single PR that “ports everything”
- [ ] Epics without a demoable exit criterion
