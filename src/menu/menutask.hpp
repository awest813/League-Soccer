#ifndef _HPP_GUI2_MENUTASK
#define _HPP_GUI2_MENUTASK

#include "../gamedefines.hpp"
#include "ingame/gamepage.hpp"
#include "scene/scene3d/scene3d.hpp"
#include "utils/gui2/guitask.hpp"
#include "utils/gui2/widgets/image.hpp"

class Match;
class MatchData;

using namespace blunted;

constexpr float kMenuAspectRatio = 16.0f / 9.0f;

enum e_MenuAction {
  e_MenuAction_Menu,  // start main menu
  e_MenuAction_Game,  // start game
  e_MenuAction_None
};

struct SideSelection {
  int controllerID = -1;
  Gui2Image* controllerImage = nullptr;
  int side = 0;  // -1, 0, 1
};

// todo: just load match-, team-, and playerdata before starting match
// this requires some bigger changes, so stick with this imperfect system for the time being
struct QueuedFixture {
  QueuedFixture() {
    team1KitNum = 1;
    team2KitNum = 2;
    matchData = 0;
  }
  std::vector<SideSelection> sides;  // queued match fixture
  std::string teamID1, teamID2;      // queued match fixture
  int team1KitNum, team2KitNum;
  MatchData* matchData;
};

void SetActiveController(int side, bool keyboard);

class MenuTask : public Gui2Task {
public:
  MenuTask(float aspectRatio, float margin, TTF_Font* defaultFont, TTF_Font* defaultOutlineFont);
  virtual ~MenuTask();

  virtual void ProcessPhase();

  bool QuickStart();
  void QuitGame();
  // Quits once delay_ms has passed (smoke tests keep a page on screen long
  // enough to be drawn and captured).
  void QuitGameAfter(unsigned long delay_ms);

  void ReleaseAllButtons();

  // A gamepad that was playing in the current match was unplugged. Returns the
  // side it played for (-1/1) once, or 0 when there is nothing to report.
  int ConsumeControllerDisconnect() {
    const int side = disconnectedSide;
    disconnectedSide = 0;
    return side;
  }
  // The controller setup changed outside the controller pages (a pad was
  // unplugged); the match should rebuild its human players before resuming.
  bool ConsumeControllerSetupChanged() {
    const bool changed = controllerSetupChanged;
    controllerSetupChanged = false;
    return changed;
  }

  void SetControllerSetup(const std::vector<SideSelection>& sides) {
    queuedFixture.Lock();
    queuedFixture->sides = sides;
    queuedFixture.Unlock();
  }
  const std::vector<SideSelection> GetControllerSetup() { return queuedFixture.GetData().sides; }
  void SetTeamIDs(const std::string& id1, const std::string& id2) {
    queuedFixture.Lock();
    queuedFixture->teamID1 = id1;
    queuedFixture->teamID2 = id2;
    queuedFixture.Unlock();
  }
  int GetTeamID(int whichOne) {
    if (whichOne == 0)
      return atoi(queuedFixture.GetData().teamID1.c_str());
    else
      return atoi(queuedFixture.GetData().teamID2.c_str());
  }
  int GetTeamKitNum(int teamID) {
    if (teamID == 0)
      return queuedFixture.GetData().team1KitNum;
    else
      return queuedFixture.GetData().team2KitNum;
  }
  void SetMatchData(MatchData* matchData) {
    queuedFixture.Lock();
    queuedFixture->matchData = matchData;
    queuedFixture.Unlock();
  }
  MatchData* GetMatchData() {
    return queuedFixture.GetData().matchData;
  }  // hint: this lock is useless, since we are returning the pointer and not a copy

  void SetMenuAction(e_MenuAction menuAction) { this->menuAction = menuAction; }

protected:
  void HandleRemovedControllers();

  int disconnectedSide = 0;
  unsigned long quitAt_ms = 0;
  bool controllerSetupChanged = false;

  e_MenuAction menuAction;

  Gui2Image* menuBackground;
  Lockable<QueuedFixture> queuedFixture;  // todo: we can probably unlock this stuff
};

#endif
