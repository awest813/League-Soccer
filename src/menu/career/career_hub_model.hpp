#pragma once

#include "../../data/careerdata.hpp"

enum class CareerHubSection { OVERVIEW, TEAM, ROLE, COMPETITIONS, INBOX };
enum class CareerHubAction {
  SQUAD,
  STRATEGY,
  TRAINING,
  TRANSFERS,
  FREE_AGENTS,
  YOUTH,
  FINANCES,
  STADIUM,
  STAFF,
  SPONSORS,
  BOARD,
  RESPONSIBILITIES,
  STANDINGS,
  SEASON,
  EXPANSION,
  CUSTOM,
  REQUEST_TRANSFER,
  PRESS
};
struct CareerHubTool {
  CareerHubAction action;
  const char* id;
  const char* label;
};

inline const PlayerCareerState* ControlledCareerPlayer(const CareerSave& save) {
  if (save.mode != CareerMode::PLAYER || save.controlledEntityID <= 0)
    return nullptr;
  const PlayerCareerState* found = nullptr;
  for (const auto& player : save.roster) {
    if (player.databaseID != save.controlledEntityID)
      continue;
    if (found)
      return nullptr;  // Ambiguous old saves need identity repair, not a guess.
    found = &player;
  }
  return found;
}

inline const char* CareerWorkspaceKey(CareerMode mode) {
  return mode == CareerMode::OWNER_GM ? "career_nav_office"
         : mode == CareerMode::COACH  ? "career_nav_coaching"
                                      : "career_nav_mypro";
}

inline std::vector<CareerHubTool> CareerHubTools(const CareerSave& save, CareerHubSection section) {
  using A = CareerHubAction;
  std::vector<CareerHubTool> tools;
  if (section == CareerHubSection::TEAM) {
    tools.push_back({A::SQUAD, "squad", "career_nav_squad"});
    if (CanManageTeam(save)) {
      tools.push_back({A::STRATEGY, "strategy", "career_nav_tactics"});
      tools.push_back({A::TRAINING, "training", "career_nav_training"});
    }
    if (CanManageClub(save)) {
      tools.push_back({A::TRANSFERS, "transfers", "career_nav_transfers"});
      tools.push_back({A::FREE_AGENTS, "free_agents", "career_nav_free_agents"});
      tools.push_back({A::YOUTH, "youth", "career_nav_academy"});
    }
  } else if (section == CareerHubSection::ROLE) {
    if (CanManageClub(save)) {
      tools = {{A::FINANCES, "finances", "career_nav_finances"},
               {A::STADIUM, "stadium", "career_nav_stadium"},
               {A::STAFF, "staff", "career_nav_staff"},
               {A::SPONSORS, "sponsors", "career_nav_sponsors"},
               {A::BOARD, "board", "career_nav_board"},
               {A::RESPONSIBILITIES, "responsibilities",
                save.handsOnManagement ? "career_control_hands_on" : "career_control_delegated"}};
    } else if (save.mode == CareerMode::COACH) {
      tools = {{A::SQUAD, "squad", "career_nav_squad"},
               {A::STRATEGY, "strategy", "career_nav_tactics"},
               {A::TRAINING, "training", "career_nav_training"}};
    } else if (const auto* player = ControlledCareerPlayer(save)) {
      tools = {{A::TRAINING, "training", "career_nav_personal_training"},
               {A::REQUEST_TRANSFER, "request_transfer",
                player->contract.transferListed ? "career_nav_cancel_request"
                                                : "career_nav_request_transfer"}};
    }
    if (CanManageClub(save) || save.mode == CareerMode::COACH)
      tools.push_back({A::PRESS, "press", "career_press_nav"});
  } else if (section == CareerHubSection::COMPETITIONS) {
    tools = {{A::STANDINGS, "standings", "career_nav_standings"},
             {A::SEASON, "season", "career_nav_season"}};
    if (CanManageClub(save)) {
      tools.push_back({A::EXPANSION, "expansion", "career_nav_rules"});
      tools.push_back({A::CUSTOM, "custom", "career_nav_custom"});
    }
  }
  return tools;
}
