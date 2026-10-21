# Career consolidation audit and implementation plan

Date: 2026-09-17
Status: Three-role foundation, shared lifecycle, and shared career hub implemented. Deeper Owner/GM and Coach progression, Player identity repair/match integration, and release playtesting remain.

## Intended outcome

Offer exactly three career choices: **Owner/GM**, **Coach**, and **Player**. All three participate in the same football world, calendar, match results, development rules, and save system. Each has a distinct set of decisions, objectives, and home-screen priorities.

The audit below records the implementation before consolidation. The progress section describes subsequent changes and verification.

## Current implementation and findings

| Priority | Finding and evidence | Consequence / proposed response |
| --- | --- | --- |
| High | `src/data/careerdata.hpp` defines PLAYER, MANAGER, GM, COACH, OWNER. `careerpages.cpp` exposes five creation routes; `career_database.cpp:170` interprets string aliases and defaults to Manager. | Consolidate the domain model, creation routes, labels, summaries, and defaults together. Removing menu buttons alone leaves five underlying modes. |
| High | `careerpages.cpp:647` disables different buttons in a shared management hub. Owner has its own hub in `ownerpages.cpp`, including tactics, training, transfers, and infrastructure. | Role boundaries and navigation are distributed across pages. Build one capability policy and a shared hub shell with role-specific content. |
| High | `careerpages.cpp:504` creates an 18-year-old CF with database ID 99999. `PlayMatchFixture` at line 2636 launches team control without passing `controlledEntityID`. The inspected source references to that ID concern creation, training, player detail, and persistence. | Player identity exists in the save, but a single-player match-control connection is not established by this launch path. Implement and verify roster-to-match identity mapping and player lock before claiming a complete Player experience. |
| High | `careerpages.cpp:1554` exposes the same training-plan controls to every role; `career_training.cpp:13` applies that plan across the senior roster. Player only gets a separate immediate Individual session button. | Player currently has influence over whole-squad development. Separate personal training choices from team training, with team decisions handled by the coach. |
| High | `career_persistence.cpp:21,134,475,626,643` stores/reads mode integers using enum casts in payloads and summaries. | Reordering the enum can silently reinterpret saves. Introduce version-aware explicit decoding and migration for both SQLite and legacy text saves, including save-list summaries. |
| Medium | `CareerSeasonPage::AdvanceSeason` at `careerpages.cpp:2294` adds finance processing and board/sponsor work only for Owner. Calendar progression is split between simulated-match Back navigation (`GoBack`) and `CareerSim::Process3DMatchResult`. | Move match completion and season closing into shared services; role determines decisions and presentation, not whether world finances operate. Verify exactly-once advancement and rewards. |
| Medium | `career_database.cpp:232` forwards recruitment without a role check; `career_transfers.cpp:17` also has no role check. Similar restrictions are currently visible in menu code. | Enforce user-action permissions at the command boundary, while allowing explicitly attributed AI decisions to keep the club operating. Hiding buttons is insufficient. |
| Medium | `CareerSave` contains overlapping roster/squad, finance, board, and season fields. Initialization in `career_database.cpp` synchronizes some copies manually. | Document a canonical source for each value and replace duplicate write paths incrementally. Do not combine an unbounded data rewrite with the first menu change. |
| Medium | Ongoing progression caps OVR at potential, but Individual sessions increase OVR toward 99 directly; Coach focus sessions can improve players above potential (`career_training.cpp:98,124`). | Use one documented development rule across roles; role identity should come from decisions and outcomes. Rebalance deliberate bonuses with regression coverage. |
| Medium | Matchday presentation contains fixed atmosphere/pass-accuracy values and derived awards (`careerpages.cpp`, `PopulateGrid`). | Use actual results and player participation for progression and reporting. Do not derive Player achievements from decorative match summaries. |
| Scope | Main menu also exposes League. `src/menu/league/league.cpp` uses its own save-directory/database flow. `CareerSaveRegistry::Load/Save` in `src/data/careerdata.cpp:8` are explicitly no-op stubs, distinct from working career persistence. | Keep standalone League outside the three career choices. Reuse useful league UI through adapters after checking data contracts; do not assume save compatibility or route active saves through the registry stubs. |

## Three role contracts

| Area | Owner/GM | Coach | Player |
| --- | --- | --- | --- |
| Main responsibility | Build and operate the club | Prepare and lead the team | Develop and perform as one footballer |
| Recruitment and contracts | Final transfer, wage, contract, scouting, and academy spending decisions | Request targets, positions, and promotions; front office decides spending | Respond to personal offers and request a move; no club transactions |
| Team preparation | Delegate to hired coach by default; optional hands-on responsibilities | Formation, lineup, substitutions, tactics, squad training, youth integration | Personal training, recovery, position preferences; AI coach selects team |
| Business | Staff, facilities, stadium, sponsors, budgets, tickets | Request resources; view relevant limits | View personal contract and career information |
| Matchday | Simulate/watch; optional team control under hands-on settings | Tactical control, substitutions, simulate/watch; optional team control | Play locked to own footballer when selected; simulate bench/injury periods |
| Progression | Club value, financial health, supporter confidence, trophies | Results, tactical objectives, player development, job reputation | Actual minutes, performance, development, coach trust, contracts |
| Role-specific home | Club overview and pending business decisions | Team readiness and upcoming opponent | My Pro, selection status, next appearance, personal goals |

Owner/GM is one mode. Delegation settings preserve the former Manager hands-on experience without introducing another career choice. Watch/tactical-only match control and personal contract workflows are proposed capabilities; their runtime support must be verified or implemented.

Custom league and expansion settings belong to world setup/rules, not ordinary Player or Coach authority. Existing settings were intended to remain preserved in saves; as of 2026-10-21 `leagueSettings` and `customLeague` are not serialized (see [gap-list.md](gap-list.md)).

## Cohesive user journey

Use one entry point: Career -> New Career / Continue / Load. New Career presents the three role cards, then shared world/club setup and role-specific identity options. Player must select a position and an existing or created footballer with a stable identity.

Every hub uses the same header (career identity, club, season/week, next fixture), save status, inbox conventions, and primary Continue action. Shared destinations are Home, Calendar, Team, Competitions, and Career/Save; role-specific destinations are Club Office, Coaching, or My Pro. Only show actions the role can perform. Shared read-only information remains accessible where useful.

Continue resolves required decisions, opens the next fixture, applies its result once, presents relevant consequences, and advances the calendar. Season review and season transition use this same shared lifecycle. AI handles responsibilities outside the user's role so transfers, finances, lineups, and development continue in all three modes.

## Save migration decisions

- Legacy numeric values are PLAYER=0, MANAGER=1, GM=2, COACH=3, OWNER=4. Decode them explicitly; never cast them into a reordered three-value enum.
- Map PLAYER to Player and COACH to Coach. Map MANAGER, GM, and OWNER to Owner/GM.
- Preserve Manager saves with hands-on responsibilities; preserve GM delegation preferences and Owner infrastructure/business state. Do not reset budgets, squads, facilities, history, or the calendar to new-game defaults.
- Version new saves and serialize stable role identifiers. Apply identical decoding to full loads and lightweight summaries. Keep compatibility aliases only at old-route/input boundaries.
- Validate the controlled player's identity and references. Repair a legacy collision through an explicit ID mapping; an unresolved identity must produce a recoverable selection flow, not silently control somebody else.
- Keep an original backup before writing a migrated save. Reject unsupported/corrupt role data with a useful message instead of silently choosing a mode.

## Delivery sequence

1. **Foundation and compatibility.** Introduce the three-role model, legacy decoder, capabilities, delegation settings, and canonical hub resolver. Update persistence and summaries. Add fixtures for all five legacy roles in both supported formats. Completion: old saves retain state and reopen in the correct new role.
2. **Shared lifecycle and action boundaries.** Centralize match completion, calendar advancement, season closing, and role-authorized commands. Give AI an explicit action context. Inventory duplicate state and define authoritative values. Completion: played/simulated matches and reloads apply results, growth, finances, and rewards once; forbidden user commands cannot mutate saves.
3. **Three-choice navigation.** Replace five creation buttons, unify hub framing/back routes, and organize shared pages by role. Update `careerpages`, `ownerpages`, save and standings pages, `menutask`, all locale files, README, and smoke routes. Completion: only three career labels appear in ordinary flows and every Back/Continue/Load route returns to the correct context.
4. **Owner/GM and Coach completion.** Merge Owner business tools with GM recruitment, implement delegation and coach resource requests, and align objectives. Completion: both careers can finish a season with meaningful decisions and automatic handling of delegated responsibilities.
5. **Player completion.** Implement stable player creation/selection, match roster integration, player lock, bench/substitution/injury handling, personal development and objectives, and personal contract/move decisions. Completion: a played appearance controls the selected footballer, records real personal outcomes, persists them, and exposes no squad/business mutation controls.
6. **Balance, migration rehearsal, and release.** Run the career regression target plus menu/layout checks, then keyboard/controller and 3D match playtests for all three modes. Update documentation to match delivered behavior. Completion: migration fixtures pass, all modes survive repeated seasons, and no obsolete choice remains outside compatibility code/tests.

Steps 1 and 2 precede the visible consolidation. Step 3 supplies the common interface for steps 4 and 5. Player match integration is the largest uncertainty and should receive an early technical spike during step 1; do not represent its work as a simple rename.

## Verification checklist

- Extend `gameplayfootball_career_tests`: legacy role decoding and round trips; save summaries; permission enforcement; controlled-player identity; personal versus squad training; potential caps; delegated decisions; exactly-once match and season processing.
- Extend `tests/run_menu_layout_audit.py` and menu smoke coverage for three-card creation, role-specific hubs, load/back routing, long lists, and all supported locale labels.
- Run existing fixture, simulation, save/load, module, and twelve-season balance audits. Add equivalent long-run coverage for each role with AI delegation enabled.
- Manually verify home/away played and simulated fixtures, quitting/reloading around result screens, season rollover, player bench entry/substitution/injury, and keyboard/controller focus.
- Migration acceptance: compare pre/post budget, roster IDs, contracts, facilities, history, fixture results, current week, and controlled-player identity, not merely whether a save loads.

## Scope guardrails

No multiplayer careers, new competition engine, or wholesale League-save migration is required for this consolidation. Reuse existing career modules and replace duplicate presentation and lifecycle paths incrementally. Broader features such as job-market depth or off-field Player stories can follow after each role has a complete season loop.

## Implementation progress: first delivery

Implemented:

- Exactly three runtime roles and three new-career choices, with labels in all five locales.
- Shared role labels and hub resolver for continue, load, standings, season return, and match return.
- Stable role strings in version 2 saves; explicit migration of all five legacy values in text and SQLite saves and save previews. Unsupported roles/versions are rejected.
- A persistent `.pre-three-modes.bak` copy before overwriting a legacy save, alongside normal rotating backups.
- Owner/GM team-control setting; legacy Manager/Owner retain hands-on access, legacy GM retains delegation. Delegation currently uses existing simulation behavior; a new AI decision system is still pending.
- Initial capability helpers and training-command restrictions so Player cannot change the shared squad training plan or request squad training. Full command authorization is still pending.
- Regression fixtures for all legacy role/format combinations, repeated migration saves, invalid roles/versions, role creation, and responsibility persistence. Added role hub layout routes.

Remaining before the full plan is complete: a shared hub shell with role-specific content, unified fixture/season lifecycle, complete command authorization and AI responsibility handling, canonical-state cleanup, Player identity repair/match control and personal career workflow, and broader balance/manual match validation. This delivery does not establish that every historical save field was already persisted correctly by the old serializer.

Verification for this delivery: Release game and career-test builds succeeded; all 105 career tests passed, including the twelve-season audit and new migration cases. All seven English career layout routes passed at 1280x720: selection, new career, save/load, training, Owner/GM hub, Coach hub, and Player hub. No manual 3D match or physical-controller playtest was performed.

## Implementation progress: shared lifecycle and command boundaries

Implemented in the second delivery:

- Played and simulated results use `CareerDatabase::CompleteFixture`, identified by season and week. The operation records scores in home/away order, applies user-relative results and development, advances the calendar, and commits the primary save together. Repeated and stale results are rejected, including after reload.
- Leaving matchday no longer advances the week. Both hubs route completed seasons to season review. Season closing is guarded against early/repeated execution and processes finances, history, rewards, contracts, upgrades, objectives, and new sponsor offers for every role.
- Lifecycle operations suppress intermediate event-driven saves. A failed primary write restores the in-memory save, bids, targets, and career RNG. Save replacement uses same-directory rename without an in-place copy fallback. Played-match save failure keeps the result screen available for retry.
- Explicit User/AI action sources guard recruitment, club spending, staff, sponsors, squad motivation, drills, and tactics. Contract extensions and transfer requests now use checked commands; Player can request a move only for the controlled footballer. World-setup page mutations also check club authority.
- Delegated team training selects Recovery below 70 average fitness and Balanced otherwise through an explicit AI action. Broader recruitment/lineup AI remains a later role-completion task.
- Saves now include season timing flags, season-scoped fixtures, finance reports, stadium projects and revenue, sponsor offers, and player identity/contract fields. Missing appended fields retain legacy defaults. Old saves caught between a simulated result and the former Back action advance to the correct next week on full load.
- Added regressions for duplicate/reloaded results, away scores, early/repeated season closing, failed writes, every role's season loop, infrastructure completion, personal requests, and delegated training. Added season and matchday layout routes; corrected matchday action-column overflow.

### Authoritative state for subsequent work

| Domain | Authority used by active career services | Existing duplicate / remaining work |
| --- | --- | --- |
| Calendar | `CareerSave::season.currentSeason/currentWeek/maxWeeks` | `currentSeason` mirrors the nested season on load/rollover. All menu advances now use the lifecycle service. |
| Results | Current-season `season.fixtures` with season/week identity; `history` contains closed seasons | `seasonWins/Draws/Losses/Goals` remain cached totals maintained by result application. Legacy unscoped fixtures are preserved until rollover. |
| Players | `roster`, `freeAgents`, `youthAcademy` | `squad.roster` and nested youth copies are not the active mutation targets. Replacing these duplicate structures remains separate cleanup. |
| Budget | Top-level `transferBudget` and `wageBudget` | `finance` mirrors these on load and season closing; `finances` holds business income, expense, and net-worth reports. Monetary-unit cleanup is not included here. |
| Board/reputation | Top-level `boardConfidence` and `reputation` | Existing event/load paths synchronize `board.confidence` and `club.reputation`. Mode-specific objective design remains for role completion. |
| Identity/contracts | Roster database ID plus `controlledEntityID`; roster contract fields | These now persist. Repairing identities already lost by older serializers and mapping the controlled player into the 3D match remain Player-phase work. |

Scope note: low-level simulation modules remain available for isolated tests/tools. Ordinary career result and season commands now use the transactional service; raw mutable save access is still part of the existing architecture. A shared hub redesign and deeper role-specific progression are still pending.

Second-delivery verification: Release game and career tests built successfully. All 111 tests passed, including the twelve-season simulations and new lifecycle, failed-save, legacy-calendar, delegated-training, and player-contract persistence cases. All nine English career layout routes passed at 1280x720, including season review and matchday. No manual 3D match or physical-controller playtest was performed. The next planned phase is the shared career hub shell and role-specific navigation/content.

## Implementation progress: shared career hub

Implemented in the third delivery:

- One shared hub replaces the separate management and Owner dashboards. Continue, Overview, Team, the role workspace, Competitions, Inbox, Save/Load, and Exit use a consistent sidebar. The canonical destination is Career Hub; the old Owner Hub page ID remains a compatibility alias.
- Club Office combines business tools, recruitment through Team, and the hands-on/delegated responsibility setting. Coaching exposes squad preparation and press conferences. My Pro shows the uniquely identified footballer's development, fitness, career goals/assists, contract years, wage, and transfer request.
- Tool availability is computed from role and responsibilities and checked again before dispatch. Inaccessible actions are omitted. Shared competition results and squad inspection remain available to all three roles.
- Overview uses saved calendar, squad, club, personal, and unread-message state. It routes completed seasons to review and does not fabricate a future opponent or preview score.
- Inbox supports scrolling, opening full messages, and saving read status. Player training now shows only the controlled footballer and an individual session; missing or ambiguous identities expose an explanation instead of personal actions.
- Added hub labels in all five locales and automated navigation coverage for sidebar movement, cross-panel movement, action scrolling, message opening, and SQLite read-status persistence. Fixtures include long names, long message subjects, twelve messages, legacy Owner routing, delegated GM migration, and missing Player identity.

The next planned phase is **Owner/GM and Coach completion**: deeper delegation, coach resource requests, and aligned objectives. Player identity repair, dedicated match control, and personal objectives remain in the Player phase. This delivery changes navigation and presentation; it does not claim those progression systems are complete.

Third-delivery verification: Release game and career-test builds succeeded. All 113 career tests passed; the two hub-policy tests were rerun successfully after retaining press-conference navigation. All 75 role/section/locale combinations passed at 1280x720 (three roles, five hub sections, five languages), including automated directional focus, scrolling, message opening, and persisted read status. Ten additional English routes passed for career selection, creation, save/load, squad training, legacy Owner hub, delegated Owner/GM, missing Player identity, personal training, season review, and matchday. All hub translation keys are present in every locale, and `git diff --check` passed. No manual 3D match or physical-controller playtest was performed.

To repeat a section check, run `python tests/run_menu_layout_audit.py build-win/Release/gameplayfootball.exe --routes career_owner_gm career_coach career_player --hub-section 2 --language en`. Sections are 0 Overview, 1 Team, 2 role workspace, 3 Competitions, and 4 Inbox.

## Shared hub audit and polish

The follow-up audit found and fixed three focus problems: changing sections returned focus to Continue, closing a message rebuilt the hub and discarded the selected message/scroll position, and personal training initially focused Back even when a session was available. Section changes now focus the selected tab; message dismissal restores the same visible message; personal training focuses an available session and otherwise Back.

Inbox read status now updates its row indicator and unread count in place after a successful save. A failed write retains unread status and displays a localized retry explanation. Hub tabs are inactive when there is no active save. Automated hub checks now exercise message dismissal at both ends of a twelve-message list and verify restored focus and read indicators.

Polish verification: Release game build passed. All 32 focused menu checks passed: 15 inbox role/language combinations at 960x540, 12 non-inbox role/section combinations at 1280x720, and five legacy/delegated/missing-identity/training routes. Checks cover tab focus, first/last message opening and dismissal, restored row visibility, read indicators, SQLite persistence, and personal-training focus. `git diff --check` passed. This was automated UI validation; no physical-controller or 3D match playtest was performed.

## Career submenu UI audit

The next UI pass covers tactics, press conferences, squad browsing, and player profiles. Tactical choices now use translated labels, focus the current selection, and become read-only when team preparation is delegated. Press screens use wrapped questions and shorter localized responses without claiming unsupported tactical effects or team-spirit rewards.

Squad rows are taller and display a concise name/position/overall/potential/fitness summary, with scrolling through the entire roster. The generic profile hint applies to all roles. Profiles use wrapped saved statistics, including goals, assists, contract, age, value, morale, and development; invented positional attribute estimates were removed. Profile actions are localized and follow team/club responsibilities, including hands-on Owner/GM preparation actions. Escape closes a profile and restores its selected roster row.

New smoke routes cover tactics, delegated tactics, press, Owner/GM roster, and Player roster. Roster fixtures contain 24 long names; checks traverse every row, open the final profile, audit its bounds, and close with Escape while verifying focus restoration.

Submenu verification: final Release game build passed, as did all 25 targeted screen checks across five languages at 960x540. These include tactical focus/delegation, complete roster scrolling, profile bounds, Escape dismissal, and restored row focus. `git diff --check` passed. Physical-controller and 3D match playtesting were not performed in this UI pass.

## Regression bug sweep

The full career run exposed an outdated localization test: Spanish now translates the hub title, so expecting the English fallback was incorrect. The assertion now checks the Spanish title and separately verifies fallback using an untranslated key.

Two additional UI defects were corrected: cancelling a roster release confirmation did not restore focus, and season review treated the final unplayed week as ready while its advance command remained blocked. Cancellation now restores the selected roster row without removing a player. Season guidance uses the same completion guard as the advance command and tells the user to finish all fixtures. Added a final-week fixture and release-cancellation smoke coverage.

Bug-sweep verification: Release game and career tests built successfully; all 113 career tests passed on the final rerun, including the twelve-season audit. All 35 existing menu routes passed, followed by 10 new release-cancellation/final-week checks across five languages. `git diff --check` passed. Automated coverage does not replace a physical-controller or 3D played-match test.
