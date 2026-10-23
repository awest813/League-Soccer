# Gameplay and Career gap list

Date: 2026-10-21
Scope: the in-match game (`src/onthepitch`, `src/menu/ingame`) and Career mode
(`src/menu/career`, `src/data/careerdata.*`). The standalone League mode was not
audited. A second pass covering controls (input devices, remapping, menus and
in-match handling) is in [section 4](#4-controls), and a third pass covering
menu and in-match UI is in [section 5](#5-ui-and-in-match-ui). [Section 6](#6-career-second-pass)
records a second career pass that fixed the career P0 defects, and [section 7](#7-database)
covers the database.

This document has three parts:

1. **Fixed in this pass**: defects that were confirmed in code and fixed, with
   regression coverage where the code can be tested without the engine.
2. **Known defects not yet fixed**: bugs confirmed in code but left for a
   follow-up, because they need a design decision or a hands-on playtest.
3. **Feature gaps**: things that are missing, stubbed, or fake, grouped by
   priority.

Priority key:

- **P0**: corrupts results or saves, or blocks a mode's season loop.
- **P1**: a core feature players expect from this kind of game.
- **P2**: depth and polish.

Line references are approximate where this pass edited the file; the function
name is the stable anchor.

---

## 1. Fixed in this pass

### Gameplay

| Area | Defect | Fix |
| --- | --- | --- |
| Match rules | Every match tied after 90' went to extra time, including league and career fixtures. A goal in extra time was then recorded as the league or career result. | `Referee::Process` ends the match at 90' unless `match_extra_time` is enabled in config (default off). Penalties still do not exist (see gaps). |
| Fatigue | Fatigue relief was applied at half-time only, not before extra time. | `Match::SetMatchPhase` also relaxes fatigue entering the first period of extra time. |
| Threads | About one career-match smoke run in nine aborted with `free(): invalid pointer` mid-match. The game thread drew on-screen messages ("Advantage", "Offside!", cards, goals) and the goal count while the graphics thread redrew the scoreboard clock and player names every frame, and Tab toggled the stats overlay from the menu. SDL_ttf/FreeType are not thread-safe, and the same caption could be drawn from two threads. | Every TTF call takes one text lock (`utils/textrenderlock.hpp`). `Match::SpamMessage` and `ToggleStatsOverlay` queue the request and `Match::Put` applies it, so in-match HUD widgets are only drawn on the graphics thread. Checked with AddressSanitizer (14 runs: career, full, gamepad and quick matches, league and career menus) and ThreadSanitizer (no reports on text or captions). |
| Threads | `Gui2Image::Redraw`/`SetZoom` took a surface lock and then the image lock, the reverse of `Image2D::SetImage` (a potential deadlock found by ThreadSanitizer). | They use the surface they already hold. |
| Tests | `CareerLongRunAudit.TwelveSeasonsFourPersonaTiers` gave a different result each run despite its seed (TierD average finish 8.1–9.4 against a limit of 8.0), and failed once in CI-like runs. Youth scouting used its own `random_device` generator. | Scouting uses the shared career RNG, so seeded runs repeat exactly. |
| AI | `TeamAIController::CalculateDynamicRoles` never advanced its iterator when the first active player was not the goalkeeper. This was an infinite loop on the game thread. | The iterator now advances. |
| Discipline | Abandoning a match when a team fell to six players kept the live score, so the offending team could "win". | `Player::SendOff` awards the match 3-0, or keeps the current margin if it is larger (IFAB Law 3). |
| Stats | Fouls were counted for every body-collision trip, including ones the referee ignored. Sliding-tackle fouls the referee did call were never counted. | Fouls are counted in `Referee::TripNotice` at the point a foul is registered, including fouls where advantage is played. |
| Stats | The full-time possession figures were rounded independently and could add up to 101%. | The second figure is derived as `100 - first`. |
| Pause menu | The minute was clamped at 90', so stoppage and extra time showed "(90')". | It shows period-relative stoppage time: `45+2'`, `90+3'`, `105+1'`, `120+1'`. |
| Career/match flow | Quitting a career 3D match left the fixture armed. The next Quick Match result was written into the career and advanced the week. | `PreQuitPage::GoMenu` clears the career fixture. Starting a Quick Match clears any armed career or league fixture, which also covers backing out of Match Options. |

### Career

| Area | Defect | Fix |
| --- | --- | --- |
| Transfers | Free agents and transfer targets read `players.base_stat` (a 0-1 float) as an integer, so every one had OVR 0. The free-agent filter `base_stat <= 70` matched everyone. | Values are read as a double and scaled to 1-99; the filter is now `<= 0.70`. |
| Transfers | The market and free-agent pool could offer the user's own players. | Both queries exclude the user's club. The free-agent pool is reseeded once the club is known. |
| Positions | The database stores Football Manager notation (`D C`, `AM RLC, F C`, `ST`). Only `GK` and `DM` were recognised, so career squads were almost all "CM". This flattened goal-scorer weights and position-focused training. | Added `CareerCommon::PositionFromDbRole`, used for the career squad, free agents and transfer targets. |
| New career | `StartCareer` filled in the club, squad and budgets after `CreateNewCareer` had already saved. Quitting before the first save produced a club-less, unplayable career. | The populated career is saved and autosaved at the end of `StartCareer`. |
| Contracts | Signed free agents kept 0 contract years and walked out at the next rollover. | Signing gives at least a 2-year contract and records the contract wage. |
| Contracts | A contract expiring at rollover did not return the player's wage to the wage budget. | The wage is refunded, as `ReleasePlayer` already did. |
| Ageing | Academy prospects and free agents never aged, so prospects kept the under-21 development bonus forever. | Both age once per rollover; departing players are not aged twice. |
| Finance | Each ticket-price click moved fan base and satisfaction relative to a fixed 40. Toggling between two prices raised both without limit. | The change is measured from the previous price, so toggling is neutral. |
| Finance | Sponsor revenue was paid at signing and again at season end. Active sponsors were re-offered, so identical deals stacked. | Revenue is paid only at season end. Active sponsors are not re-offered. |
| Finance | `transferSpending` and `transferIncome` were never reset, so every past fee was charged again each season. | Both are reset after the season's finances are processed. |
| Training | Individual training matched `databaseID == controlledEntityID`. Without a controlled identity it trained every player with ID 0 (all signings and youth). | It now requires a uniquely identified controlled player. Squad sessions remain unavailable in Player mode. |
| Standings | The standings query pasted the saved league name into SQL, so an apostrophe broke it. | It resolves the league from the club's integer ID. |

### Localization

- `en.ini` contained a verbatim second copy of about 185 lines (settings, HUD and
  match options). The loader keeps the last value it reads, so the first copy was
  dead and has been removed.
- Each locale defined `ingame_goal` twice with different punctuation. The value
  already in effect is kept; `match.cpp` appends the "!".
- `de`, `es`, `fr` and `pt` each lacked about 300 of the English keys (mostly
  career, plus gameplay settings and match options), so those screens fell back
  to English. All four are complete now, matching each file's existing terms;
  `LocalizationTest.EveryLanguageHasEveryEnglishKey` checks every key and its
  `{n}` placeholders. Older lines written without accents were corrected
  ("Schuesse", "Posesion", "Precisao", "Periode").
- The career matchday page was hard-coded English with invented figures
  (weather, "Attendance 96%", corners and fouls derived from the shot count,
  "Pass Accuracy 86%", the first scorer as Man of the Match) and duplicate Play
  and Simulate buttons. It now shows the fixture, the real tactic and form, and
  after a simulation only the scorers, shots and possession, all translated.
  Its footer counted only the current visit ("Matches: 0" beside a form guide);
  it shows the season so far.
- The transfer bid page was English and promised what the logic does not do:
  "Guaranteed Acceptance", an 85% "Negotiated Offer" and a 70% offer that are in
  fact always rejected unless raised from My Bids, a flat 5% agent fee (it has a
  EUR 25,000 minimum), and an invented "Agent Patience 80%" and "Interest:
  HIGH". It now uses the existing translated keys, labels the lower offers as
  rejected unless raised, and computes the fee with the same
  `CareerTransfers::AgentFeeFor` the bid uses.
- League standings: the table was a space-padded string, so its columns did not
  line up in the proportional font; it is a grid with a translated header now.
  The subtitle was English with a fixed "of 38". The Golden Boot listed ten
  world stars from other leagues on a formula ("Saka, Arsenal" in an Eredivisie
  career); its rivals now come from the clubs in the user's own league table and
  their goals, and it says "1 goal". Emoji the font cannot draw were removed.
- Save slots were labelled "SLOT 1: ... Trust ... Budget" in English, on one
  line; they are translated, with the details on a second line, and Back is no
  longer a full-width bar.
- New layout-audit routes `career_bid_detail` and `career_standings`.
- Career events (the board room's log) and board objectives were English;
  they are translated when recorded, in all five languages. The board room
  printed each event's type twice ("[transfer] transfer: ..."). Three unused
  helpers that produced invented English news and opponent previews
  (`GetNewsHeadlines`, `GetNextOpponentPreview`, `GetConditionArrow`) were
  removed.

### Verification

Linux Debug build with tests:

- **Unit tests:** all 273 pass. That includes 7 new tests in
  `tests/career/career_audit_regression_test.cpp`. With the career fixes reverted,
  6 of the 7 fail; the seventh tests the new helper.
- **Smoke tests (under Xvfb):** the Quick Match, Full Match and Career menu tests
  pass.
- **Not done:** physical-controller testing and hands-on 3D playtesting.

---

## 2. Known defects not yet fixed

### Gameplay

| Pri | Location | Defect | Why it was deferred |
| --- | --- | --- | --- |
| P0 | `playerbase.cpp` `PlayerBase::ResetSituation` | `injuryLevel = 0` runs on every restart, so collision injuries (up to a 40% stat penalty) last only until the next throw-in. | Keeping injuries for the whole match is a balance change. It needs a design for decay and forced substitutions, plus a playtest. |
| P1 | `matchdata.hpp` `AddPassAttempt`/`RecordBallTouch`; `humanoid.cpp` touch callers | Pass completion is only checked on trap and ball-control touches. Slides, headers and keeper interceptions leave a pass pending, and one-touch passes are undercounted. | The fix is to record every touch through `Team::SetLastTouchPlayer`, which needs a scripted-match regression. |
| P1 | `humanoid.cpp` shot-on-target projection | "On target" uses a straight-line projection with no gravity or bounce. Dipping or bouncing shots are miscounted. | Should sample `Ball::Predict` at the goal plane. |
| P1 | `team.cpp` `Team::ResetSituation` | After a goalkeeper red card, the restart can pick the deactivated keeper (`players.at(0)`), and the human can be attached to him. | Needs an in-engine red-card scenario to verify the fix. |
| P1 | `player.cpp` `Player::SendOff` (GK branch) | The stand-in keeper is not given the GK dynamic role or the goalkeeper kit, and is chosen arbitrarily. | Same as above. |
| P2 | `Match::Put` and the radar, player HUD and officials | ThreadSanitizer still reports unsynchronised reads of single values written by the game thread: player and ball positions, fatigue, animation types, `actualTime_ms`, the pause flag and image visibility flags. At worst a HUD element is a frame out of date; none of them touch the heap. | Fixing them means reading these through the put buffers like the 3D models do, a wider refactor. |
| P2 | `gameplan.cpp` / `teamdata.cpp` `SwitchPlayers` | The in-match Lineup button (non-release builds) swaps `PlayerData` order but not live players, so names and stats get mismatched. | Hidden in release builds; replace with real substitutions. |
| P2 | `teamAIcontroller.cpp` set-piece positioning | The Set Piece Editor's goal-kick depth/width and corner depth are ignored. | Either apply them or remove the controls. |
| P2 | `match.cpp` collision trips / `Referee::TripNotice` | Standing-tackle trips (`tripType` 1) can never be fouls. | Referee tuning. |
| P2 | `humancontroller.cpp` shot direction | Keyboard shots force full auto-direction and override the shot-assist slider. | Possibly intended for digital input; needs a decision. |
| P2 | `team.cpp` kit fallback | The missing-kit fallback differs between `InitPlayers` (`almost_white`) and `SetKitNumber` (`white`). | Cosmetic. |

### Career

| Pri | Location | Defect |
| --- | --- | --- |
| P1 | `career_finance.cpp` `InitializeOwnerData` | Player wages are weekly but counted once per season, while staff salaries are annual. |
| P1 | `career_staff.cpp` `HireStaff` | The first salary is charged at hire and again at season end. |
| P1 | `career_sim.cpp` `GetTopScorers` | The Golden Boot compares the user's all-time goals with other players' season goals. |
| P1 | `career_training.cpp` `ScoutYouthPlayer`; `career_sim.cpp` | Results can't be reproduced: a separate `random_device` RNG, `std::hash` for opponent strength, and the implementation-defined `std::poisson_distribution`. Browsing staff or free-agent pages advances the career RNG. |
| P1 | `careerpages.cpp` matchday | If the save fails, the next attempt re-simulates the match, so the result can be re-rolled. |
| P2 | `CareerDatabase::ExtendContract`, press conference, `MotivatePlayer` | Free and unlimited, so they can be farmed for morale, reputation and board confidence. |
| P2 | `career_persistence.cpp` | Inbox, objective and bid enum values are cast from the save without range checks. |
| P2 | Several files | Reputation is clamped to [-100,100] in `AddEvent` but [0,100] elsewhere. Board-room events print their type twice. `CareerSim::Process3DMatchResult` (used only by tests) would advance the week twice. |

---

## 3. Feature gaps

### Gameplay

| Pri | Feature | State |
| --- | --- | --- |
| P1 | Substitutions | **Absent.** Benches are loaded but never used: no substitution menu, limits or AI subs. Blocks real injuries and fatigue management. |
| P1 | Knockout rules | Extra time is now opt-in (`match_extra_time`), but no menu exposes it and no competition sets it. **Penalty shootout absent:** the `Penalties` phase immediately ends the match. `onthepitch/matchclock.hpp` has the right model but only tests use it. |
| P1 | Set pieces | The taker is the nearest player. There are no designated takers, no aiming, power or curl control for free kicks, corners or penalties, and the kick-off is a forced short pass. |
| P1 | Injuries | **Stub** (see §2): no stretcher, no forced substitution, no carry-over into Career. |
| P2 | Goalkeeper control | Player switching skips the keeper. |
| P2 | Skill moves | Only the fake shot and knock-on exist; the sidestep dribble is disabled (`playercontroller.cpp`, "not implemented yet"). |
| P2 | Handball / VAR | Absent. |
| P2 | Weather | Physics only, driven by hidden config keys (`match_wind_x/y`, `match_wetness`). There is no menu option and no rain or wind visuals. |
| P2 | Statistics | No cards, corners, offsides or saves. Goal scorers and minutes now show on the full-time screen, and goals count towards Man of the Match. |
| P2 | Replays | `sig_OnShortReplayMoment` is never emitted; there is no highlights reel. |
| P2 | Commentary | Absent; only crowd audio and on-screen messages. |

Present and working: advantage, offside with restart exemptions, cards
(including deferred cautions), the in-match tactics sliders, the stamina HUD, the
radar, the stats overlay, goal replays, celebrations, and the difficulty
multipliers for the CPU.

### Career: shared systems

| Pri | Feature | State |
| --- | --- | --- |
| P1 | Fixture list | No round-robin schedule: the opponent is `(week*3) % N` over every database team in every league, so opponents repeat about 5 times a season. Opponent strength is a hash of the club name. |
| P1 | Competitions | There are no cups or continental competitions ("Continental Cup" is only a message). Promotion and relegation are computed but never applied. |
| P1 | Transfer market | Bids resolve instantly. There are no transfer windows or deadlines, no outgoing sales or loans, and no AI clubs; "Agent Patience" is display-only and contract length is fixed at 3 years. |
| P1 | Contracts | No renewal negotiation, player refusals, release clauses or loans. Staff contracts expire silently. In Coach and Player careers the AI front office renews by a simple rule (rating near the squad average, or young with potential). |
| P1 | Squad selection | `squad.startingXI` and `bench` are unused. The automatic match squad (one keeper and the ten strongest outfielders) cannot be changed, and the simulation still averages the whole squad's strength. |
| P1 | Injuries | `InjuryStatus` is never set. Assists are credited in simulated matches but not yet in 3D matches. |
| P2 | Youth | 25 name combinations and lookups by name (duplicate names are likely). There is no intake schedule; `scoutingNetworkLevel` can't be raised, and `YouthAcademyState` and `ScoutingState` are unused. |
| P2 | Staff | Only Assistant Coach and Youth Coach have any effect. Clicking a staff row fires them without confirmation, and there is no hiring cap. |
| P2 | Finances | Processed only at season end. Stadium condition, fan satisfaction, season tickets and debt are decorative. The upgrade revenue bonus is ignored, and the stadium rename is fixed. |
| P2 | Balance | Squads grow to about 80 overall within a few seasons, so a mid-table Coach club wins 9 of 12 titles in the long-run audit. Starters-only team strength would make this worse until the opponent pool is recalibrated. |
| P2 | Board | Low confidence has no consequence, objectives are owner-focused for every role, and there is no in-season progress display. |
| P2 | Morale and press | Morale comes only from goals, press, training and motivation. The press conference is one fixed question. |
| P2 | History and achievements | `seasonSummaries` is never written, so past seasons always show empty. `achievementdata` is unreferenced, and event timestamps are 0. |
| P2 | Matchday presentation | Pass accuracy, corners, fouls, player rating and the tactics label are fabricated. |
| P2 | Dead modules | `moraledata`, `youthacademy`, `staffdata`, `transfermarket`, `facilitydata` are unreferenced; `scoutingdata`/`draftdata` are used only by tests. |

### Career: by role

| Role | Pri | Gap |
| --- | --- | --- |
| Owner/GM | P1 | Delegation covers only the training plan. No AI handles transfers, line-ups or contracts. |
| Owner/GM | P2 | No player sales, loans, transfer/wage budget split, or coach hire/fire. |
| Coach | P1 | No line-up, formation or bench selection that affects results. |
| Coach | P2 | No resource or transfer requests to the front office, opponent scouting, or job market. |
| Player | P1 | There is no selection or bench status: the user's footballer is always in the match squad. A transfer request only sets a flag. |
| Player | P2 | No personal contract offers or renewals, and no personal objectives. |

### Localization

| Pri | Gap |
| --- | --- |
| P2 | Career events are now translated when recorded, but two arguments inside them stay English: the training focus area and the strategy name ("Focused training on Attacking"). Events recorded before this change keep their English text. |

---

## 4. Controls

Second pass, same date. Scope: `src/hid`, `src/managers/usereventmanager.cpp`,
controller settings and selection pages, GUI input (`src/utils/gui2`), and
human match control (`humancontroller.cpp`, player switching in `team.cpp`).

### Fixed

| Area | Defect | Fix |
| --- | --- | --- |
| Devices | Gamepads present at launch were added before the keyboard, so the keyboard was not controller 0. Keyboard rebinds were then never saved, the menu never got the pad's confirm/back buttons, and unplugging that pad left a stale entry. | The keyboard is inserted at index 0. |
| Devices | Menu confirm and back both defaulted to raw button 1, and a pad plugged in after launch never set them: A did nothing, B confirmed *and* went back. | Defaults are A=0, B=1. The first gamepad, at launch or hot-plugged, applies its mapping. Axis or duplicate mappings are refused. |
| Devices | Unplugging a pad mid-match deleted the `HIDGamepad` while the match still held its pointer (use-after-free). | Unplugged pads are retired, not deleted, and read as no input. |
| Devices | Key/button releases and hot-plug events were discarded while the window was unfocused. Alt-Tab while holding Sprint left it held for good. | Releases and hot-plug are always processed, and held input is cleared when focus is lost. |
| Gamepad | The default Xbox R2 (Dribble) was axis 2 negative, which is the left trigger at rest, so Dribble stuck on after the first LT pull and Sprint became Super Cancel. RT was unbound. | R2 is axis 5 positive. L2 uses axis 2 on Linux and axis 4 elsewhere, matching SDL's joystick axis order. |
| Gamepad | Uncalibrated triggers used a centre rest of 0, so they needed a half pull and a released trigger read as the opposite direction. | The default rest comes from SDL's initial axis state when it shows a trigger. |
| Gamepad | Stick axes got a per-axis deadzone of 0.2 before the radial one, snapping any angle within about 11° of a cardinal onto it. | Movement axes use only the radial deadzone; buttons and triggers keep theirs. |
| Gamepad | "Reset to defaults" reloaded the saved bindings, because saved keys take priority over defaults. | Reset erases the pad's saved mapping and calibration first. |
| Gamepad | Calibrating without moving an axis gave it a zero range, and it then read as dead. | Axes moved through less than 8000 units keep their previous calibration. |
| Robustness | SDL axis/button/hat indices were not range-checked; joystick events left axis values uninitialised; corrupt config mappings could index out of bounds. | Indices are checked, events are zeroed, and loaded mappings fall back to defaults. |
| Menus | Holding a key repeated every GUI tick from 250 ms to the OS repeat delay, then stopped once OS repeats refreshed the press time. A held D-pad never repeated. | Keys, D-pad and stick share one repeat: a step on press, then every 110 ms after 400 ms. OS auto-repeat events are ignored. |
| Menus | A released key could read as held for good: a race re-inserted it into the pressed-key map. | Uses `find()`. |
| Controller Select | Confirm/back were level-triggered: the A press that opened the page skipped it, and B from Team Select backed out twice. In a match, gamepad back discarded side changes. The hint promised the D-pad, which did nothing. | Confirm/back need a fresh press; in-match back saves like Escape; the D-pad moves teams. |
| Controller Select | With no gamepad, the keyboard started on "No team", so Enter silently started CPU vs CPU. | Without a gamepad, the keyboard starts on Home. |
| Keyboard | F1, the default Select key, was also a sun-lighting debug key in every build, which consumed it. | The debug key only works in non-release builds. |
| Keyboard | Remapping accepted Escape, Tab and F12, which have fixed meanings (pause, stats, quit). | They are rejected; the previous binding stays. |
| Match | Steady sprint tapping knocked the ball on with every tap: the press that completed a pair also started the next. | A completed pair resets the history. |
| Match | In Fully manual switching, Switch took control away from the dribbler, breaking LB+X (chip) and LB+A (one-two) and handing the ball to the AI. | No mode switches away from a player with sole possession (`ShouldSwitchOnPress`). |
| Match | When the obvious switch target was the keeper, the press did nothing. | The keeper is skipped and the next candidate is used. |
| Match | A defensive action (Pressure/Sliding) stayed armed after release until the next animation requeue, dropping attacking presses made meanwhile (e.g. shooting just after winning the ball). | Released defensive actions clear on the next tick. |
| Match | Holding an action past 500 ms locks its power (about 47%), but the HUD gauge kept filling to 100%. | The HUD shows the committed power once the action is dispatched. |
| Match | A zero-length aim input produced a zero shot direction. | The committed direction is kept. |
| Docs | README Controls listed WASD movement, Shift sprint, A/S/D passes and Space switch, none of which are the defaults. The Controller Select keyboard hint promised Space. | README rewritten from the actual defaults, with gamepad equivalents and combinations; "Space" removed from the hint in all five languages. |

Regression tests: `tests/onthepitch/controls_audit_test.cpp` covers sprint-tap
pairing, the switch decision, zero aim, axis-binding encoding, trigger rest,
the radial deadzone and menu repeat timing.

Verification (Linux Debug, Xvfb): all 280 unit tests pass. The Quick Match,
Full Match and scripted-gamepad match smoke tests pass; the gamepad smoke
drives a human player through `HumanController` for a full match. Not done:
physical controllers (stick feel, trigger defaults on Windows/macOS, menu
repeat timing) and hands-on match play.

### Follow-up: identity, disconnects and options

| Area | Before | Now |
| --- | --- | --- |
| Gamepad identity | Settings were keyed by `<name> #<connection index>`, so a pad lost or swapped its mapping when pads were connected in another order. | Keyed by SDL GUID (identical pads get `-2`, `-3` in connection order). Settings saved under the old key are copied over once. Menus show the pad's name. |
| Keyboard layouts | Bindings were saved as keycodes, so AZERTY/QWERTZ players got the QWERTY letters scattered across the board. | Saved as physical scancodes (keycodes kept for older builds) and resolved on the current layout, including when the OS layout changes. Defaults are physical positions. |
| Disconnect | An unplugged pad's team silently stopped responding, and later pads' entries in the controller setup pointed at the wrong devices. | The setup drops the pad and renumbers the rest. If it was playing, the match pauses with a notice and focus on Controller Select; resuming without reassigning hands that team to the CPU. |
| Presets | One fixed layout. | PES (unchanged default) and FIFA layouts on the gamepad setup page. |
| Options | No invert, deadzone or rumble settings. | Per-pad invert vertical, stick deadzone (15–30%) and rumble (off/half/full). |
| Rumble | Goal and woodwork rumble went to every connected pad. | Only pads controlling a team rumble (`Match::RumblePlayingPads`). |
| LB modifier | LB before an action braked the dribbler and fired a teammate run; after it did neither. | A run is requested on LB release only if no action was armed while it was held; braking needs LB held alone for 200 ms (`SwitchModifierTracker`). |
| Hardware profile caption | Started with an emoji the menu font cannot draw; not localized. | Localized, no emoji. |

Tests: `controls_audit_test.cpp` also covers the presets, option cycling,
GUID identity and legacy-key migration, controller-setup renumbering and the
LB modifier. A new smoke test (`data/menu_smoke_controller_disconnect.config`,
also in CI) attaches an SDL virtual gamepad, plays with it, unplugs it
mid-match and checks the prompt. The layout audit's `gamepad_setup_virtual`
route checks the new option buttons with a real `HIDGamepad`.

### Known defects not yet fixed

| Pri | Location | Defect | Why it was deferred |
| --- | --- | --- | --- |
| P1 | `humancontroller.cpp` dispatch, `humanoid.cpp` touch | Power is locked at the 500 ms dispatch, about 47% gauge, so a held pass/shot can never be stronger. The original comment assumed power could still grow before the touch. | Reading the gauge live at touch time makes held passes and shots noticeably stronger: a balance change that needs a playtest. The HUD is now honest about the locked value. |
| P1 | `humancontroller.cpp` possession context | HighPass has no +0.15 weight while Sliding has −0.15, so B on a contested ball leans to a sliding tackle. The code's own TODO says the weights assume one layout. | Feel change; needs a playtest. |
| P2 | `usereventmanager.cpp` / HID `Process` | Button state is level-sampled every 10 ms; a tap that starts and ends between two samples is lost (e.g. at 30 fps rendering). | Needs per-button "pressed since last sample" latching. |
| P1 | Player-mode lock (`HumanGamer::SetLockedPlayerID`) | With the controller locked to one footballer, a loose ball nearest that footballer can sit unplayed: teammates leave it to him, and play stalls around it. A scripted pad that only pushes one way hit this in about a third of smoke runs; before stoppage time the clock ran on past 800 minutes, and the half now ends at most two minutes after its added time. A real player would go for the ball, so it may only matter for idle input, but the AI never steps in. The career_match smoke test hands the side to the CPU once the lock is checked. | Needs a decision on when teammates should take over from a locked human, plus a playtest. |
| P2 | Fully-manual switching | Switching ignores the stick direction (`AI_GetBestSwitchTargetPlayer` never reads it). | Feature work. |
| P2 | Fully-manual switching | Actions can be armed during stoppages before a set piece. | Needs set-piece timing verified in-engine. |
| P2 | `HumanController::Reset` | Does not chain to `PlayerController::Reset`; harmless today. | Cleanup. |

### Feature gaps

| Pri | Gap |
| --- | --- |
| P1 | **SDL GameController API.** Devices are opened as raw joysticks, so layouts differ per platform and driver: PlayStation/Switch pads under SDL's HIDAPI driver get wrong defaults and no D-pad. `data/gamecontrollerdb.txt` is loaded but absent. Moving to `SDL_GameController` would replace the per-platform guesses fixed above. |
| P2 | **Controller Select while open.** It builds its device list when it opens, so a pad plugged in while it is showing appears only after reopening it. |
| P2 | **Remapping.** No conflict warning for duplicate bindings, no secondary keys, no keyboard presets, no trigger-threshold setting. The layout label still says PES/FIFA after individual functions are remapped. |
| P2 | **On-screen control legend.** Button glyphs for Xbox/PlayStation/Switch exist (`HIDGamepad::GetButtonGlyphPath`) but nothing displays them. |
| P2 | **Special button.** Mapped (Z / LT) but not read anywhere in a match. |
| P2 | **Fake shot.** It is a ball-control cut at dribbling pace; the `FakeShot` modifier it sets is never read, so there is no dedicated feint animation. (`docs/gameplay.md` now describes it as it is.) |
| P2 | **Second local player on one team.** The power gauge only shows for the first human on a team. |

## 5. UI and in-match UI

Audited by capturing frames of each menu route and of live matches under Xvfb
(`GF_CAPTURE_DIRECTORY`, now with `GF_CAPTURE_INTERVAL_MS` and
`GF_CAPTURE_LIMIT`), in all five languages.

### Fixed

| Area | Defect | Fix |
| --- | --- | --- |
| Focus colours | Focused buttons, icon selectors and sliders were briefly switched to dark navy text on the gold fill. | Reverted at the user's request: focused text is white with its dark outline again. |
| In-match HUD | The radar and player panels kept drawing over the pause menu, game-over screen and replay menu. | `Match::SetHUDVisible`; the in-match page shows the HUD and hides it when an overlay replaces it. |
| Game over | The OK button overlapped the man-of-the-match caption, and the stats grid left a large empty gap. | Tighter frame, stats grid, caption and button positions. |
| Replay menu | The replay banner covered the scoreboard, and the timer read "-0s" at the end. | Banner moved below the scoreboard; timer reads `elapsed / total (percent)`. |
| Scoreboard | The clock started as "0:00" and switched to "00:00" on the first update. | Starts at "00:00". |
| Player condition | Condition used ASCII marks ("^", "/^", "->", "\\v", "v"). | Unicode arrows (↑ ↗ → ↘ ↓), written as UTF-8 byte escapes so MSVC keeps them. |
| Sliders | Gameplay sliders and slider values were too small to read at 720p. | Taller gameplay sliders; larger value text. |
| Gamepad calibration | The panel ran off the right edge of the screen and had no title. | Fixed-size centred panel with a title. |
| Gamepad functions | Long labels were squeezed into a 10% column and shrank to a few pixels; modifier buttons did not line up with the rest. | 22% label column for every row, and a short dribble label for this page. |
| Localization | The keyboard, gamepad list, gamepad setup, calibration, button mapping, function, graphics and language pages, and the icon selector's "None selected", were hard-coded English. | 55 new keys in all five locale files; prompts with a device or control name use `TRF`. |
| Empty panels | The Settings, Controller, Audio, Language, Gamepads, Keyboard and Graphics pages drew a fixed 70–90% tall panel around a few rows. | Panels end just below their content (`Gui2Frame::SetSize` now resizes the background). Graphics' Back button moved into its panel. |
| Frame resizing | `Gui2Frame::SetSize` changed the frame's size but not its background, so the press-key, press-button and resolution-restart prompts drew a strip running off the right edge of the screen, and the restart prompt was sized to its first line only. | The background is redrawn at the new size; the restart prompt covers its widest line. |
| Gamepads page | With a pad connected, the Back button was added after the list was shown and never appeared. | Shown explicitly. |
| Version watermark | The version text sat under the second player's HUD panel, bottom right. | Moved to the top-right corner. |
| Key names | The keyboard page showed SDL's English key names ("Left Shift", "Up") in every language. | Arrows, Space, Enter, modifiers, navigation keys and keypad keys are translated (23 keys); letters and digits keep SDL's layout-aware name. |
| Kit clash | Home always wore kit 1 and away kit 2, whatever the colours. | The match samples each shirt's colour from its kit texture; when the away shirt is too close to the home one and its other kit stands out clearly better, the away side changes kit (`onthepitch/kitclash.hpp`). The kit options page now shows the kits actually worn. |
| Stoppage time | A half ended the first time the ball was near the halfway line after 45'/90'; no added time was calculated or shown. | As each period reaches its end the added time is set from its goals and cards (1–5 minutes, 1–2 in extra time; `StoppageMinutes` in `matchclock.hpp`), announced on screen and shown on the scoreboard as "+3". The referee ends the period once it has been played, with the ball near halfway or at most two minutes later. |
| Full-time screen | `Match` emitted `sig_OnGameOver` every frame once the match ended, re-creating the full-time page. It also had no scorers, and Man of the Match ignored goals. | Emitted once, when the game page is listening. Scorers and minutes (`45+2'` for stoppage time) are listed under the score, and goals raise a player's rating. |
| Tables | Standings, squad, transfer, bid, history and league-mode lists were space- or pipe-padded strings ("\| 0 \| 0 \|"), so columns did not line up in the proportional font, and league mode showed raw 0-1 stats. | `Gui2Table` (broadcast style after FIFA/PES/FM league tables): header bar with a gold rule, banded rows, zone strips, the user's row highlighted, aligned columns, "..." for long text, and selectable rows that scroll. Used for career standings and Golden Boot, squad, transfer market, My Bids, match history, league mode's table, squad (ratings out of 99) and calendar, and the Owner/GM staff, candidates and sponsor pages (unhappy staff, top candidates, final-year deals and offers the reputation cannot yet unlock are marked). Contract lengths read "1 yr" / "3 yrs". |
| Career hub | The overview was a few lines of small text in a large empty panel. | Stat cards (league position, W-D-L record, form, and budget, board confidence or the footballer's rating by role) above larger summary text. |
| Matchday and season | The matchday page was a column of small captions; the season review printed its summary as one caption whose line breaks never rendered, so it ran across the panel. | Matchday is a preview card (both clubs around a large "v", the score once played, then form and tactic or the result, scorers and stats) with Play and Simulate side by side. Season review is two rows of stat cards (position, prize money, budgets, squad, expiring contracts in red, and net worth and profit for Owner/GM). Both share the hub's card helper. |
| Buttons | Back buttons ranged from 3% to 3.6% tall at different heights; training's plan and session buttons were 3% tall. | Back buttons are 3.6% tall on a common bottom line; training buttons are 4.2%. |
| Smoke tooling | Layout-audit routes quit before slower pages had drawn, so captures showed the previous page. | `menu_smoke_quit_delay_ms` keeps the route open for a set time. |

### Known gaps

| Pri | Gap |
| --- | --- |
| P2 | **First match frame.** With software GL the first live frame after loading can be white before the pitch draws. Not seen with hardware GL; needs checking on a real GPU. |
| P2 | **Layout audit scope.** The audit flags controls outside the screen, not text that a caption shrinks to fit its box. Shrunken text still has to be found by looking at captures. |

---

## 6. Career, second pass

The career P0 defects from [section 2](#2-known-defects-not-yet-fixed) and
[section 3](#3-feature-gaps) are fixed, along with three related gaps. Each has a regression test in
`tests/career/career_audit_regression_test.cpp`.

| Area | Defect | Fix |
| --- | --- | --- |
| Transfers | Bids were saved without their transfer targets, so an accepted bid could never complete after Continue/Load, and the market was re-rolled on every load. | The market is saved with the bids (`target.N` lines) and restored on load. |
| Save slots | Lifecycle commits (matches, season rollover) always wrote slot 0, so playing a career loaded from slot 3 overwrote slot 0. | The database tracks the active slot: the one loaded or last saved to. Commits and Save go there. A career restored from the shared autosave commits to slot 0. |
| League settings | `leagueSettings` and `customLeague` were not saved, and custom-league Reset skipped the `CanManageClub` check. | Both are saved and loaded. Every expansion and custom-league action checks the permission and saves. |
| Appearances | `matchesPlayed` counted goals, not appearances: only scorers gained one, once per goal. | Each fixture gives one appearance to the match squad: one keeper and the ten strongest outfielders, plus the user's footballer in Player mode. Only that squad can score in simulated matches. |
| 3D match stats | Played 3D fixtures passed no scorers, so they recorded no personal stats. | `MatchData` records each goal's scorer by database ID; the career credits the user's scorers who are in its squad. |
| Standings | AI records were regenerated from a new seed each week, so points could go down. The bundled database has two clubs per league, so the table showed two clubs. Rank, prize money, TV revenue and the board used `EstimateLeaguePosition`, and the form guide was invented. | Other clubs' records are stored in the save and advance one round per user fixture. Every round pairs them, and the club meeting the user gets the mirrored score. Results come from a local, platform-independent RNG, so the career RNG is untouched. Lists of fewer than ten clubs are replaced by a 20-club default league. Rank, prize money, TV revenue, the board and the season summary use the table position. Form guides show real results. |
| Board | Objectives were judged against targets recomputed from reputation *after* the season's changes, and regenerating them wiped the outcome. | Targets are fixed and saved when objectives are set. They are judged before the rollover, and each outcome is recorded as a board event. |
| Wages | Recalculating a player's value (after every appearance and at rollover) overwrote their negotiated or extended wage. | Value updates no longer touch the wage. |
| 3D match squad | The 3D match used the club's database squad, so signings, releases and academy graduates never appeared on the pitch. | A one-shot `SquadOverride` gives `TeamData` the career squad: the match squad starts and up to seven substitutes sit on the bench. Starters are placed in formation slots by position (`data/squadslots.hpp`). Database players load as usual. Players not in the database (the user's footballer, academy graduates, signings) are built from their career rating and age. Roster players get unique IDs so their goals map back. |
| Player mode | A Player-mode career gave the user the whole team in 3D matches, and the user's footballer was not in the match. | The footballer is always in the match squad, and the human controller is locked to them while they are on the pitch (`HumanGamer::SetLockedPlayerID`). |

A new smoke test (`career_match` route, run in CI) plays a Player-mode fixture
in 3D from the fixture in `data/menu_smoke_career_match`. It checks that the
career squad is fielded, the controller is locked to the user's footballer, and
the result and appearances are recorded.

A further pass added:

| Area | Before | Now |
| --- | --- | --- |
| Inbox | The screen worked, but nothing posted to it. | Each fixture posts a match report (score, scorers, league position). The rollover posts a board review (each objective met or missed), a season review (finish, points, record, prize money) and contract news. Completed transfers also post. Newest messages are listed first; the inbox keeps 40, dropping read ones first. Text is translated in all five languages. |
| Front office | In Coach and Player careers nobody ran the club, so contracts expired and the squad shrank. | Before the rollover the AI front office renews expiring players worth keeping (always the user's own footballer). After it, it signs affordable free agents until the squad has 18 players. Each decision is reported in the inbox. Owner/GM careers are untouched. |
| Assists | `careerAssists` was never incremented. | About seven simulated goals in ten have an assist, weighted towards creative midfielders and wingers; the assister gets an appearance. |

The long-run audit test now checks that each season's finish equals the club's
place in the final table, instead of the old estimate.

---

## 7. Database

Scope: the bundled `data/databases/default/database.sqlite` (4 leagues, 8 clubs,
144 players), `utils/database.*`, and the code that reads and writes it: match
loading, League mode, Career mode and the Football Manager importer.

### Fixed

| Area | Defect | Fix |
| --- | --- | --- |
| Text in SQL | Every SQLite error is fatal (`Log(e_FatalError)` exits). Text pasted into SQL quit the game whenever it held a quote. That covered a manager name like "O'Neill" when starting a league or saving League settings, player names in the release/signing inbox messages and the player detail lookup, and club names in match-report messages. | `SqlString` (`utils/sqltext.hpp`) quotes and escapes text values; every site that pasted user or database text now uses it. |
| Season year | League settings saved `seasonyear = <text>`. The field allows only digits but can be empty, which gave a syntax error, so the game quit. | An empty year leaves the stored year unchanged. |
| Importer | The Football Manager importer wrapped text in double quotes. A `"` in a name broke the query, and SQLite reads a double-quoted word that matches a column name as that column, so a lookup for a club or league named like a column matched every row. | All text values use `SqlString`. |
| `Database` | `Query` used uninitialised row/column counts and a null table if the call failed or no database was open. `Load` kept SQLite's half-open handle after a failed open. | Both are initialised and checked; a failed open leaves no handle and `Query` refuses to run. |
| Player roles | `PlayerData` split Football Manager notation ("D C, DM") on spaces, so nearly every outfielder was labelled CM in the match HUD and line-up screens (e.g. "[CM] Ramos"). | The career parser moved to `data/dbroles.hpp` and both use it; a player gets each listed position, natural one first. |
| One-name players | Isco, Jesé, Marcelo, Pedro, Pepe and Rafinha had an empty surname. The HUD and goal messages show surnames, so they appeared blank. Four other one-name players already stored the name as the surname. | The data now stores all ten the same way, and `PlayerData` moves a lone first name into the surname for other databases. |
| Fatal errors | Any failed query quit the game, so a damaged League save took it down on the first read. | `Query` logs the error, keeps it in `GetLastError()` and returns no rows; callers already check for empty results. Loading a League save checks it opened and has settings, clubs and players. If not, the bundled database is reopened and a "This save could not be read" dialog is shown (all five languages). The `load_damaged_save` League smoke route, run in CI, tests this with a non-SQLite file. |
| Missing rows | `TeamData` and `PlayerData` read the first row without checking, so a missing player ID threw and ended the game. Formation slots the XML left out, or slots without a `position`/`role` field, read uninitialised or end-iterator values. | A missing player gets default stats and a placeholder name, with an error logged. A missing club ends with a message naming it, since no match can be built around it. Formations start from a 4-4-2 and only take the fields that are present. |
| League save files | Replacing `database.sqlite`/`autosave.sqlite` used the throwing `filesystem::remove`. | The error-code overload; a failure is reported by the copy that follows. |
| League generator | `SetupFourLeagues` (currently unused) gave players hairstyle "short" and skin colour 0, neither of which has an asset. | `short01` and skin 1-4. |

`tests/career/database_integrity_test.cpp` now checks the bundled file:
SQLite's integrity check, references between regions, countries, leagues, clubs
and players, an eleven with a goalkeeper first for every club, names, stat
ranges, profile stats, and that every logo and kit image exists.

### Known gaps

| Pri | Gap |
| --- | --- |
| P2 | **Size.** Two clubs per league; Career mode pads its tables with a default league. `tournaments` and `tournamentdata` are empty and unused. |
| P2 | **No schema version.** Saves and imports create tables with ad-hoc `CREATE TABLE` strings in several places; there is no version number or migration step. |
| P2 | **No indexes.** Lookups by `players.team_id` scan the table. That is fine at 144 players but not for an imported database with tens of thousands. |

---

## 8. Platforms

### Fixed

| Area | Problem | Fix |
|---|---|---|
| Void Linux | Never built: the package list was unverified, and a fresh install or container refuses `xbps-install` until xbps itself is updated. | `setup_linux_deps.sh` updates xbps first and installs `mesa-dri` (the GL drivers a minimal install lacks). CI builds and tests Void glibc and Void musl containers (`scripts/ci_void_linux.sh`): unit tests plus quick match, league, career menu and full-match smokes. |
| musl thread stacks | musl gives new threads 128 KiB of stack (glibc 8 MiB). Emulated on glibc, the game survives 64 KiB but crashes at 16 KiB, so the margin was small. | The executable asks for 8 MiB (`-z stack-size`), which musl honours and glibc ignores. All 214 game sources also compile against musl headers (checked with zig's musl target). |
| Smoke tests without xvfb-run | Void and Alpine ship `Xvfb` but not Debian's `xvfb-run`, so the smokes had no display. | `run_menu_smoke.sh` starts `Xvfb -displayfd` itself when needed. |
| macOS bash 3.2 | `./run.sh` with no arguments failed on stock macOS (`EXTRA_ARGS[@]: unbound variable`); `build.sh` and the smoke script had the same pattern. | Empty arrays expand with `${a[@]+"${a[@]}"}`; checked in a bash 3.2 container. |
| macOS dependencies | `setup_macos_deps.sh` installed `openal-soft`, whose llvm/z3 dependencies made CI fail after ~40 minutes. | Uses the system OpenAL.framework; CI now runs the same script. |
| macOS package | `package_macos.sh` copied the binary and assets but none of the Homebrew libraries, so the "standalone" folder needed Homebrew; `build.sh --package` ran the Linux packager on macOS. | Builds `Gameplay Football.app`: libraries bundled in `Contents/Frameworks` and relinked to `@rpath` (checked with `otool`), Info.plist, ad-hoc signature, optional zip. CI smoke-tests the packaged app and uploads it. |
| macOS saves | Inside an app the working directory (assets, config, saves) is read-only under App Translocation, and writing there breaks the signature. | The app runs from `~/Library/Application Support/Gameplay Football`, with the data linked from the bundle and the config copied once (unit-tested). |

### Known gaps

- No Void or macOS hardware run by hand yet; CI covers both under a virtual display or a headless runner.
- The macOS app is not notarised (needs an Apple Developer ID) and has no icon.
- League saves still tell the user to look in `<game directory>/saves`; in the macOS app that is the Application Support folder.

## Suggested order

1. **Substitutions and injuries:** one feature, and a prerequisite for Coach-mode
   bench decisions.
2. **Fixture generator and competitions:** a round-robin schedule, a knockout cup
   using `match_extra_time` plus a new penalty shootout, and applied promotion and
   relegation.
3. **Localization backfill:** the 292 missing keys per language, and moving
   hardcoded English into locale files.
