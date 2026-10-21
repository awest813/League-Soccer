# Gameplay and Career gap list

Date: 2026-10-21
Scope: the in-match game (`src/onthepitch`, `src/menu/ingame`) and Career mode
(`src/menu/career`, `src/data/careerdata.*`). The standalone League mode was not
audited. A second pass covering controls (input devices, remapping, menus and
in-match handling) is in [section 4](#4-controls).

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
| P1 | `match.cpp` `sig_OnGameOver` | Emitted every frame while the game is over (marked "todonow: just once"). It works only because `GamePage` deletes itself on the first call. | Changing it needs care with page lifetimes when another page is on top. |
| P2 | `gameplan.cpp` / `teamdata.cpp` `SwitchPlayers` | The in-match Lineup button (non-release builds) swaps `PlayerData` order but not live players, so names and stats get mismatched. | Hidden in release builds; replace with real substitutions. |
| P2 | `teamAIcontroller.cpp` set-piece positioning | The Set Piece Editor's goal-kick depth/width and corner depth are ignored. | Either apply them or remove the controls. |
| P2 | `match.cpp` collision trips / `Referee::TripNotice` | Standing-tackle trips (`tripType` 1) can never be fouls. | Referee tuning. |
| P2 | `humancontroller.cpp` shot direction | Keyboard shots force full auto-direction and override the shot-assist slider. | Possibly intended for digital input; needs a decision. |
| P2 | `team.cpp` kit fallback | The missing-kit fallback differs between `InitPlayers` (`almost_white`) and `SetKitNumber` (`white`). | Cosmetic. |

### Career

| Pri | Location | Defect |
| --- | --- | --- |
| P0 | `career_persistence.cpp`, `CareerDatabase::LoadCareerSlot` | Bids are saved but their transfer targets are not, so an accepted bid can never complete after Continue/Load. |
| P0 | `career_sim.cpp` `ApplyMatchResult`; `CareerDatabase::ConsumePlayedFixture` | `matchesPlayed` counts goals, not appearances. 3D-played fixtures pass no scorers, so they record no personal stats at all. |
| P0 | `career_sim.cpp` `GenerateLeagueStandings` | AI records are regenerated from a new seed each week, so points can go down. The default database has 2 clubs per league, so the table shows 2 clubs. Rank, prize money and board evaluation use `EstimateLeaguePosition`, which can return only 9 values. The form guide is invented. |
| P0 | `CareerDatabase::AdvanceSeason`, `career_board.cpp` | Board objectives are judged against targets recomputed from reputation *after* the season's changes, and regenerating objectives wipes the `completed` flags. |
| P0 | `career_persistence.cpp` | `leagueSettings` and `customLeague` are not saved. The custom-league Reset control also skips the `CanManageClub` check. |
| P0 | `CareerDatabase::CommitLifecycle` | Always writes slot 0, so loading slot 3 and playing overwrites the career in slot 0. |
| P1 | `career_finance.cpp` `InitializeOwnerData` | Player wages are weekly but counted once per season, while staff salaries are annual. |
| P1 | `career_sim.cpp` `UpdatePlayerValue` | Recalculating a player's value overwrites their negotiated or extended wage. |
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
| P1 | Stoppage time | **Absent.** A half ends the first time the ball is near the halfway line after 45'/90'; no added time is calculated or shown. |
| P1 | Knockout rules | Extra time is now opt-in (`match_extra_time`), but no menu exposes it and no competition sets it. **Penalty shootout absent:** the `Penalties` phase immediately ends the match. `onthepitch/matchclock.hpp` has the right model but only tests use it. |
| P1 | Set pieces | The taker is the nearest player. There are no designated takers, no aiming, power or curl control for free kicks, corners or penalties, and the kick-off is a forced short pass. |
| P1 | Injuries | **Stub** (see §2): no stretcher, no forced substitution, no carry-over into Career. |
| P2 | Goalkeeper control | Player switching skips the keeper. |
| P2 | Skill moves | Only the fake shot and knock-on exist; the sidestep dribble is disabled (`playercontroller.cpp`, "not implemented yet"). |
| P2 | Handball / VAR | Absent. |
| P2 | Weather | Physics only, driven by hidden config keys (`match_wind_x/y`, `match_wetness`). There is no menu option and no rain or wind visuals. |
| P2 | Kit clash | Home is always kit 1 and away kit 2, with no clash detection. |
| P2 | Statistics | No cards, corners, offsides, saves, scorer list or player ratings. The goal scorer is not stored after the match. |
| P2 | Replays | `sig_OnShortReplayMoment` is never emitted; there is no highlights reel. |
| P2 | Commentary | Absent; only crowd audio and on-screen messages. |

Present and working: advantage, offside with restart exemptions, cards
(including deferred cautions), the in-match tactics sliders, the stamina HUD, the
radar, the stats overlay, goal replays, celebrations, and the difficulty
multipliers for the CPU.

### Career: shared systems

| Pri | Feature | State |
| --- | --- | --- |
| P0 | Player-mode match control | `CareerMatchdayPage::PlayMatchFixture` gives the user the whole team. The controlled footballer is not mapped into the 3D match, and the Pro (database ID 99999) is not in the match squad. |
| P1 | Fixture list | No round-robin schedule: the opponent is `(week*3) % N` over every database team in every league, so opponents repeat about 5 times a season. Opponent strength is a hash of the club name. |
| P1 | Match squad | The 3D match uses the full database squad, not the career squad, so signings, releases and youth promotions never appear on the pitch. |
| P1 | Competitions | There are no cups or continental competitions ("Continental Cup" is only a message). Promotion and relegation are computed but never applied. |
| P1 | Transfer market | Bids resolve instantly. There are no transfer windows or deadlines, no outgoing sales or loans, and no AI clubs; "Agent Patience" is display-only and contract length is fixed at 3 years. |
| P1 | Contracts | No renewal negotiation, player refusals, release clauses or loans. Staff contracts expire silently. |
| P1 | Squad selection | `squad.startingXI` and `bench` are unused. The simulation averages the whole squad, and every squad member can score. |
| P1 | Injuries and assists | `InjuryStatus` is never set, and `careerAssists` is never incremented. |
| P1 | Inbox | The inbox screen works, but nothing ever adds a message to it. |
| P2 | Youth | 25 name combinations and lookups by name (duplicate names are likely). There is no intake schedule; `scoutingNetworkLevel` can't be raised, and `YouthAcademyState` and `ScoutingState` are unused. |
| P2 | Staff | Only Assistant Coach and Youth Coach have any effect. Clicking a staff row fires them without confirmation, and there is no hiring cap. |
| P2 | Finances | Processed only at season end. Stadium condition, fan satisfaction, season tickets and debt are decorative. The upgrade revenue bonus is ignored, and the stadium rename is fixed. |
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
| Player | P0 | No locked single-player control in 3D matches (above). |
| Player | P1 | Played matches record no personal goals or appearances, and there is no selection or bench status. A transfer request only sets a flag. |
| Player | P2 | No personal contract offers or renewals, and no personal objectives. |

### Localization

| Pri | Gap |
| --- | --- |
| P1 | `de`, `es`, `fr` and `pt` each lack 292 of the 775 English keys: 264 career, 21 gameplay settings, 6 match options, 1 settings. Career screens fall back to English in those languages. |
| P2 | Hardcoded English text: the career bid detail and matchday pages ("PLAY 3D MATCH", "FINAL SCORE", "[VICTORY!]"), standings ("of 38"), save-slot labels, form guide/news/opponent preview helpers, and every career event and board-objective string. Event and objective text is also stored in saves, so it cannot be re-translated on load. |

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

### Known defects not yet fixed

| Pri | Location | Defect | Why it was deferred |
| --- | --- | --- | --- |
| P1 | `humancontroller.cpp` dispatch, `humanoid.cpp` touch | Power is locked at the 500 ms dispatch, about 47% gauge, so a held pass/shot can never be stronger. The original comment assumed power could still grow before the touch. | Reading the gauge live at touch time makes held passes and shots noticeably stronger: a balance change that needs a playtest. The HUD is now honest about the locked value. |
| P1 | `humancontroller.cpp` Switch handling (Assisted) | Pressing LB before the action brakes the dribbler and fires an attacking run, so LB-then-X and X-then-LB differ. | Needs a design call on modifier timing. |
| P1 | `humancontroller.cpp` possession context | HighPass has no +0.15 weight while Sliding has −0.15, so B on a contested ball leans to a sliding tackle. The code's own TODO says the weights assume one layout. | Feel change; needs a playtest. |
| P2 | `usereventmanager.cpp` / HID `Process` | Button state is level-sampled every 10 ms; a tap that starts and ends between two samples is lost (e.g. at 30 fps rendering). | Needs per-button "pressed since last sample" latching. |
| P2 | `match.cpp`, `ball.cpp` rumble | Goal and woodwork rumble goes to every connected pad, including the opponent's and unassigned ones. | Needs the team-to-device mapping. |
| P2 | Fully-manual switching | Switching ignores the stick direction (`AI_GetBestSwitchTargetPlayer` never reads it). | Feature work. |
| P2 | Fully-manual switching | Actions can be armed during stoppages before a set piece. | Needs set-piece timing verified in-engine. |
| P2 | `HumanController::Reset` | Does not chain to `PlayerController::Reset`; harmless today. | Cleanup. |

### Feature gaps

| Pri | Gap |
| --- | --- |
| P1 | **SDL GameController API.** Devices are opened as raw joysticks, so layouts differ per platform and driver: PlayStation/Switch pads under SDL's HIDAPI driver get wrong defaults and no D-pad. `data/gamecontrollerdb.txt` is loaded but absent. Moving to `SDL_GameController` would replace the per-platform guesses fixed above. |
| P1 | **Stable device identity.** Bindings are keyed by `name #deviceIndex`, so a pad that enumerates in a different order loses or swaps its mapping. Key by GUID instead. |
| P1 | **Keyboard layouts.** Bindings are keycodes, not scancodes, so AZERTY/QWERTZ players get a scattered default cluster. |
| P1 | **Disconnect flow.** No pause or "reconnect controller" prompt when a pad drops mid-match, and Controller Select does not pick up pads plugged in after it opens. |
| P2 | **Remapping.** No conflict warning for duplicate bindings, no presets (PES/FIFA layouts), no secondary keys, no invert-axis, deadzone, trigger-threshold or rumble settings. The keyboard, gamepad mapping and function pages have hard-coded English text. |
| P2 | **On-screen control legend.** Button glyphs for Xbox/PlayStation/Switch exist (`HIDGamepad::GetButtonGlyphPath`) but nothing displays them. |
| P2 | **Special button.** Mapped (Z / LT) but not read anywhere in a match. |
| P2 | **Fake shot.** It is a ball-control cut at dribbling pace; the `FakeShot` modifier it sets is never read, so there is no dedicated feint animation. (`docs/gameplay.md` now describes it as it is.) |
| P2 | **Second local player on one team.** The power gauge only shows for the first human on a team. |

---

## Suggested order

1. **Career data integrity (P0):** bid targets, appearances, cumulative standings
   against a padded 20-club league, board-objective timing, saving league settings,
   and active-slot commits.
2. **Career squad in the 3D match, and Player-mode lock:** these two together make
   Player mode real and make transfers matter on the pitch.
3. **Substitutions and injuries:** one feature, and a prerequisite for Coach-mode
   bench decisions.
4. **Fixture generator and competitions:** a round-robin schedule, a knockout cup
   using `match_extra_time` plus a new penalty shootout, and applied promotion and
   relegation.
5. **Localization backfill:** the 292 missing keys per language, and moving
   hardcoded English into locale files.
