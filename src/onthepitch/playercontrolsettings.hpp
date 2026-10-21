// Player-control configuration shared by the menu and match controllers.

#ifndef _HPP_FOOTBALL_ONTHEPITCH_PLAYERCONTROLSETTINGS
#define _HPP_FOOTBALL_ONTHEPITCH_PLAYERCONTROLSETTINGS

#include <algorithm>

enum class PlayerSwitchMode { Assisted = 0, FullyManual = 1 };

inline constexpr const char* kPlayerSwitchModeConfigKey = "gameplay_player_switch_mode";
inline constexpr unsigned long kPassRequestDuration_ms = 1200;

inline PlayerSwitchMode PlayerSwitchModeFromSlider(float sliderValue) {
  return std::clamp(sliderValue, 0.0f, 1.0f) >= 0.5f ? PlayerSwitchMode::FullyManual
                                                     : PlayerSwitchMode::Assisted;
}

inline float PlayerSwitchModeToSlider(PlayerSwitchMode mode) {
  return mode == PlayerSwitchMode::FullyManual ? 1.0f : 0.0f;
}

inline bool AllowsAutomaticPlayerSelection(PlayerSwitchMode mode) {
  return mode == PlayerSwitchMode::Assisted;
}

inline bool ShouldCallForPass(PlayerSwitchMode mode, bool isInPlay, bool isSetPiece,
                              bool selectedPlayerIsPossessionPlayer, bool teamHasPossession,
                              bool shortPassDown, bool shortPassWasDown) {
  return mode == PlayerSwitchMode::FullyManual && isInPlay && !isSetPiece &&
         !selectedPlayerIsPossessionPlayer && teamHasPossession && shortPassDown &&
         !shortPassWasDown;
}

// Whether a fresh Switch press should move control to another player.
// assistedSwitchAllowed carries Assisted mode's extra possession checks. In
// every mode the player dribbling the ball keeps control: Switch doubles as the
// chip / one-two modifier, and switching would hand the ball to the AI.
inline bool ShouldSwitchOnPress(PlayerSwitchMode mode, bool selectedHasUniquePossession,
                                bool assistedSwitchAllowed) {
  if (selectedHasUniquePossession)
    return false;
  return mode == PlayerSwitchMode::FullyManual || assistedSwitchAllowed;
}

// Switch (LB / L1) doubles as a modifier (chip shot, one-two) and, with the
// ball, as "send a teammate on a run" and "hold to stop". Deciding those on
// press made LB-then-action differ from action-then-LB: pressing LB first
// braked the dribbler and fired a run. Instead, a run is requested on release
// only if no action was armed while LB was held, and braking needs LB held on
// its own for a moment.
inline constexpr unsigned long kSwitchBrakeDelay_ms = 200;

struct SwitchModifierTracker {
  // Returns true when a teammate run should be requested (LB released after
  // being used on its own).
  bool Update(bool down, bool wasDown, bool actionArmed, unsigned long now_ms) {
    if (down && !wasDown) {
      usedAsModifier = actionArmed;
      pressedAt_ms = now_ms;
    }
    if (down && actionArmed)
      usedAsModifier = true;
    if (!down && wasDown) {
      const bool requestRun = !usedAsModifier;
      usedAsModifier = false;
      return requestRun;
    }
    return false;
  }

  bool ShouldBrake(bool down, bool actionArmed, unsigned long now_ms) const {
    return down && !actionArmed && !usedAsModifier && now_ms >= pressedAt_ms &&
           now_ms - pressedAt_ms >= kSwitchBrakeDelay_ms;
  }

  void Reset() {
    usedAsModifier = false;
    pressedAt_ms = 0;
  }

  bool usedAsModifier = false;
  unsigned long pressedAt_ms = 0;
};

inline unsigned long PassRequestExpiresAt(unsigned long requestedAt_ms) {
  return requestedAt_ms + kPassRequestDuration_ms;
}

inline bool IsPassRequestExpired(unsigned long currentTime_ms, unsigned long expiresAt_ms) {
  return currentTime_ms > expiresAt_ms;
}

template <typename Configuration>
PlayerSwitchMode ReadConfiguredPlayerSwitchMode(const Configuration& configuration) {
  return PlayerSwitchModeFromSlider(configuration.GetReal(kPlayerSwitchModeConfigKey, 0.0f));
}

template <typename Configuration>
bool UsesFullyManualPlayerSwitching(const Configuration& configuration) {
  return ReadConfiguredPlayerSwitchMode(configuration) == PlayerSwitchMode::FullyManual;
}

#endif  // _HPP_FOOTBALL_ONTHEPITCH_PLAYERCONTROLSETTINGS
