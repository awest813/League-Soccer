# Menu audit and polish — 2026-09-06

## Changes

- Corrected reversed grid coordinates in Gameplay, Controller, gamepad setup, match options, forfeit confirmation, and pause/halftime statistics.
- Kept caption text inside its assigned width, preserved localized capitalization and UTF-8, and retained natural-width player overlays.
- Added button padding and separated slider labels, values, and tracks, including shorter tactics sliders.
- Removed phantom grid margins, stabilized column widths while scrolling, and revealed rows when focus moves into them. Bounded wrapped navigation so sparse grids cannot loop forever.
- Made long keyboard, graphics, league, owner, and roster lists scroll. The career roster no longer silently drops players after the first 18. Read-only squad/standings tables support arrow navigation.
- Fixed duplicate ownership of the Keyboard Setup frame, which caused a crash on shutdown.
- Sized history columns and halftime actions to fit their panels. Added visible Back actions to connected gamepad setup/mapping/function screens.
- Kept dropdowns on-screen and contained navigation while they are open. Empty dropdowns no longer try to select a nonexistent entry.
- Made dialog titles, content, and buttons visible and bounded. Confirmation dialogs default to Cancel. Detail dialogs retain Close as their safe default; Escape never invokes Delete, Release, Sign, or Load. The database picker has an explicit cancel handler.
- Fixed message text duplicating previously added paragraphs and hanging on an unbroken word longer than the wrapping limit. Dialog bodies now fit their panels; the database picker displays its contents within its viewport.

## Verification

- Windows Release build passed.
- 16 focused unit checks passed, covering layout, wrapping, difficulty, slider quantization, and localization.
- 19 standalone menu routes passed at 1280×720 in English.
- The 20-route menu/widget audit passed at 1024×768 in German. After the final text/dialog changes, the affected widget, Gameplay, Keyboard, and career-save routes were rechecked successfully.
- 11 existing league and quick-match smoke flows passed with bounds diagnostics. League creation, inbox, management, and team routes were rechecked after the final dialog changes.
- Runtime widget checks cover dialog visibility, directional focus, safe Escape handling for Confirm/Cancel and Close/Delete, long text, paragraph duplication, empty/edge dropdowns, and read-only scrolling.

## Repeatable checks

Run from the repository root, replacing the executable path for your build:

```sh
python tests/run_menu_layout_audit.py build-win/Release/gameplayfootball.exe
python tests/run_menu_layout_audit.py build-win/Release/gameplayfootball.exe --width 1024 --height 768 --language de
```

The runner uses temporary configurations and rejects crashes, timeouts, missing completion markers, and controls/text outside the screen. Set `menu_layout_audit` to `true` in a disposable configuration to enable the same bounds diagnostics on existing smoke flows. Normal play does not enable these diagnostics.

## Coverage limits

This is a source and automated runtime audit across the menu families and their shared widgets, not a visual sign-off of every page, dataset, translation, or gameplay state. Bounds checks do not prove that text is readable or that every pair of controls is free of overlap. No screenshot comparison was performed. Void Linux, physical-controller calibration/mapping, and every populated career/owner state still need hands-on verification. The standalone gamepad routes test missing-device fallbacks; league/quick-match flows exercise additional stateful pages.
