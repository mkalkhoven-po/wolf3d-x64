# Bug triage (QA board)

## Labels

| Label | Use |
|-------|-----|
| `type:bug` | Defect vs MVP / Met bar |
| `severity:crash` | Abort, freeze, hard lock |
| `severity:blocker` | Cannot complete checklist step |
| `severity:major` | Wrong gameplay, data loss |
| `severity:minor` | Workable | 
| `severity:polish` | Cosmetic |
| `area:*` | platform / video / audio / gameplay / data / qa / docs |
| `repro:always` / `sometimes` / `unknown` | As needed |
| `priority:p0`–`p2` | Scheduling |

## Project hygiene (important)

Closing an issue on GitHub **does not always** set Project Status to Done.

When you close a QA bug/story:

1. Merge PR with `Fixes #N` if code.  
2. Confirm issue **CLOSED**.  
3. On [Project #2](https://github.com/users/mkalkhoven-po/projects/2), set **Status → Done**.

(Delivery board Project #1 historically drifted on #22/#23 — same trap.)

## Bug report minimum

Template: **Bug report** on “New issue”.

- Steps, expected, actual  
- Commit SHA + WL1/WL6  
- Map # if in game  
- Severity  
- FR id if regression  
- Log snippet for smoke failures  

## Not bugs

- Full original AI / boss parity when FR is **Partial** — file as enhancement or document only.  
- Missing commercial data in CI.  
- Hermes chat private-link 404 previews.
