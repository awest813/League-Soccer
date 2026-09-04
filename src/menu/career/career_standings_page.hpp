#ifndef CAREER_STANDINGS_PAGE_HPP
#define CAREER_STANDINGS_PAGE_HPP

#include "career_database.hpp"
#include "career_sim.hpp"
#include "utils/gui2/page.hpp"
#include "utils/gui2/widgets/button.hpp"
#include "utils/gui2/widgets/caption.hpp"
#include "utils/gui2/widgets/frame.hpp"
#include "utils/gui2/widgets/grid.hpp"
#include "utils/gui2/windowmanager.hpp"

namespace blunted {

class CareerStandingsPage : public Gui2Page {
public:
  CareerStandingsPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  ~CareerStandingsPage() override;

private:
  void PopulateStandingsGrid();
  void PopulateScorersGrid();
  void GoBack();
  void GoMatchday();
  void GoSeason();

  Gui2Frame* frame;
  Gui2Grid* standingsGrid;
  Gui2Grid* scorersGrid;
  bool m_fromMenu;
};

}  // namespace blunted

#endif  // CAREER_STANDINGS_PAGE_HPP
