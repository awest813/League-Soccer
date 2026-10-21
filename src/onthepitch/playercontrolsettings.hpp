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
