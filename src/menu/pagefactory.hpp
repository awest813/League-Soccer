#ifndef _HPP_PAGEFACTORY
#define _HPP_PAGEFACTORY

#include "utils/gui2/page.hpp"

using namespace blunted;

enum e_PageID {
  e_PageID_MainMenu,
  e_PageID_Game,
  e_PageID_Intro,
  e_PageID_Outro,
  e_PageID_Credits,
  e_PageID_Settings,
  e_PageID_ControllerSelect,
  e_PageID_TeamSelect,
  e_PageID_MatchOptions,
  e_PageID_LoadingMatch,
  e_PageID_MatchPhase,
  e_PageID_GameOver,
  e_PageID_Ingame,
  e_PageID_PreQuit,
  e_PageID_GamePlan,
  e_PageID_VisualOptions,
  e_PageID_Replay,
  e_PageID_Camera,
  e_PageID_Gameplay,
  e_PageID_Controller,
  e_PageID_Keyboard,
  e_PageID_Gamepads,
  e_PageID_GamepadSetup,
  e_PageID_GamepadCalibration,
  e_PageID_GamepadMapping,
  e_PageID_GamepadFunction,
  e_PageID_Graphics,
  e_PageID_Audio,
  e_PageID_Language,

  e_PageID_League_Start,
  e_PageID_League_Start_Load,
  e_PageID_League_Start_New,
  e_PageID_League,
  e_PageID_League_Forward,
  e_PageID_League_Inbox,
  e_PageID_League_Team,
  e_PageID_League_Team_Formation,
  e_PageID_League_Team_PlayerSelection,
  e_PageID_League_Team_Tactics,
  e_PageID_League_Team_PlayerOverview,
  e_PageID_League_Team_PlayerDevelopment,
  e_PageID_League_Team_Setup,
  e_PageID_League_Calendar,
  e_PageID_League_Standings,
  e_PageID_League_Standings_League_Table,
  e_PageID_League_Standings_League_Stats,
  e_PageID_League_Management,
  e_PageID_League_Management_Contracts,
  e_PageID_League_Management_Transfers,
  e_PageID_League_System,
  e_PageID_League_System_Save,
  e_PageID_League_System_Settings,
  e_PageID_League_PreMatch,
  e_PageID_League_Matchday,

  e_PageID_MatchHistory,
  e_PageID_SetPieceEditor,

  e_PageID_CareerMenu,
  e_PageID_CareerNewGame,
  e_PageID_CareerHub,
  e_PageID_CareerTransferMarket,
  e_PageID_CareerTransferBids,
  e_PageID_CareerTransferBidDetail,
  e_PageID_CareerPressConference,  // 6.13
  e_PageID_CareerLeagueExpansion,  // 6.16
  e_PageID_CareerCustomLeague,     // 6.17
  e_PageID_CareerFreeAgency,
  e_PageID_CareerTraining,
  e_PageID_CareerStrategy,
  e_PageID_CareerYouthAcademy,
  e_PageID_CareerSquadRoster,
  e_PageID_CareerSeason,
  e_PageID_CareerMatchday,

  e_PageID_OwnerHub,
  e_PageID_OwnerStadium,
  e_PageID_OwnerFinances,
  e_PageID_OwnerStaff,
  e_PageID_OwnerStaffHire,
  e_PageID_OwnerSponsors,
  e_PageID_OwnerBoardRoom
};

class PageFactory : public Gui2PageFactory {
public:
  using Gui2PageFactory::CreatePage;
  virtual Gui2Page* CreatePage(const Gui2PageData& pageData);

protected:
};

#endif
