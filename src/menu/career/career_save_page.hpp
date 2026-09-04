#pragma once

#include <string>
#include <vector>

#include "career_database.hpp"
#include "utils/gui2/page.hpp"
#include "utils/gui2/widgets/button.hpp"
#include "utils/gui2/widgets/caption.hpp"
#include "utils/gui2/widgets/frame.hpp"
#include "utils/gui2/widgets/grid.hpp"
#include "utils/gui2/windowmanager.hpp"

namespace blunted {

class CareerSavePage : public Gui2Page {
public:
  CareerSavePage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~CareerSavePage();

protected:
  void RefreshSlots();
  void SetFeedback(const std::string& message);
  void OnSaveSlot(int slotIndex);
  void OnLoadSlot(int slotIndex);
  void PerformLoadSlot(int slotIndex);
  void OnDeleteSlot(int slotIndex);
  void GoBack();

private:
  Gui2Frame* frame;
  Gui2Grid* slotsGrid;
  Gui2Caption* feedbackCaption;
  bool m_fromMenu;
};

}  // namespace blunted
