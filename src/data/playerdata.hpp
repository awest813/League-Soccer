// written by bastiaan konings schuiling 2008 - 2015
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not
// be used for anything important. i do not offer support, so don't ask. to be used for inspiration
// :)

#ifndef _HPP_PLAYERDATA
#define _HPP_PLAYERDATA

#include "../gamedefines.hpp"
#include "../utils.hpp"
#include "base/properties.hpp"
#include "defines.hpp"

enum e_PlayerCondition {
  e_PlayerCondition_Terrible = 0,  // ⬇️ Purple (-9%)
  e_PlayerCondition_Poor = 1,      // ↘️ Blue (-4%)
  e_PlayerCondition_Normal = 2,    // ➡️ Yellow (0%)
  e_PlayerCondition_Good = 3,      // ↗️ Orange (+4%)
  e_PlayerCondition_Top = 4        // ⬆️ Red (+9%)
};

class PlayerData {
public:
  PlayerData(int playerDatabaseID);
  PlayerData();
  virtual ~PlayerData();

  std::string GetFirstName() const { return firstName; }
  std::string GetLastName() const { return lastName; }
  int GetDatabaseID() const { return databaseID; }
  const std::vector<e_PlayerRole>& GetRoles() const;
  std::string GetRoleName() const;

  float GetStat(const char* name);

  e_PlayerCondition GetCondition() const { return condition; }
  void SetCondition(e_PlayerCondition cond) { condition = cond; }
  void CalculateCondition(int matchSeed = 0);
  float GetConditionMultiplier() const;
  std::string GetConditionSymbol() const;
  Vector3 GetConditionColor() const;

  int GetSkinColor() { return skinColor; }
  std::string GetHairStyle() { return hairStyle; }
  std::string GetHairColor() { return hairColor; }
  float GetHeight() { return height; }

protected:
  int databaseID;
  std::string firstName;
  std::string lastName;
  std::vector<e_PlayerRole> roles;

  Properties stats;
  e_PlayerCondition condition;

  int skinColor;
  std::string hairStyle;
  std::string hairColor;
  float height;
};

#endif
