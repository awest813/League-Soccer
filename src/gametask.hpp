#ifndef _HPP_FOOTBALL_GAMETASK
#define _HPP_FOOTBALL_GAMETASK

#include "hid/gamepad.hpp"
#include "hid/keyboard.hpp"
#include "menu/menuscene.hpp"
#include "menu/menutask.hpp"
#include "onthepitch/match.hpp"
#include "types/iusertask.hpp"
#include "types/thread.hpp"

using namespace blunted;

class UpdateFullbodyModel : public Command {
public:
  UpdateFullbodyModel(const std::vector<PlayerBase*>& playersToProcess)
      : Command("UpdateFullBodyModel"), playersToProcess(playersToProcess) {};
  virtual ~UpdateFullbodyModel() {};

protected:
  void Update() {
    for (auto* player : playersToProcess) {
      player->UpdateFullbodyModel();
    }
  }

  virtual bool Execute(void* = nullptr) override {
    Update();
    return true;
  }

  std::vector<PlayerBase*> playersToProcess;
};

class UploadFullbodyModel : public Command {
public:
  UploadFullbodyModel(std::vector<boost::intrusive_ptr<Geometry>> geometryToUpload)
      : Command("UploadFullBodyModel"), geometryToUpload(geometryToUpload) {};
  virtual ~UploadFullbodyModel() {};

protected:
  void Update();

  virtual bool Execute(void* = nullptr) {
    Update();
    return true;
  }

  std::vector<boost::intrusive_ptr<Geometry>>
      geometryToUpload;  // use shared pointers to geometry objects instead, to assure lifetime
};

enum e_GameTaskMessage {
  e_GameTaskMessage_StartMatch,
  e_GameTaskMessage_StopMatch,
  e_GameTaskMessage_StartMenuScene,
  e_GameTaskMessage_StopMenuScene,
  e_GameTaskMessage_None
};

class GameTask : public IUserTask {
public:
  GameTask();
  virtual ~GameTask();

  void Exit();

  void Action(e_GameTaskMessage message);

  virtual void GetPhase();
  virtual void ProcessPhase();
  virtual void PutPhase();

  Match* GetMatch() { return match; }
  MenuScene* GetMenuScene() { return menuScene; }

  MessageQueue<e_GameTaskMessage> messageQueue;

  virtual std::string GetName() const { return "game"; }

  std::mutex matchLifetimeMutex;

protected:
  Match* match;
  MenuScene* menuScene;

  std::timed_mutex matchPutBufferMutex;
  std::mutex menuSceneLifetimeMutex;

  std::shared_ptr<Scene3D> scene3D;
};

#endif
