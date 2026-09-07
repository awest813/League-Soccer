# Career training progression

The training screen now offers an ongoing plan alongside existing training-point sessions.
Changing plans is saved immediately and grants no development. Each completed simulated or
played fixture applies the plan once through the shared match-result handler.

| Plan | Base progress per match | Fitness after match wear |
| --- | --- | --- |
| Balanced | 3 | +2 |
| Development | 7 | -4 |
| Recovery | 0 | +10 |

100 progress earns one OVR point, capped at potential and 99. Players aged 21 or younger
receive two extra progress points; players aged 30 or older develop more slowly. Players
below 60 fitness or with an injury rest instead, recovering 10 fitness without development.
Academy prospects follow a separate Balanced schedule, even when the first team recovers.
Promotion preserves accumulated development.

An active Assistant Coach or Youth Coach grants one bonus progress point (two at skill 80+).
Only the strongest such coach contributes. Head Coach mode adds one point. Completed
Training Complex and Youth Academy facilities add one senior or two academy progress points,
respectively; duplicate facilities do not stack ongoing bonuses.

Squad fitness below an average of 90 gradually reduces attacking and defensive strength in
career simulations. This change does not modify the 3D match engine's physical model.

Save records append development and injury state; older player records default to zero
progress and healthy status. Missing or invalid plan values default to Balanced. Existing
training-point sessions retain their previous immediate effects.

## Verification

The career regression suite covers plan trade-offs, rest, age, staff/facilities, promotion,
potential limits, played/simulated progression, save compatibility and simulation fatigue.
The existing twelve-season balance audit remains part of that suite.

`tests/run_menu_layout_audit.py` includes a populated `career_training` route with 24 senior
players and eight academy prospects. It uses a disposable save and checks keyboard scrolling
through all 32 players and on-screen bounds. Screenshot and physical-controller verification
are separate from these automated checks.
