// written by bastiaan konings schuiling 2008 - 2015
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not
// be used for anything important. i do not offer support, so don't ask. to be used for inspiration
// :)

#include "cameramenu.hpp"

#include "../main.hpp"
#include "utils/localization.hpp"

using namespace blunted;

CameraPage::CameraPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  sliderZoom = new Gui2Slider(windowManager, "camzoomslider", 0, 0, 30, 6, "Zoom");
  sliderHeight = new Gui2Slider(windowManager, "camheightslider", 0, 0, 30, 6, "Height");
  sliderFOV = new Gui2Slider(windowManager, "camfovslider", 0, 0, 30, 6, "FOV (perspective)");
  sliderAngleFactor =
      new Gui2Slider(windowManager, "camangleslider", 0, 0, 30, 6, "Horizontal angle");
  sliderZoom->AddHelperValue(Vector3(80, 80, 250), "default", _default_CameraZoom);
  sliderHeight->AddHelperValue(Vector3(80, 80, 250), "default", _default_CameraHeight);
  sliderFOV->AddHelperValue(Vector3(80, 80, 250), "default", _default_CameraFOV);
  sliderAngleFactor->AddHelperValue(Vector3(80, 80, 250), "default", _default_CameraAngleFactor);

  Gui2Frame* frame = new Gui2Frame(windowManager, "camframe", 25, 20, 60, 60, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_camera", 2, 2, 56, 3, "Camera");
  frame->AddView(title);
  title->Show();

  Gui2Grid* grid = new Gui2Grid(windowManager, "camgrid", 2, 6, 56, 28);

  grid->AddView(sliderZoom, 0, 0);
  grid->AddView(sliderHeight, 1, 0);
  grid->AddView(sliderFOV, 2, 0);
  grid->AddView(sliderAngleFactor, 3, 0);

  grid->UpdateLayout(0.5);

  sliderZoom->sig_OnChange.connect([this](...) { UpdateCamera(); });
  sliderHeight->sig_OnChange.connect([this](...) { UpdateCamera(); });
  sliderFOV->sig_OnChange.connect([this](...) { UpdateCamera(); });
  sliderAngleFactor->sig_OnChange.connect([this](...) { UpdateCamera(); });
  this->sig_OnClose.connect([this](...) { OnClose(); });

  Gui2Button* buttonPresetStandard =
      new Gui2Button(windowManager, "cam_preset_standard", 0, 0, 30, 3, "Standard (16:9)");
  Gui2Button* buttonPresetWidescreen =
      new Gui2Button(windowManager, "cam_preset_widescreen", 0, 0, 30, 3, "Widescreen (16:9)");
  Gui2Button* buttonPresetUltrawide =
      new Gui2Button(windowManager, "cam_preset_ultrawide", 0, 0, 30, 3, "Ultrawide (21:9)");
  Gui2Button* backButton = new Gui2Button(windowManager, "cam_button_back", 0, 0, 30, 3,
                                          Localization::GetInstance().Translate("action_back"));
  backButton->sig_OnClick.connect([this](...) { GoBack(); });

  buttonPresetStandard->sig_OnClick.connect([this](...) { ApplyPreset(0.5f, 0.3f, 0.4f, 0.0f); });
  buttonPresetWidescreen->sig_OnClick.connect([this](...) { ApplyPreset(0.6f, 0.2f, 0.5f, 0.1f); });
  buttonPresetUltrawide->sig_OnClick.connect([this](...) { ApplyPreset(0.7f, 0.15f, 0.6f, 0.2f); });

  Gui2Grid* presetGrid = new Gui2Grid(windowManager, "cam_presetgrid", 2, 36, 56, 22);
  presetGrid->AddView(buttonPresetStandard, 0, 0);
  presetGrid->AddView(buttonPresetWidescreen, 1, 0);
  presetGrid->AddView(buttonPresetUltrawide, 2, 0);
  presetGrid->AddView(backButton, 3, 0);
  presetGrid->UpdateLayout(0.5);

  frame->AddView(presetGrid);
  presetGrid->Show();

  frame->AddView(grid);
  grid->Show();

  sliderZoom->SetFocus();

  float zoom, height, fov, angleFactor;
  GetGameTask()->GetMatch()->GetCameraParams(zoom, height, fov, angleFactor);

  sliderZoom->SetValue(zoom);
  sliderHeight->SetValue(height);
  sliderFOV->SetValue(fov);
  sliderAngleFactor->SetValue(angleFactor);

  this->Show();
}

CameraPage::~CameraPage() {}

void CameraPage::ApplyPreset(float zoom, float height, float fov, float angleFactor) {
  sliderZoom->SetValue(zoom);
  sliderHeight->SetValue(height);
  sliderFOV->SetValue(fov);
  sliderAngleFactor->SetValue(angleFactor);
  UpdateCamera();
}

void CameraPage::OnClose() {
  if (Verbose())
    printf("saving camera settings\n");
  GetConfiguration()->SaveFile(GetConfigFilename());
}

void CameraPage::UpdateCamera() {
  GetConfiguration()->Set("camera_zoom", sliderZoom->GetValue());
  GetConfiguration()->Set("camera_height", sliderHeight->GetValue());
  GetConfiguration()->Set("camera_fov", sliderFOV->GetValue());
  GetConfiguration()->Set("camera_anglefactor", sliderAngleFactor->GetValue());
  GetGameTask()->GetMatch()->SetCameraParams(sliderZoom->GetValue(), sliderHeight->GetValue(),
                                             sliderFOV->GetValue(), sliderAngleFactor->GetValue());
}
