#pragma once

#include <string>
#include <vector>

#include "../../data/careerdata.hpp"
#include "../../utils/gui2/page.hpp"
#include "../../utils/gui2/widgets/button.hpp"
#include "../../utils/gui2/widgets/caption.hpp"
#include "../../utils/gui2/widgets/editline.hpp"
#include "../../utils/gui2/widgets/grid.hpp"
#include "../../utils/gui2/widgets/pulldown.hpp"
#include "../../utils/gui2/widgets/table.hpp"
#include "../../utils/gui2/windowmanager.hpp"
#include "career_database.hpp"

using namespace blunted;

// ---------------------------------------------------------------------------
// Owner Mode Pages
// ---------------------------------------------------------------------------

// Stadium management
class OwnerStadiumPage : public Gui2Page {
public:
  OwnerStadiumPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~OwnerStadiumPage();

protected:
  void UpgradeStadium(int index);
  void RepairStadium(int points = 10);
  void RenameStadium(const std::string& newName);
};

// Financial overview
class OwnerFinancesPage : public Gui2Page {
public:
  OwnerFinancesPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~OwnerFinancesPage();

protected:
  void SetTicketPrice(int price);
  void InvestFanBase();
  void InvestPrestige();
};

// Staff hiring/firing
class OwnerStaffPage : public Gui2Page {
public:
  OwnerStaffPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~OwnerStaffPage();

protected:
  void FireStaff(const std::string& name);
  void GoHirePage();
  blunted::Gui2Table* m_staffTable = nullptr;
};

// Staff hiring candidates page
class OwnerStaffHirePage : public Gui2Page {
public:
  OwnerStaffHirePage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~OwnerStaffHirePage();

protected:
  void HireCandidate(int index);
  std::vector<blunted::StaffMember> m_candidates;
};

// Sponsor deals
class OwnerSponsorsPage : public Gui2Page {
public:
  OwnerSponsorsPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~OwnerSponsorsPage();

protected:
  void AcceptDeal(int index);
  void TerminateDeal(const std::string& sponsorName);
  blunted::Gui2Table* m_firstTable = nullptr;  // focused on entry
};

// Board room / objectives
class OwnerBoardRoomPage : public Gui2Page {
public:
  OwnerBoardRoomPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~OwnerBoardRoomPage();
};
