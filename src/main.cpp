#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <filesystem>

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include "SDL2/SDL_ttf.h"
#include "base/log.hpp"
#include "base/math/bluntmath.hpp"
#include "base/utils.hpp"
#include "framework/scheduler.hpp"
#include "hid/scriptedgamepad.hpp"
#include "main.hpp"
#include "managers/resourcemanagerpool.hpp"
#include "managers/scenemanager.hpp"
#include "managers/systemmanager.hpp"
#include "scene/objectfactory.hpp"
#include "scene/scene2d/scene2d.hpp"
#include "scene/scene3d/scene3d.hpp"
#include "systems/audio/audio_system.hpp"
#include "types/thread.hpp"
#include "utils/localization.hpp"
#include "utils/objectloader.hpp"
#include "utils/orbitcamera.hpp"
#include "utils/threadhud.hpp"

#if defined(WIN32) && defined(__MINGW32__)
#undef main
#endif

using namespace blunted;

GraphicsSystem* graphicsSystem;
AudioSystem* audioSystem;

std::shared_ptr<Scene2D> scene2D;
std::shared_ptr<Scene3D> scene3D;

std::shared_ptr<TaskSequence> graphicsSequence;
std::shared_ptr<TaskSequence> gameSequence;

std::shared_ptr<GameTask> gameTask;
std::shared_ptr<MenuTask> menuTask;

boost::intrusive_ptr<Geometry> greenPilon;
boost::intrusive_ptr<Geometry> bluePilon;
boost::intrusive_ptr<Geometry> yellowPilon;
boost::intrusive_ptr<Geometry> redPilon;

boost::intrusive_ptr<Geometry> smallDebugCircle1;
boost::intrusive_ptr<Geometry> smallDebugCircle2;
boost::intrusive_ptr<Geometry> largeDebugCircle;

void SetGreenDebugPilon(const Vector3& pos) {
  greenPilon->SetPosition(pos, false);
}
void SetBlueDebugPilon(const Vector3& pos) {
  bluePilon->SetPosition(pos, false);
}
void SetYellowDebugPilon(const Vector3& pos) {
  yellowPilon->SetPosition(pos, false);
}
void SetRedDebugPilon(const Vector3& pos) {
  redPilon->SetPosition(pos, false);
}

void SetSmallDebugCircle1(const Vector3& pos) {
  smallDebugCircle1->SetPosition(pos, false);
}
void SetSmallDebugCircle2(const Vector3& pos) {
  smallDebugCircle2->SetPosition(pos, false);
}
void SetLargeDebugCircle(const Vector3& pos) {
  largeDebugCircle->SetPosition(pos, false);
}

boost::intrusive_ptr<Geometry> GetGreenDebugPilon() {
  return greenPilon;
}
boost::intrusive_ptr<Geometry> GetBlueDebugPilon() {
  return bluePilon;
}
boost::intrusive_ptr<Geometry> GetYellowDebugPilon() {
  return yellowPilon;
}
boost::intrusive_ptr<Geometry> GetRedDebugPilon() {
  return redPilon;
}

boost::intrusive_ptr<Geometry> GetSmallDebugCircle1() {
  return smallDebugCircle1;
}
boost::intrusive_ptr<Geometry> GetSmallDebugCircle2() {
  return smallDebugCircle2;
}
boost::intrusive_ptr<Geometry> GetLargeDebugCircle() {
  return largeDebugCircle;
}

Database* db;

Properties* config;

boost::intrusive_ptr<Image2D> debugImage;
boost::intrusive_ptr<Image2D> debugOverlay;

std::vector<IHIDevice*> controllers;

bool superDebug = false;
e_DebugMode debugMode = e_DebugMode_Off;

std::string activeSaveDirectory;

std::string configFile = "football.config";
std::string GetConfigFilename() {
  return configFile;
}

namespace {

std::string ResolveConfigFilename(const std::string& requestedFilename) {
  namespace fs = std::filesystem;

  fs::path requestedPath(requestedFilename);
  if (fs::exists(requestedPath)) {
    return requestedPath.generic_string();
  }

  if (requestedPath.is_relative()) {
    fs::path dataRelativePath = fs::path("data") / requestedPath.filename();
    if (fs::exists(dataRelativePath)) {
      return dataRelativePath.generic_string();
    }
  }

  return requestedFilename;
}

}  // namespace

std::shared_ptr<Scene2D> GetScene2D() {
  return scene2D;
}

std::shared_ptr<Scene3D> GetScene3D() {
  return scene3D;
}

GraphicsSystem* GetGraphicsSystem() {
  return graphicsSystem;
}

std::shared_ptr<GameTask> GetGameTask() {
  return gameTask;
}

std::shared_ptr<MenuTask> GetMenuTask() {
  return menuTask;
}

Database* GetDB() {
  return db;
}

bool IsReleaseVersion() {
  if (GetConfiguration()->GetBool("debug", false))
    return false;
  else
    return true;
}

bool Verbose() {
  return !IsReleaseVersion();
}

bool UpdateNonImportableDB() {
  if (IsReleaseVersion())
    return false;
  else
    return true;
}

Properties* GetConfiguration() {
  return config;
}

std::string GetActiveSaveDirectory() {
  return activeSaveDirectory;
}

void SetActiveSaveDirectory(const std::string& dir) {
  activeSaveDirectory = dir;
}

bool SuperDebug() {
  return superDebug;
}

e_DebugMode GetDebugMode() {
  return debugMode;
}

boost::intrusive_ptr<Image2D> GetDebugImage() {
  return debugImage;
}

boost::intrusive_ptr<Image2D> GetDebugOverlay() {
  return debugOverlay;
}

void GetDebugOverlayCoord(Match* match, const Vector3& worldPos, int& x, int& y) {
  Vector3 proj = GetProjectedCoord(worldPos, match->GetCamera());
  int dud1, dud2;
  GetMenuTask()->GetWindowManager()->GetCoordinates(proj.coords[0], proj.coords[1], 1, 1, x, y,
                                                    dud1, dud2);

  int contextW, contextH, bpp;
  GetScene2D()->GetContextSize(contextW, contextH, bpp);
  x = clamp(x, 0, contextW - 1);
  y = clamp(y, 0, contextH - 1);
}

int PredictFrameTimeToGo_ms(int frameCount) {
  int averageFrameTime_ms = GetGraphicsSystem()->GetAverageFrameTime_ms(frameCount);
  int timeSinceLastSwap_ms = GetGraphicsSystem()->GetTimeSinceLastSwap_ms();
  int timeToNextSwapPrediction_ms = averageFrameTime_ms - timeSinceLastSwap_ms;
  // printf("super prediction! %i - %i = %i\n", averageFrameTime_ms, timeSinceLastSwap_ms,
  // timeToNextSwapPrediction_ms);
  timeToNextSwapPrediction_ms = clamp(timeToNextSwapPrediction_ms, 0, 1000);
  return timeToNextSwapPrediction_ms;
}

void InitDebugImage() {
  SDL_Surface* sdlSurface = CreateSDLSurface(200, 150);

  boost::intrusive_ptr<Resource<Surface>> resource =
      ResourceManagerPool::GetInstance()
          .GetManager<Surface>(e_ResourceType_Surface)
          ->Fetch("debugimage", false, true);
  Surface* surface = resource->GetResource();

  surface->SetData(sdlSurface);

  debugImage = boost::static_pointer_cast<Image2D>(
      ObjectFactory::GetInstance().CreateObject("debugimage", e_ObjectType_Image2D));
  scene2D->CreateSystemObjects(debugImage);
  debugImage->SetImage(resource);

  int contextW, contextH, bpp;  // context
  scene2D->GetContextSize(contextW, contextH, bpp);
  debugImage->SetPosition(contextW - 210, contextH - 160);

  scene2D->AddObject(debugImage);

  debugImage->DrawRectangle(0, 0, 200, 150, Vector3(40, 20, 20), 100);
  debugImage->OnChange();
}

void InitDebugOverlay() {
  int contextW, contextH, bpp;  // context
  scene2D->GetContextSize(contextW, contextH, bpp);

  SDL_Surface* sdlSurface = CreateSDLSurface(contextW, contextH);

  boost::intrusive_ptr<Resource<Surface>> resource =
      ResourceManagerPool::GetInstance()
          .GetManager<Surface>(e_ResourceType_Surface)
          ->Fetch("debugoverlay", false, true);
  Surface* surface = resource->GetResource();

  surface->SetData(sdlSurface);

  debugOverlay = boost::static_pointer_cast<Image2D>(
      ObjectFactory::GetInstance().CreateObject("debugoverlay", e_ObjectType_Image2D));
  scene2D->CreateSystemObjects(debugOverlay);
  debugOverlay->SetImage(resource);

  debugOverlay->SetPosition(0, 0);

  scene2D->AddObject(debugOverlay);

  debugOverlay->DrawRectangle(0, 0, contextW, contextH, Vector3(0, 0, 0), 0);
  debugOverlay->OnChange();
}

const std::vector<IHIDevice*>& GetControllers() {
  return controllers;
}

void AddGamepad(int deviceIndex, int gamepadID) {
  if (deviceIndex < 0 || deviceIndex >= SDL_NumJoysticks())
    return;
  HIDGamepad* gamepad = new HIDGamepad(deviceIndex, gamepadID);
  controllers.push_back(gamepad);
  printf("[main] Gamepad added: index %d (slot %d), total controllers: %zu\n", deviceIndex,
         gamepadID, controllers.size());
}

void RemoveGamepad(int gamepadID) {
  if (controllers.size() <= 1)
    return;
  for (auto it = controllers.begin() + 1; it != controllers.end(); ++it) {
    HIDGamepad* gp = dynamic_cast<HIDGamepad*>(*it);
    if (gp && gp->GetGamepadID() == gamepadID) {
      delete gp;
      controllers.erase(it);
      printf("[main] Gamepad removed: slot %d, total controllers: %zu\n", gamepadID,
             controllers.size());
      break;
    }
  }

  // Dense-slot renumbering: UserEventManager packs the remaining joystick slots
  // down on removal (see CompactJoystickSlots), so any gamepad that lived in a
  // higher slot must track its new, one-lower slot.
  for (IHIDevice* device : controllers) {
    HIDGamepad* gp = dynamic_cast<HIDGamepad*>(device);
    if (gp && gp->GetGamepadID() > gamepadID) {
      gp->SetGamepadID(gp->GetGamepadID() - 1);
    }
  }
}

class ThreadHudThread : public Thread {
public:
  ThreadHudThread() { hud = new ThreadHud(GetScene2D()); }
  virtual ~ThreadHudThread() { delete hud; }

  virtual void operator()() override {
    bool quit = false;
    while (!quit) {
      SetState(e_ThreadState_Busy);

      bool isMessage = false;
      boost::intrusive_ptr<Command> message = boost::intrusive_ptr<Command>();
      message = messageQueue.GetMessage(isMessage);
      if (isMessage) {
        if (!message->Handle(this))
          quit = true;
        message.reset();
      }

      hud->Execute();

      SetState(e_ThreadState_Idle);

      std::this_thread::yield();
    }
  }

protected:
  ThreadHud* hud;
};

int main(int argc, const char** argv) {
  if (argc > 0) {
    try {
      std::filesystem::path exeDir = std::filesystem::absolute(std::filesystem::path(argv[0])).parent_path();
      if (!exeDir.empty()) {
        std::filesystem::current_path(exeDir);
      }
    } catch (...) {}
  }
  config = new Properties();
  if (argc > 1)
    configFile = argv[1];
  configFile = ResolveConfigFilename(configFile);
  config->LoadFile(configFile.c_str());

  // Initialize localization using saved language preference (default: "en")
  Localization::GetInstance().Load(config->Get("locale_language", "en"));

  Initialize(*config);

  srand(time(nullptr));
  rand();        // mingw32? buggy compiler? first value seems bogus
  randomseed();  // for the boost random
  fastrandomseed();

  int timeStep_ms = config->GetInt("physics_frametime_ms", 10);

  // database

  db = new Database();
  bool dbSuccess = db->Load("databases/default/database.sqlite");
  if (!dbSuccess)
    Log(e_FatalError, "main", "()", "Could not open database");

  // initialize systems

  SystemManager* systemManager = SystemManager::GetInstancePtr();

  graphicsSystem = new GraphicsSystem();
  bool returnvalue = systemManager->RegisterSystem("GraphicsSystem", graphicsSystem);
  if (!returnvalue)
    Log(e_FatalError, "football", "main", "Could not register GraphicsSystem");

  audioSystem = new AudioSystem();
  returnvalue = systemManager->RegisterSystem("AudioSystem", audioSystem);
  if (!returnvalue)
    Log(e_FatalError, "football", "main", "Could not register AudioSystem");

  printf("[MAIN] graphicsSystem->Initialize\n"); fflush(stdout);
  graphicsSystem->Initialize(*config);
  printf("[MAIN] audioSystem->Initialize\n"); fflush(stdout);
  audioSystem->Initialize(*config);

  // init scenes
  printf("[MAIN] init scenes\n"); fflush(stdout);

  scene2D = std::shared_ptr<Scene2D>(new Scene2D("scene2D", *config));
  SceneManager::GetInstance().RegisterScene(scene2D);

  scene3D = std::shared_ptr<Scene3D>(new Scene3D("scene3D"));
  SceneManager::GetInstance().RegisterScene(scene3D);

  if (SuperDebug())
    InitDebugImage();
  if (GetDebugMode() == e_DebugMode_AI)
    InitDebugOverlay();

  ThreadHudThread* threadHudThread = nullptr;
  if (!IsReleaseVersion() && 1 == 2) {
    threadHudThread = new ThreadHudThread();
    threadHudThread->Run();
  } else {
    threadHudThread = nullptr;
  }

  // debug pilons
  printf("[MAIN] loading pilons\n"); fflush(stdout);

  boost::intrusive_ptr<Resource<GeometryData>> geometry =
      ResourceManagerPool::GetInstance()
          .GetManager<GeometryData>(e_ResourceType_GeometryData)
          ->Fetch("media/objects/helpers/green.ase", true);
  greenPilon = boost::static_pointer_cast<Geometry>(
      ObjectFactory::GetInstance().CreateObject("greenPilon", e_ObjectType_Geometry));
  scene3D->CreateSystemObjects(greenPilon);
  greenPilon->SetGeometryData(geometry);
  greenPilon->SetLocalMode(e_LocalMode_Absolute);
  greenPilon->SetPosition(Vector3(0, 0, -10));

  geometry = ResourceManagerPool::GetInstance()
                 .GetManager<GeometryData>(e_ResourceType_GeometryData)
                 ->Fetch("media/objects/helpers/blue.ase", true);
  bluePilon = boost::static_pointer_cast<Geometry>(
      ObjectFactory::GetInstance().CreateObject("bluePilon", e_ObjectType_Geometry));
  scene3D->CreateSystemObjects(bluePilon);
  bluePilon->SetGeometryData(geometry);
  bluePilon->SetLocalMode(e_LocalMode_Absolute);
  bluePilon->SetPosition(Vector3(0, 0, -10));

  geometry = ResourceManagerPool::GetInstance()
                 .GetManager<GeometryData>(e_ResourceType_GeometryData)
                 ->Fetch("media/objects/helpers/yellow.ase", true);
  yellowPilon = boost::static_pointer_cast<Geometry>(
      ObjectFactory::GetInstance().CreateObject("yellowPilon", e_ObjectType_Geometry));
  scene3D->CreateSystemObjects(yellowPilon);
  yellowPilon->SetGeometryData(geometry);
  yellowPilon->SetLocalMode(e_LocalMode_Absolute);
  yellowPilon->SetPosition(Vector3(0, 0, -10));

  geometry = ResourceManagerPool::GetInstance()
                 .GetManager<GeometryData>(e_ResourceType_GeometryData)
                 ->Fetch("media/objects/helpers/red.ase", true);
  redPilon = boost::static_pointer_cast<Geometry>(
      ObjectFactory::GetInstance().CreateObject("redPilon", e_ObjectType_Geometry));
  scene3D->CreateSystemObjects(redPilon);
  redPilon->SetGeometryData(geometry);
  redPilon->SetLocalMode(e_LocalMode_Absolute);
  redPilon->SetPosition(Vector3(0, 0, -10));

  geometry = ResourceManagerPool::GetInstance()
                 .GetManager<GeometryData>(e_ResourceType_GeometryData)
                 ->Fetch("media/objects/helpers/smalldebugcircle.ase", true);
  smallDebugCircle1 = boost::static_pointer_cast<Geometry>(
      ObjectFactory::GetInstance().CreateObject("smallDebugCircle1", e_ObjectType_Geometry));
  scene3D->CreateSystemObjects(smallDebugCircle1);
  smallDebugCircle1->SetGeometryData(geometry);
  smallDebugCircle1->SetLocalMode(e_LocalMode_Absolute);
  smallDebugCircle1->SetPosition(Vector3(0, 0, -10));

  geometry = ResourceManagerPool::GetInstance()
                 .GetManager<GeometryData>(e_ResourceType_GeometryData)
                 ->Fetch("media/objects/helpers/smalldebugcircle.ase", true);
  smallDebugCircle2 = boost::static_pointer_cast<Geometry>(
      ObjectFactory::GetInstance().CreateObject("smallDebugCircle2", e_ObjectType_Geometry));
  scene3D->CreateSystemObjects(smallDebugCircle2);
  smallDebugCircle2->SetGeometryData(geometry);
  smallDebugCircle2->SetLocalMode(e_LocalMode_Absolute);
  smallDebugCircle2->SetPosition(Vector3(0, 0, -10));

  geometry = ResourceManagerPool::GetInstance()
                 .GetManager<GeometryData>(e_ResourceType_GeometryData)
                 ->Fetch("media/objects/helpers/largedebugcircle.ase", true);
  largeDebugCircle = boost::static_pointer_cast<Geometry>(
      ObjectFactory::GetInstance().CreateObject("largeDebugCircle", e_ObjectType_Geometry));
  scene3D->CreateSystemObjects(largeDebugCircle);
  largeDebugCircle->SetGeometryData(geometry);
  largeDebugCircle->SetLocalMode(e_LocalMode_Absolute);
  largeDebugCircle->SetPosition(Vector3(0, 0, -10));

  geometry.reset();

  // controllers
  printf("[MAIN] init controllers\n"); fflush(stdout);

  HIDKeyboard* keyboard = new HIDKeyboard();
  controllers.push_back(keyboard);

  // Headless "gamepad controls" smoke match: inject an autonomous scripted
  // controller that drives a human team through the real input pipeline.
  if (config->GetBool("menu_smoke_test_gamepad_match", false)) {
    controllers.push_back(new ScriptedGamepad());
    printf("[main] Scripted gamepad controller injected (%zu total controllers)\n",
           controllers.size());
  }

  // sequences

  std::mutex
      graphicsGameMutex;  // todo: this mutex seems necessary for visual fluency, doesn't this imply
                          // that i'm setting positional stuff during something else than gametask
                          // put? (or reading during something else than graphics get)

  printf("[MAIN] creating GameTask\n"); fflush(stdout);
  gameTask = std::shared_ptr<GameTask>(new GameTask());

  std::string fontfilename =
      config->Get("font_filename", "media/fonts/alegreya/AlegreyaSansSC-ExtraBold.ttf");
  printf("[MAIN] loading font: %s\n", fontfilename.c_str()); fflush(stdout);
  TTF_Font* defaultFont = TTF_OpenFont(fontfilename.c_str(), 32);
  if (!defaultFont)
    Log(e_FatalError, "football", "main", "Could not load font " + fontfilename);
  TTF_Font* defaultOutlineFont = TTF_OpenFont(fontfilename.c_str(), 32);
  TTF_SetFontOutline(defaultOutlineFont, 2);
  printf("[MAIN] creating MenuTask\n"); fflush(stdout);
  menuTask = std::shared_ptr<MenuTask>(
      new MenuTask(kMenuAspectRatio, 0, defaultFont, defaultOutlineFont));
  if (controllers.size() > 1) {
    HIDGamepad* menuGamepad = dynamic_cast<HIDGamepad*>(controllers.at(1));
    if (menuGamepad) {
      menuTask->SetEventJoyButtons(menuGamepad->GetControllerMapping(e_ControllerButton_A),
                                   menuGamepad->GetControllerMapping(e_ControllerButton_B));
    }
  }

  gameSequence = std::shared_ptr<TaskSequence>(new TaskSequence("game", timeStep_ms, false));

  gameSequence->AddUserTaskEntry(menuTask, e_TaskPhase_Get);
  gameSequence->AddUserTaskEntry(menuTask, e_TaskPhase_Process);
  gameSequence->AddUserTaskEntry(menuTask, e_TaskPhase_Put);

  gameSequence->AddUserTaskEntry(gameTask, e_TaskPhase_Get);
  gameSequence->AddUserTaskEntry(gameTask, e_TaskPhase_Process);

  GetScheduler()->RegisterTaskSequence(gameSequence);

  graphicsSequence = std::shared_ptr<TaskSequence>(
      new TaskSequence("graphics", config->GetInt("graphics3d_frametime_ms", 0), true));

  graphicsSequence->AddUserTaskEntry(gameTask, e_TaskPhase_Put);
  graphicsSequence->AddSystemTaskEntry(graphicsSystem, e_TaskPhase_Get);
  graphicsSequence->AddSystemTaskEntry(graphicsSystem, e_TaskPhase_Process);
  graphicsSequence->AddSystemTaskEntry(graphicsSystem, e_TaskPhase_Put);

  GetScheduler()->RegisterTaskSequence(graphicsSequence);

  // fire!
  printf("[MAIN] calling Run()\n"); fflush(stdout);

  Run();

  // exit

  if (SuperDebug())
    scene2D->DeleteObject(debugImage);
  if (GetDebugMode() == e_DebugMode_AI)
    scene2D->DeleteObject(debugOverlay);

  gameTask.reset();
  menuTask.reset();

  gameSequence.reset();
  graphicsSequence.reset();

  greenPilon->Exit();
  greenPilon.reset();
  bluePilon->Exit();
  bluePilon.reset();
  yellowPilon->Exit();
  yellowPilon.reset();
  redPilon->Exit();
  redPilon.reset();
  smallDebugCircle1->Exit();
  smallDebugCircle1.reset();
  smallDebugCircle2->Exit();
  smallDebugCircle2.reset();
  largeDebugCircle->Exit();
  largeDebugCircle.reset();

  if (threadHudThread) {
    boost::intrusive_ptr<Message_Shutdown> shutdownMessage = new Message_Shutdown();
    threadHudThread->messageQueue.PushMessage(shutdownMessage);
    threadHudThread->Join();
    delete threadHudThread;
    shutdownMessage.reset();
  }

  scene2D.reset();
  scene3D.reset();

  for (auto* controller : controllers) {
    delete controller;
  }
  controllers.clear();

  TTF_CloseFont(defaultFont);         // todo: better timed closefont?
  TTF_CloseFont(defaultOutlineFont);  // todo: better timed closefont?

  delete db;
  delete config;

  Exit();

  return 0;
}
