// written by bastiaan konings schuiling 2008 - 2015
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not
// be used for anything important. i do not offer support, so don't ask. to be used for inspiration
// :)

#include "playerdata.hpp"

#include <cmath>

#include "../main.hpp"
#include "base/utils.hpp"
#include "utils/database.hpp"

PlayerData::PlayerData(int playerDatabaseID) : databaseID(playerDatabaseID) {
  // std::string test = "select * from players where id = " + int_to_str(databaseID) + " limit 1";
  // printf("test: %s\n", test.c_str());

  auto result = GetDB()->Query(
      "select firstname, lastname, role, base_stat, profile_xml, age, skincolor, hairstyle, "
      "haircolor, height from players where id = " +
      int_to_str(databaseID) + " limit 1");

  std::string roleString;
  std::string profileString;
  float baseStat = 0.0f;
  int age = 15;

  skinColor = int(round(random(1, 4)));
  hairStyle = "short01";
  hairColor = "darkblonde";
  height = 1.8f;

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


  std::vector<std::string> roleStrings;
  tokenize(roleString, roleStrings);

  for (int i = 0; i < (signed int)roleStrings.size(); i++) {
    roles.push_back(GetRoleFromString(roleStrings.at(i)));
  }

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

PlayerData::PlayerData() {
  // officials, for example, use this constructor
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
    case e_PlayerCondition_Top:
      return "^";
    case e_PlayerCondition_Good:
      return "/^";
    case e_PlayerCondition_Normal:
      return "->";
    case e_PlayerCondition_Poor:
      return "\\v";
    case e_PlayerCondition_Terrible:
      return "v";
    default:
      return "->";
  }
}

Vector3 PlayerData::GetConditionColor() const {
  switch (condition) {
    case e_PlayerCondition_Top:
      return Vector3(230, 40, 40);     // Red
    case e_PlayerCondition_Good:
      return Vector3(240, 150, 20);    // Orange
    case e_PlayerCondition_Normal:
      return Vector3(240, 220, 30);    // Yellow
    case e_PlayerCondition_Poor:
      return Vector3(50, 130, 240);    // Blue
    case e_PlayerCondition_Terrible:
      return Vector3(160, 60, 200);    // Purple
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
