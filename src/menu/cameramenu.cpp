// written by bastiaan konings schuiling 2008 - 2015
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not
// be used for anything important. i do not offer support, so don't ask. to be used for inspiration
// :)

#include "cameramenu.hpp"

#include "../main.hpp"

using namespace blunted;

CameraPage::CameraPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  sliderZoom = new Gui2Slider(windowManager, "camzoomslider", 0, 0, 30, 6, "zoom");
  sliderHeight = new Gui2Slider(windowManager, "camheightslider", 0, 0, 30, 6, "height");
  sliderFOV = new Gui2Slider(windowManager, "camfovslider", 0, 0, 30, 6, "fov (perspective)");
  sliderAngleFactor =
      new Gui2Slider(windowManager, "camangleslider", 0, 0, 30, 6, "horizontal angle");
  sliderZoom->AddHelperValue(Vector3(80, 80, 250), "default", _default_CameraZoom);
  sliderHeight->AddHelperValue(Vector3(80, 80, 250), "default", _default_CameraHeight);
  sliderFOV->AddHelperValue(Vector3(80, 80, 250), "default", _default_CameraFOV);
  sliderAngleFactor->AddHelperValue(Vector3(80, 80, 250), "default", _default_CameraAngleFactor);

  Gui2Grid* grid = new Gui2Grid(windowManager, "camgrid", 30, 30, 50, 50);

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
      new Gui2Button(windowManager, "cam_preset_standard", 0, 0, 30, 3, "standard (16:9)");
  Gui2Button* buttonPresetWidescreen =
      new Gui2Button(windowManager, "cam_preset_widescreen", 0, 0, 30, 3, "widescreen (16:9)");
  Gui2Button* buttonPresetUltrawide =
      new Gui2Button(windowManager, "cam_preset_ultrawide", 0, 0, 30, 3, "ultrawide (21:9)");

  buttonPresetStandard->sig_OnClick.connect([this](...) { ApplyPreset(0.5f, 0.3f, 0.4f, 0.0f); });
  buttonPresetWidescreen->sig_OnClick.connect([this](...) { ApplyPreset(0.6f, 0.2f, 0.5f, 0.1f); });
  buttonPresetUltrawide->sig_OnClick.connect([this](...) { ApplyPreset(0.7f, 0.15f, 0.6f, 0.2f); });

  Gui2Grid* presetGrid = new Gui2Grid(windowManager, "cam_presetgrid", 30, 58, 50, 20);
  presetGrid->AddView(buttonPresetStandard, 0, 0);
  presetGrid->AddView(buttonPresetWidescreen, 1, 0);
  presetGrid->AddView(buttonPresetUltrawide, 2, 0);
  presetGrid->UpdateLayout(0.5);

  this->AddView(presetGrid);
  presetGrid->Show();

  this->AddView(grid);
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
