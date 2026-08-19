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
