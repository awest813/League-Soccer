#include "playerdata.hpp"

#include <cmath>

#include "../main.hpp"
#include "base/utils.hpp"
#include "dbroles.hpp"
#include "utils/database.hpp"

PlayerData::PlayerData(int playerDatabaseID) : PlayerData() {
  // The default constructor provides every stat, so a profile missing some
  // (or a missing row) cannot trip GetStat's assert.
  databaseID = playerDatabaseID;

  auto result = GetDB()->Query(
      "select firstname, lastname, role, base_stat, profile_xml, age, skincolor, hairstyle, "
      "haircolor, height from players where id = " +
      int_to_str(databaseID) + " limit 1");

  std::string roleString;
  std::string profileString;
  float baseStat = 0.0f;
  int age = 15;

  if (result->data.empty()) {
    Log(e_Error, "PlayerData", "PlayerData",
        "Player " + int_to_str(databaseID) + " is not in the database");
    lastName = "Player " + int_to_str(databaseID);
    roles.push_back(e_PlayerRole_CM);
    return;
  }

  for (unsigned int c = 0; c < result->data.at(0).size(); c++) {
    if (result->header.at(c).compare("firstname") == 0)
      firstName = result->data.at(0).at(c);
    if (result->header.at(c).compare("lastname") == 0)
      lastName = result->data.at(0).at(c);
    if (result->header.at(c).compare("role") == 0)
      roleString = result->data.at(0).at(c);
    if (result->header.at(c).compare("base_stat") == 0)
      baseStat = atof(result->data.at(0).at(c).c_str());
    if (result->header.at(c).compare("profile_xml") == 0)
      profileString = result->data.at(0).at(c);
    if (result->header.at(c).compare("age") == 0)
      age = atoi(result->data.at(0).at(c).c_str());
    if (result->header.at(c).compare("skincolor") == 0)
      skinColor = atoi(result->data.at(0).at(c).c_str());
    if (result->header.at(c).compare("hairstyle") == 0)
      hairStyle = result->data.at(0).at(c);
    if (result->header.at(c).compare("haircolor") == 0)
      hairColor = result->data.at(0).at(c);
    if (result->header.at(c).compare("height") == 0)
      height = atof(result->data.at(0).at(c).c_str());
  }

  // Football Manager notation ("D C, DM"); splitting it on spaces made almost
  // every outfielder a CM.
  for (const std::string& code : dbroles::PositionsFromDbRole(roleString))
    roles.push_back(GetRoleFromString(code));

  // One-name players ("Isco") belong in the surname, which the HUD shows.
  if (lastName.empty())
    std::swap(firstName, lastName);

  // get average stat for current age

  XMLLoader loader;
  XMLTree tree = loader.Load(profileString);

  // printf("player: %s, %s (age %i)\n", lastName.c_str(), firstName.c_str(), age);
  map_XMLTree::const_iterator iter = tree.children.begin();
  while (iter != tree.children.end()) {
    float profileStat = atof((*iter).second.value.c_str());  // profile value

    float value = CalculateStat(baseStat, profileStat, age, e_DevelopmentCurveType_Normal);
    // printf("base: %f; profile: %f; result: %f\n", baseStat, profileStat, value);

    stats.Set((*iter).first.c_str(), value);
    iter++;
  }

  condition = e_PlayerCondition_Normal;
  CalculateCondition(0);
}

PlayerData::PlayerData(int playerDatabaseID, const std::string& firstName,
                       const std::string& lastName, const std::string& role, float baseStat,
                       int age)
    : PlayerData() {
  databaseID = playerDatabaseID;
  this->firstName = firstName;
  this->lastName = lastName;
  roles.push_back(GetRoleFromString(role));
  // An average profile (0.5) for every stat the default constructor defines.
  const map_Properties* defaults = stats.GetProperties();
  std::vector<std::string> names;
  for (const auto& entry : *defaults)
    names.push_back(entry.first);
  for (const auto& name : names)
    stats.Set(name.c_str(), CalculateStat(baseStat, 0.5f, age, e_DevelopmentCurveType_Normal));
  CalculateCondition(0);
}

PlayerData::PlayerData() {
  // officials, for example, use this constructor
  databaseID = 0;
  skinColor = int(round(random(1, 4)));
  hairStyle = "short01";
  hairColor = "darkblonde";
  height = 1.8f;
  condition = e_PlayerCondition_Normal;

  stats.Set("physical_balance", 0.6);
  stats.Set("physical_reaction", 0.6);
  stats.Set("physical_acceleration", 0.6);
  stats.Set("physical_velocity", 0.6);
  stats.Set("physical_stamina", 0.6);
  stats.Set("physical_agility", 0.6);
  stats.Set("physical_shotpower", 0.6);
  stats.Set("technical_standingtackle", 0.6);
  stats.Set("technical_slidingtackle", 0.6);
  stats.Set("technical_ballcontrol", 0.6);
  stats.Set("technical_dribble", 0.6);
  stats.Set("technical_shortpass", 0.6);
  stats.Set("technical_highpass", 0.6);
  stats.Set("technical_header", 0.6);
  stats.Set("technical_shot", 0.6);
  stats.Set("technical_volley", 0.6);
  stats.Set("mental_calmness", 0.6);
  stats.Set("mental_workrate", 0.6);
  stats.Set("mental_resilience", 0.6);
  stats.Set("mental_defensivepositioning", 0.6);
  stats.Set("mental_offensivepositioning", 0.6);
  stats.Set("mental_vision", 0.6);
}

PlayerData::~PlayerData() {}

const std::vector<e_PlayerRole>& PlayerData::GetRoles() const {
  return roles;
}

std::string PlayerData::GetRoleName() const {
  if (roles.empty())
    return "SUB";
  return ::GetRoleName(roles.front());
}

void PlayerData::CalculateCondition(int matchSeed) {
  // PES condition distribution: ~15% Red, ~20% Orange, ~30% Yellow, ~20% Blue, ~15% Purple
  int val = (std::abs(databaseID * 37 + matchSeed * 17)) % 100;
  if (val < 15)
    condition = e_PlayerCondition_Terrible;
  else if (val < 35)
    condition = e_PlayerCondition_Poor;
  else if (val < 65)
    condition = e_PlayerCondition_Normal;
  else if (val < 85)
    condition = e_PlayerCondition_Good;
  else
    condition = e_PlayerCondition_Top;
}

float PlayerData::GetConditionMultiplier() const {
  switch (condition) {
    case e_PlayerCondition_Top:
      return 1.09f;
    case e_PlayerCondition_Good:
      return 1.04f;
    case e_PlayerCondition_Normal:
      return 1.00f;
    case e_PlayerCondition_Poor:
      return 0.96f;
    case e_PlayerCondition_Terrible:
      return 0.91f;
    default:
      return 1.00f;
  }
}

std::string PlayerData::GetConditionSymbol() const {
  switch (condition) {
    // PES-style condition arrows as UTF-8 bytes (portable to MSVC code
    // pages); the HUD font has these glyphs.
    case e_PlayerCondition_Top:
      return "\xE2\x86\x91";  // up arrow
    case e_PlayerCondition_Good:
      return "\xE2\x86\x97";  // up-right arrow
    case e_PlayerCondition_Normal:
      return "\xE2\x86\x92";  // right arrow
    case e_PlayerCondition_Poor:
      return "\xE2\x86\x98";  // down-right arrow
    case e_PlayerCondition_Terrible:
      return "\xE2\x86\x93";  // down arrow
    default:
      return "\xE2\x86\x92";
  }
}

Vector3 PlayerData::GetConditionColor() const {
  switch (condition) {
    case e_PlayerCondition_Top:
      return Vector3(230, 40, 40);  // Red
    case e_PlayerCondition_Good:
      return Vector3(240, 150, 20);  // Orange
    case e_PlayerCondition_Normal:
      return Vector3(240, 220, 30);  // Yellow
    case e_PlayerCondition_Poor:
      return Vector3(50, 130, 240);  // Blue
    case e_PlayerCondition_Terrible:
      return Vector3(160, 60, 200);  // Purple
    default:
      return Vector3(240, 220, 30);
  }
}

float PlayerData::GetStat(const char* name) {
  bool exists = stats.Exists(name);
  if (!exists)
    printf("Stat named '%s' does not exist!\n", name);
  assert(exists);
  float baseValue = stats.GetReal(name, 1.0f);
  return clamp(baseValue * GetConditionMultiplier(), 0.01f, 1.0f);
}
