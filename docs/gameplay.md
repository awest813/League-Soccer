# Gameplay Design & Tuning

League Soccer's gameplay is specifically engineered and tuned to capture the golden era of football games, with **Pro Evolution Soccer 5 and 6** serving as the primary targets. 

The core philosophy is that football is a game of space, timing, momentum, and deliberate build-up, rather than an arcade-style sprint-fest.

## PES 5/6 Target Mechanics

The following parameters and systems have been audited, tuned, and polished to recreate the heavy, tactical, and rewarding feel of classic PES.

### 1. Player Movement & Speed (`humanspeed.hpp`, `velocitystate.hpp`)
* **Weight & Momentum:** Players feel appropriately heavy. Acceleration is deliberate, and stopping/turning takes time, especially at high speeds.
* **Speed Profile:** The difference between jogging and sprinting is highly pronounced. The sprint ceiling is capped (default ~7.0 m/s) to ensure elite pace is rewarding but doesn't completely negate defensive positioning.
* **Close Control:** The boundary between walking and dribbling is tightly tuned, making slow dribbling "sticky" and rewarding players who use left-stick control without holding the sprint button.

### 2. Ball Physics (`ball.cpp`, `ballphysics.hpp`)
* **Independent Entity:** The ball is an independent physics object, never magnetically "glued" to a player's feet.
* **Ground Friction & Bounce:** Linear friction (`1.38f`) is tuned so ground passes decelerate realistically, forcing receivers to come to the ball rather than waiting for it. The ball's restitution (bounce) is livelier, skipping off the turf like a real match ball.

### 3. First Touch & Fatigue (`gameplaytuning.hpp`)
* **Pressure Penalty:** Being closed down while receiving a fast-paced ball sharply increases the likelihood of a heavy first touch (error multiplier is `0.10` vs the standard `0.08`).
* **Condition Modifiers:** The classic PES 5-star condition arrows are represented mathematically. A player in terrible condition suffers compounding errors to composure, first touch, and accuracy.
* **Workload Fatigue:** Sprinting, especially while carrying the ball, burns stamina much faster than jogging. Late in the match, exhausted players will visibly struggle to track back or maintain top speed.

### 4. Advanced Controls (`humancontroller.cpp`, `humanoid_utils.cpp`)
All the classic PES inputs have been meticulously mapped and tuned:
* **Super Cancel:** Full 360-degree unassisted manual movement, allowing you to break off rails and fight for position.
* **1-2 Pass (L1 + Short Pass):** Forces the passer to immediately break forward dynamically.
* **Fake Shot:** Instantly triggers a sharp cut in the stick direction, completely dropping the player's momentum.
* **Chip Shot (L1 + Shoot):** Uses a classic scoop trajectory (desired height `0.42f`).
* **Finesse/Controlled Shot (R2 + Shoot):** Drops shot power significantly but cuts the error/scatter radius by 50% for high placement precision.

### 5. AI & Tactics (`elizacontroller.cpp`, `aitactics.hpp`, `teamAIcontroller.cpp`)
* **Disciplined Mid-Block:** The CPU holds a structured shape and primarily engages when the opponent enters the middle third, rather than frantically pressing everywhere.
* **Run Selection:** The AI is highly selective about forward runs (threshold `0.62`), preferring to maintain the offensive shape and rely on combination play (`0.55`) over reckless solo dribbles.
* **Zone Pressure:** When possession is lost, up to three players will coordinate to corral the ball-carrier, recreating the famous PES 5 defensive intensity.
* **Goalkeeper Positioning:** Keepers will hold their line during deep possession, only rushing out when they have a calculated positional advantage.

### 6. Refereeing & Fouls (`referee.cpp`)
* **Advantage Rule:** The referee gives the attacking team a strict 3.5-second window to realize an advantage before pulling play back for a foul.
* **Sliding Tackles:** The collision detection threshold is slightly raised so that marginal, ball-first sliding tackles aren't instantly penalized, bringing back the physicality of the mid-2000s era.

## Gameplay audit — September 2026

This pass focused on input conflicts and referee timing, preserving the existing
movement speeds, ball physics, and tactical balance.

- Super Cancel (Sprint + Dribble) suppresses automatic ball control, trapping,
  tackling, pass requests, and attacking-run requests. It also bypasses the brief
  movement assistance after switching players. Previously, the same combination
  queued a knock-on and automatic touches ahead of manual movement.
- Knock-ons use two separate sprint presses less than 280 ms apart. The first
  press cannot trigger one at kickoff. A queued knock-on expires after 280 ms;
  stoppages, set pieces, Super Cancel, and player switching clear its history.
- Advantage uses elapsed time since the foul, preventing unsigned underflow near
  kickoff. Possession loss recalls the foul after the 600 ms grace period and
  through the 3.5-second window; an expired advantage clears its pending state.

Regression tests cover sprint input sequences and advantage timing. The existing
scripted match checks match completion and basic human-controller integration;
its short-pass-only driver does not validate Super Cancel feel or controller
combination inputs. Those still need a hands-on playtest.

The referee follow-up below resolves the previously identified lost-caution issue.

Validation: Windows Release build succeeded; all 249 automated tests passed,
including nine new regressions. The scripted human-controller match completed
both halves and reached the results screen (Arsenal 0–1 Manchester United),
with no logged warnings/errors and empty standard error output.

### Referee follow-up

- Yellow cards from an advantage are preserved independently of the current foul.
  Goals, an expired advantage, and later offences no longer erase them. Deferred
  cautions are issued once at the next stoppage, before the restart; second
  cautions stop play, including when the earlier caution is still pending.
- Goal-line decisions cannot be overwritten by the touchline check in the same
  update. Once a restart or period break stops play, later boundary checks skip it.
- Direct goal kicks and corners join throw-ins as offside-exempt restarts. Every
  restart clears stale offside candidates. Normal offside tracking resumes after
  the direct reception.
- The foul victim pointer is initialized before any referee checks.

Rules references: [IFAB Law 12, advantage and disciplinary action](https://www.theifab.com/laws/latest/fouls-and-misconduct/)
and [IFAB Law 11, direct-restart exemptions](https://www.theifab.com/laws/latest/offside/).
Deferred cautions update the player's card count and show a notification; this
pass does not add a separate deferred-card referee animation.

Follow-up validation: Windows Release build succeeded; all 252 selected tests
passed, including six new caution/restart regressions. The three career long-run
audit tests were excluded from this referee-only rerun. A fresh scripted match
completed both halves and reached the results screen with no logged warnings or
errors and empty standard error output. This smoke test verifies match flow,
not a forced in-engine replay of each disciplinary/offside scenario.

### Passing and shooting follow-up

- Late shot aim was calculated and discarded. The bounded correction now updates
  the command read by shot physics, including difficulty and spin calculations.
  The existing 0.1*pi (18-degree) commitment limit remains unchanged.
- One-two runs begin when a short-pass animation actually contacts the ball,
  rather than while a pass is merely queued. A cancelled or missed pass therefore
  does not launch its runner. Holding the run/switch modifier no longer repeatedly
  retargets the run; standalone run requests use a fresh button press.
- Chip takes precedence over finesse throughout trajectory, accuracy and spin
  calculation when both modifiers are present. Normal individual chip and finesse
  tuning is unchanged.

Validation: Windows Release build and 258 selected tests passed, including six
new aim/style regressions. The three career long-run audit tests were excluded
from this action-only rerun. Unit tests cover bounded aim corrections in both
directions, angle wraparound, and modifier precedence. One-two timing and the
feel of late shot aiming still require a hands-on combination-input playtest.

The updated build also completed a fresh scripted human-controller match through
both halves and the results screen, with no logged warnings/errors and empty
standard error output. The short-pass-only driver validates general match flow,
not the one-two or shot-modifier combinations themselves.

### Teammate AI audit

- Automatic run decisions now examine space ahead of the runner toward the
  opponent's goal, using mirrored coordinates for the two teams. Previously the
  density check sampled ten metres toward the runner's own goal.
- Automatic run selection excludes goalkeepers, human-controlled players,
  inactive players, and the ball carrier. The evaluated candidate is the same
  player assigned the run; a missing carrier produces no run candidate.
- Defender support now honors a selected overlap/one-two run when attacking
  possession bias is strong. Existing defender positioning and support weights
  remain in force.
- Secondary pressure retains its current eligible defender unless a replacement
  improves the distance-plus-role score by more than 1.5 metres. Assignments still
  change if the current defender becomes the primary defender, human-controlled,
  inactive, or otherwise ineligible.
- Support positioning falls back to formation position if no carrier is available.
  Pass requests are no longer discarded by an unrelated caller; existing caller
  guards already prevent that case during normal play, so this is defensive hardening.

Validation: Release build and all 263 selected tests passed, including five new
regressions for mirrored run probes, runner eligibility, and pressure continuity.
The three career long-run audit tests were excluded. These tests exercise decision
helpers; they do not establish improved win rates or replace hands-on evaluation
of teammate spacing, timing, and difficulty balance.

The scripted human-controller match completed both halves and reached its results
screen with no logged warnings/errors and empty standard error output. This checks
general match flow with the updated AI, not comparative tactical effectiveness.

## October 2026 audit

Extra time is now opt-in: a match level after 90 minutes ends unless the
`match_extra_time` config key is set. League, career and Quick Match fixtures
therefore finish as draws. There is still no penalty shootout. The gameplay
fixes, remaining defects and missing features are tracked in
[gap-list.md](gap-list.md).
