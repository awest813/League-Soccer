#pragma once

#include <string>

// Keep historic numeric values explicit; disk input is decoded, never enum-cast.
enum class CareerMode { PLAYER = 0, COACH = 3, OWNER_GM = 4 };

inline const char* CareerModeKey(CareerMode mode) {
  switch (mode) {
    case CareerMode::PLAYER:
      return "player";
    case CareerMode::COACH:
      return "coach";
    case CareerMode::OWNER_GM:
      return "owner_gm";
  }
  return "";
}

inline const char* CareerModeLabelKey(CareerMode mode) {
  switch (mode) {
    case CareerMode::PLAYER:
      return "career_mode_player";
    case CareerMode::COACH:
      return "career_mode_coach";
    case CareerMode::OWNER_GM:
      return "career_mode_owner_gm";
  }
  return "career_mode_default";
}

// Legacy Manager and Owner retain hands-on control; legacy GM delegates it.
// The output parameters are unchanged for an unsupported role.
inline bool DecodeCareerRole(const std::string& value, CareerMode& mode, bool& handsOn,
                             bool allowLegacy = false) {
  if (value == "player" || (allowLegacy && value == "0")) {
    mode = CareerMode::PLAYER;
    handsOn = false;
  } else if (value == "coach" || (allowLegacy && (value == "3" || value == "mycoach"))) {
    mode = CareerMode::COACH;
    handsOn = true;
  } else if (value == "owner_gm" || (allowLegacy && (value == "1" || value == "4" ||
                                                     value == "manager" || value == "owner"))) {
    mode = CareerMode::OWNER_GM;
    handsOn = true;
  } else if (allowLegacy && (value == "2" || value == "mygm")) {
    mode = CareerMode::OWNER_GM;
    handsOn = false;
  } else {
    return false;
  }
  return true;
}
