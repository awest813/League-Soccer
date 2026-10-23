#include "ownerpages.hpp"

#include <algorithm>

#include "../../data/careerdata.hpp"
#include "../../main.hpp"
#include "../../utils/gui2/widgets/frame.hpp"  // For card layout
#include "../../utils/gui2/widgets/table.hpp"
#include "../pagefactory.hpp"
#include "career_navigation.hpp"
#include "careerpages.hpp"  // For PageIDs
#include "utils/localization.hpp"

namespace {

std::string BuildOwnerTopLine(const CareerSave* save) {
  if (!save)
    return TR("career_nosave_owner");
  return TRF("career_owner_topline",
             {save->name, std::to_string(save->season.currentSeason),
              std::to_string(save->season.currentWeek), std::to_string(save->season.maxWeeks),
              std::to_string(save->boardConfidence), std::to_string(save->reputation)});
}

std::string FormatOwnerMoney(long long amount) {
  const bool negative = amount < 0;
  unsigned long long value =
      negative ? static_cast<unsigned long long>(-amount) : static_cast<unsigned long long>(amount);
  std::string digits = std::to_string(value);
  std::string grouped;
  int count = 0;
  for (int i = static_cast<int>(digits.size()) - 1; i >= 0; --i) {
    if (count > 0 && count % 3 == 0)
      grouped.push_back(',');
    grouped.push_back(digits[static_cast<size_t>(i)]);
    ++count;
  }
  std::reverse(grouped.begin(), grouped.end());
  return std::string("EUR ") + (negative ? "-" : "") + grouped;
}

int GetHubPageID() {
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
  return CareerHubPageID(save);
}

}  // namespace

// ---------------------------------------------------------------------------
// OwnerStadiumPage
// ---------------------------------------------------------------------------

OwnerStadiumPage::OwnerStadiumPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();

  Gui2Frame* root = new Gui2Frame(windowManager, "frame_stad_root", 4, 3, 92, 94, true);
  this->AddView(root);
  root->Show();

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_stadium", 10, 3, 80, 3, TR("career_stadium_title"));
  root->AddView(title);
  title->Show();

  if (save) {
    Gui2Caption* topLine =
        new Gui2Caption(windowManager, "caption_stad_top", 6, 8, 82, 2, BuildOwnerTopLine(save));
    root->AddView(topLine);
    topLine->Show();

    auto& stad = save->stadium;

    Gui2Frame* infoFrame = new Gui2Frame(windowManager, "frame_stad_info", 4, 12, 84, 10, true);

    std::string info1 = TRF("career_stadium_info1",
                            {stad.name, std::to_string(stad.capacity),
                             std::to_string(stad.condition), std::to_string(stad.fanSatisfaction)});
    Gui2Caption* stadInfo = new Gui2Caption(windowManager, "caption_stad_info", 2, 2, 80, 2, info1);
    infoFrame->AddView(stadInfo);
    stadInfo->Show();

    std::string info2 = TRF("career_stadium_info2", {std::to_string(stad.maintenanceCost),
                                                     std::to_string(stad.matchDayRevenue),
                                                     std::to_string(save->finances.netWorth)});
    Gui2Caption* costInfo = new Gui2Caption(windowManager, "caption_stad_cost", 2, 5, 80, 2, info2);
    infoFrame->AddView(costInfo);
    costInfo->Show();

    root->AddView(infoFrame);
    infoFrame->Show();

    int nextY = 24;

    if (!stad.upgrades.empty()) {
      int activeHeight = 4 + static_cast<int>(stad.upgrades.size()) * 3;
      Gui2Frame* activeFrame =
          new Gui2Frame(windowManager, "frame_stad_active", 4, nextY, 84, activeHeight, true);

      Gui2Caption* activeTitle = new Gui2Caption(windowManager, "caption_stad_active", 2, 1, 80, 2,
                                                 TR("career_stadium_active"));
      activeFrame->AddView(activeTitle);
      activeTitle->Show();

      int yOff = 4;
      for (const auto& u : stad.upgrades) {
        std::string status =
            u.isComplete()
                ? "[" + TR("career_complete") + "]"
                : "[" + TRF("career_seasons_left", {std::to_string(u.seasonsRemaining)}) + "]";
        Gui2Caption* entry = new Gui2Caption(
            windowManager, "caption_upg_" + u.name, 2, yOff, 80, 2,
            "  " + u.name + " " + status + " +" + std::to_string(u.capacityIncrease) + " " +
                TR("career_seats") + ", +" + TR("career_eur") + " " +
                std::to_string(u.revenueBonus) + "/" + TR("career_season"));
        activeFrame->AddView(entry);
        entry->Show();
        yOff += 3;
      }
      root->AddView(activeFrame);
      activeFrame->Show();
      nextY += activeHeight + 1;
    }

    Gui2Frame* availFrame =
        new Gui2Frame(windowManager, "frame_stad_avail", 4, nextY, 84, 86 - nextY, true);

    Gui2Caption* availTitle = new Gui2Caption(windowManager, "caption_stad_avail", 2, 1, 80, 2,
                                              TR("career_stadium_available"));
    availFrame->AddView(availTitle);
    availTitle->Show();

    Gui2Grid* ugGrid = new Gui2Grid(windowManager, "stad_ug_grid", 2, 4, 80, 80 - nextY);
    int row = 0;

    Gui2Button* btnRename = new Gui2Button(windowManager, "btn_stad_rename", 0, 0, 80, 2.5,
                                           TRF("career_stadium_rename", {save->name}));
    btnRename->sig_OnClick.connect(
        [this, save](...) { RenameStadium(save->name + " Elite Park"); });
    ugGrid->AddView(btnRename, row++, 0);

    for (int i = 0; i < static_cast<int>(stad.availableUpgrades.size()); i++) {
      const auto& u = stad.availableUpgrades[i];
      std::string label = TRF("career_stadium_upgrade_row",
                              {u.name, std::to_string(u.cost), std::to_string(u.capacityIncrease),
                               std::to_string(u.buildTimeSeasons)});
      Gui2Button* btn =
          new Gui2Button(windowManager, "btn_upg_" + std::to_string(i), 0, 0, 80, 2.5, label);
      btn->sig_OnClick.connect([this, i](...) { UpgradeStadium(i); });
      ugGrid->AddView(btn, row++, 0);
    }

    int repairPoints = 10;
    long long repairCost = 50000LL * std::max(1, repairPoints / 10);
    Gui2Button* btnRepair = new Gui2Button(
        windowManager, "btn_stad_repair", 0, 0, 80, 2.5,
        TRF("career_stadium_repair", {std::to_string(repairPoints), std::to_string(repairCost)}));
    btnRepair->sig_OnClick.connect([this, repairPoints](...) { RepairStadium(repairPoints); });
    ugGrid->AddView(btnRepair, row++, 0);

    ugGrid->UpdateLayout(0.5);
    availFrame->AddView(ugGrid);
    ugGrid->Show();

    root->AddView(availFrame);
    availFrame->Show();
  }

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_stad_back", 26, 88, 40, 3.6f, TR("career_back_owner_hub"));
  btnBack->sig_OnClick.connect([this](...) { CreatePage(e_PageID_OwnerHub); });
  root->AddView(btnBack);
  btnBack->Show();

  btnBack->SetFocus();
  this->Show();
}

OwnerStadiumPage::~OwnerStadiumPage() {}

void OwnerStadiumPage::UpgradeStadium(int index) {
  CareerDatabase::GetInstance().UpgradeStadium(index);
  CreatePage(e_PageID_OwnerStadium);
}

void OwnerStadiumPage::RepairStadium(int points) {
  CareerDatabase::GetInstance().RepairStadium(points);
  CreatePage(e_PageID_OwnerStadium);
}

void OwnerStadiumPage::RenameStadium(const std::string& newName) {
  CareerDatabase::GetInstance().RenameStadium(newName);
  CreatePage(e_PageID_OwnerStadium);
}

// ---------------------------------------------------------------------------
// OwnerFinancesPage
// ---------------------------------------------------------------------------

OwnerFinancesPage::OwnerFinancesPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();

  Gui2Frame* root = new Gui2Frame(windowManager, "frame_fin_root", 4, 3, 92, 94, true);
  this->AddView(root);
  root->Show();

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_finances", 10, 3, 80, 3, TR("career_finances_title"));
  root->AddView(title);
  title->Show();

  Gui2Caption* topLine =
      new Gui2Caption(windowManager, "caption_fin_topline", 6, 8, 82, 2, BuildOwnerTopLine(save));
  root->AddView(topLine);
  topLine->Show();

  if (save) {
    auto& fin = save->finances;

    Gui2Frame* overviewFrame =
        new Gui2Frame(windowManager, "frame_fin_overview", 4, 12, 84, 8, true);

    std::string healthStr = TRF(
        "career_fin_health",
        {CareerDatabase::GetInstance().GetFinancialHealthString(), std::to_string(fin.debtLevel)});
    Gui2Caption* healthCap = new Gui2Caption(windowManager, "fin_health", 2, 2, 80, 2, healthStr);
    overviewFrame->AddView(healthCap);
    healthCap->Show();

    std::string netWorthStr = TRF("career_net_worth", {std::to_string(fin.netWorth)});
    Gui2Caption* nwCap = new Gui2Caption(windowManager, "fin_nw", 2, 4, 80, 2, netWorthStr);
    overviewFrame->AddView(nwCap);
    nwCap->Show();

    root->AddView(overviewFrame);
    overviewFrame->Show();

    Gui2Frame* revFrame = new Gui2Frame(windowManager, "frame_fin_rev", 4, 22, 41, 16, true);

    Gui2Caption* revTitle =
        new Gui2Caption(windowManager, "fin_revtitle", 2, 1, 37, 2, TR("career_revenue"));
    revFrame->AddView(revTitle);
    revTitle->Show();

    std::string revBody = TRF(
        "career_rev_body", {std::to_string(fin.matchDayIncome), std::to_string(fin.sponsorIncome),
                            std::to_string(fin.merchandiseIncome), std::to_string(fin.tvRevenue),
                            std::to_string(fin.transferIncome), std::to_string(fin.totalRevenue)});
    Gui2Caption* revText = new Gui2Caption(windowManager, "fin_revbody", 2, 4, 37, 10, revBody);
    revFrame->AddView(revText);
    revText->Show();

    root->AddView(revFrame);
    revFrame->Show();

    Gui2Frame* expFrame = new Gui2Frame(windowManager, "frame_fin_exp", 49, 22, 41, 16, true);

    Gui2Caption* expTitle =
        new Gui2Caption(windowManager, "fin_exptitle", 2, 1, 37, 2, TR("career_expenses"));
    expFrame->AddView(expTitle);
    expTitle->Show();

    std::string expBody = TRF(
        "career_exp_body", {std::to_string(fin.playerWages), std::to_string(fin.staffWages),
                            std::to_string(fin.stadiumCosts), std::to_string(fin.transferSpending),
                            std::to_string(fin.totalExpenses)});
    Gui2Caption* expText = new Gui2Caption(windowManager, "fin_expbody", 2, 4, 37, 10, expBody);
    expFrame->AddView(expText);
    expText->Show();

    root->AddView(expFrame);
    expFrame->Show();

    Gui2Frame* profitFrame = new Gui2Frame(windowManager, "frame_fin_profit", 4, 40, 84, 8, true);

    std::string profitStr =
        TRF("career_net_profit", {std::to_string(CareerDatabase::GetInstance().GetSeasonProfit())});
    Gui2Caption* profitCap = new Gui2Caption(windowManager, "fin_profit", 2, 2, 80, 2, profitStr);
    profitFrame->AddView(profitCap);
    profitCap->Show();

    std::string tktStr = TRF("career_ticket_line", {std::to_string(fin.ticketPrice),
                                                    std::to_string(fin.seasonTicketHolders)});
    Gui2Caption* tktCap = new Gui2Caption(windowManager, "fin_tp", 2, 4, 80, 2, tktStr);
    profitFrame->AddView(tktCap);
    profitFrame->Show();

    root->AddView(profitFrame);
    profitFrame->Show();

    Gui2Frame* actFrame = new Gui2Frame(windowManager, "frame_fin_act", 4, 50, 84, 12, true);
    Gui2Grid* actGrid = new Gui2Grid(windowManager, "fin_act_grid", 2, 2, 80, 8);
    int row = 0;

    int currentPrice = fin.ticketPrice;

    Gui2Button* btnTicketUp = new Gui2Button(windowManager, "btn_ticket_up", 0, 0, 40, 2.5,
                                             TRF("career_ticket_inc", {std::to_string(10)}));
    btnTicketUp->sig_OnClick.connect(
        [this, currentPrice](...) { SetTicketPrice(currentPrice + 10); });
    actGrid->AddView(btnTicketUp, row, 0);

    Gui2Button* btnTicketDown = new Gui2Button(windowManager, "btn_ticket_down", 0, 0, 40, 2.5,
                                               TRF("career_ticket_dec", {std::to_string(10)}));
    btnTicketDown->sig_OnClick.connect(
        [this, currentPrice](...) { SetTicketPrice(currentPrice - 10); });
    actGrid->AddView(btnTicketDown, row++, 1);

    Gui2Button* btnFanInvest =
        new Gui2Button(windowManager, "btn_fan_invest", 0, 0, 40, 2.5, TR("career_invest_fan"));
    btnFanInvest->sig_OnClick.connect([this](...) { InvestFanBase(); });
    actGrid->AddView(btnFanInvest, row, 0);

    Gui2Button* btnPrestige = new Gui2Button(windowManager, "btn_prestige_invest", 0, 0, 40, 2.5,
                                             TR("career_invest_prestige"));
    btnPrestige->sig_OnClick.connect([this](...) { InvestPrestige(); });
    actGrid->AddView(btnPrestige, row++, 1);

    actGrid->UpdateLayout(0.5);
    actFrame->AddView(actGrid);
    actGrid->Show();

    root->AddView(actFrame);
    actFrame->Show();
  }

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_fin_back", 26, 88, 40, 3.6f, TR("career_back_owner_hub"));
  btnBack->sig_OnClick.connect([this](...) { CreatePage(e_PageID_OwnerHub); });
  root->AddView(btnBack);
  btnBack->Show();

  btnBack->SetFocus();
  this->Show();
}

OwnerFinancesPage::~OwnerFinancesPage() {}

void OwnerFinancesPage::SetTicketPrice(int price) {
  CareerDatabase::GetInstance().SetTicketPrice(price);
  CreatePage(e_PageID_OwnerFinances);
}

void OwnerFinancesPage::InvestFanBase() {
  CareerDatabase::GetInstance().InvestInFanBase(2000000);
  CreatePage(e_PageID_OwnerFinances);
}

void OwnerFinancesPage::InvestPrestige() {
  CareerDatabase::GetInstance().InvestInPrestige(3000000);
  CreatePage(e_PageID_OwnerFinances);
}

// ---------------------------------------------------------------------------
// OwnerStaffPage
// ---------------------------------------------------------------------------

OwnerStaffPage::OwnerStaffPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();

  Gui2Frame* root = new Gui2Frame(windowManager, "frame_staff_root", 4, 3, 92, 94, true);
  this->AddView(root);
  root->Show();

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_staff", 10, 3, 80, 3, TR("career_staff_title"));
  root->AddView(title);
  title->Show();

  Gui2Caption* topLine =
      new Gui2Caption(windowManager, "caption_staff_topline", 6, 8, 82, 2, BuildOwnerTopLine(save));
  root->AddView(topLine);
  topLine->Show();

  if (save) {
    Gui2Frame* overviewFrame =
        new Gui2Frame(windowManager, "frame_staff_overview", 4, 12, 84, 10, true);
    std::string overview = TRF("career_staff_overview", {std::to_string(save->staff.size()),
                                                         std::to_string(save->finances.staffWages),
                                                         std::to_string(save->boardConfidence)});
    Gui2Caption* overviewCap =
        new Gui2Caption(windowManager, "caption_staff_overview", 2, 2, 80, 4, overview);
    overviewFrame->AddView(overviewCap);
    overviewCap->Show();
    root->AddView(overviewFrame);
    overviewFrame->Show();

    Gui2Frame* staffFrame = new Gui2Frame(windowManager, "frame_staff", 4, 24, 84, 48, true);

    using Align = Gui2Table::Align;
    Gui2Caption* hint =
        new Gui2Caption(windowManager, "caption_staff_hdr", 2, 1, 80, 2.4, TR("career_staff_hint"));
    staffFrame->AddView(hint);
    hint->Show();

    auto* table = new Gui2Table(windowManager, "table_staff", 2, 4.5f,
                                {{TR("career_col_name"), 24, Align::Left},
                                 {TR("career_col_role"), 18, Align::Left},
                                 {TR("career_col_skill"), 8, Align::Center},
                                 {TR("career_col_salary"), 14, Align::Right},
                                 {TR("career_col_contract"), 8, Align::Center},
                                 {TR("career_col_morale"), 8, Align::Center}},
                                3.4f, 11);
    std::vector<std::string> staffNames;
    for (const auto& s : save->staff) {
      Gui2Table::Row row;
      row.cells = {s.name,
                   s.role,
                   std::to_string(s.skill),
                   FormatOwnerMoney(s.salary),
                   CareerCommon::FormatYears(s.contractYearsRemaining),
                   std::to_string(s.morale) + "%"};
      // Unhappy staff stand out: they are the first candidates to replace.
      if (s.morale < 40) {
        row.hasZone = true;
        row.zoneColor = Vector3(220, 70, 60);
      }
      table->AddRow(row);
      staffNames.push_back(s.name);
    }
    table->SetSelectable(!staffNames.empty());
    table->sig_OnActivate.connect([this, staffNames](int index) {
      if (index >= 0 && index < static_cast<int>(staffNames.size()))
        FireStaff(staffNames[index]);
    });
    table->Build();
    staffFrame->AddView(table);
    table->Show();
    m_staffTable = table;

    root->AddView(staffFrame);
    staffFrame->Show();

    Gui2Frame* notesFrame = new Gui2Frame(windowManager, "frame_staff_notes", 4, 74, 84, 10, true);
    Gui2Caption* notesTitle = new Gui2Caption(windowManager, "caption_staff_notes_title", 2, 1, 80,
                                              2, TR("career_staff_notes_title"));
    notesFrame->AddView(notesTitle);
    notesTitle->Show();
    Gui2Caption* notesBody = new Gui2Caption(windowManager, "caption_staff_notes_body", 2, 4, 80, 4,
                                             TR("career_staff_notes_body"));
    notesFrame->AddView(notesBody);
    notesBody->Show();
    root->AddView(notesFrame);
    notesFrame->Show();
  }

  Gui2Button* btnHire = new Gui2Button(windowManager, "btn_staff_hire", 8, 87, 34, 3.6f,
                                       TR("career_browse_candidates"));
  btnHire->sig_OnClick.connect([this](...) { GoHirePage(); });
  root->AddView(btnHire);
  btnHire->Show();

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_staff_back", 50, 87, 34, 3.6f, TR("career_back_hub"));
  btnBack->sig_OnClick.connect([this](...) { CreatePage(GetHubPageID()); });
  root->AddView(btnBack);
  btnBack->Show();

  if (m_staffTable && m_staffTable->GetRowCount() > 0)
    m_staffTable->SetFocus();
  else
    btnHire->SetFocus();
  this->Show();
}

OwnerStaffPage::~OwnerStaffPage() {}

void OwnerStaffPage::FireStaff(const std::string& name) {
  CareerDatabase::GetInstance().FireStaff(name);
  CreatePage(e_PageID_OwnerStaff);
}

void OwnerStaffPage::GoHirePage() {
  CreatePage(e_PageID_OwnerStaffHire);
}

// ---------------------------------------------------------------------------
// OwnerStaffHirePage
// ---------------------------------------------------------------------------

OwnerStaffHirePage::OwnerStaffHirePage(Gui2WindowManager* windowManager,
                                       const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  CareerDatabase::GetInstance().GenerateStaffCandidates(m_candidates);
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();

  Gui2Frame* root = new Gui2Frame(windowManager, "frame_hire_root", 4, 3, 92, 94, true);
  this->AddView(root);
  root->Show();

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_hiretitle", 10, 3, 80, 3,
                                       TR("career_staff_candidates"));
  root->AddView(title);
  title->Show();

  Gui2Caption* topLine =
      new Gui2Caption(windowManager, "caption_hire_topline", 6, 8, 82, 2, BuildOwnerTopLine(save));
  root->AddView(topLine);
  topLine->Show();

  Gui2Frame* marketFrame = new Gui2Frame(windowManager, "frame_hire_market", 4, 12, 84, 10, true);
  Gui2Caption* marketBody = new Gui2Caption(windowManager, "caption_hire_market", 2, 2, 80, 4,
                                            TR("career_hire_market_body"));
  marketFrame->AddView(marketBody);
  marketBody->Show();
  root->AddView(marketFrame);
  marketFrame->Show();

  Gui2Frame* hireFrame = new Gui2Frame(windowManager, "frame_hire", 4, 24, 84, 58, true);

  using Align = Gui2Table::Align;
  Gui2Caption* hint =
      new Gui2Caption(windowManager, "caption_hire_hdr", 2, 1, 80, 2.4, TR("career_hire_hint"));
  hireFrame->AddView(hint);
  hint->Show();

  auto* table = new Gui2Table(windowManager, "table_hire", 2, 4.5f,
                              {{TR("career_col_name"), 26, Align::Left},
                               {TR("career_col_role"), 20, Align::Left},
                               {TR("career_col_skill"), 10, Align::Center},
                               {TR("career_col_salary"), 14, Align::Right},
                               {TR("career_col_contract"), 10, Align::Center}},
                              3.4f, 14);
  for (const auto& c : m_candidates) {
    Gui2Table::Row row;
    row.cells = {c.name, c.role, std::to_string(c.skill), FormatOwnerMoney(c.salary),
                 CareerCommon::FormatYears(c.contractYearsRemaining)};
    if (c.skill >= 75) {
      row.hasZone = true;
      row.zoneColor = Vector3(80, 200, 110);
    }
    table->AddRow(row);
  }
  table->SetSelectable(!m_candidates.empty());
  table->sig_OnActivate.connect([this](int index) { HireCandidate(index); });
  table->Build();
  hireFrame->AddView(table);
  table->Show();

  root->AddView(hireFrame);
  hireFrame->Show();

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_hire_back", 30, 86, 32, 3.6f, TR("career_back_staff"));
  btnBack->sig_OnClick.connect([this](...) { CreatePage(e_PageID_OwnerStaff); });
  root->AddView(btnBack);
  btnBack->Show();

  if (table->GetRowCount() > 0)
    table->SetFocus();
  else
    btnBack->SetFocus();
  this->Show();
}

OwnerStaffHirePage::~OwnerStaffHirePage() {}

void OwnerStaffHirePage::HireCandidate(int index) {
  if (index >= 0 && index < static_cast<int>(m_candidates.size())) {
    CareerDatabase::GetInstance().HireStaff(m_candidates[index]);
  }
  CreatePage(e_PageID_OwnerStaff);
}

// ---------------------------------------------------------------------------
// OwnerSponsorsPage
// ---------------------------------------------------------------------------

OwnerSponsorsPage::OwnerSponsorsPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();

  Gui2Frame* root = new Gui2Frame(windowManager, "frame_sponsors_root", 4, 3, 92, 94, true);
  this->AddView(root);
  root->Show();

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_sponsors", 10, 3, 80, 3, TR("career_sponsors_title"));
  root->AddView(title);
  title->Show();

  Gui2Caption* topLine = new Gui2Caption(windowManager, "caption_sponsors_topline", 6, 8, 82, 2,
                                         BuildOwnerTopLine(save));
  root->AddView(topLine);
  topLine->Show();

  if (save) {
    Gui2Frame* summaryFrame = new Gui2Frame(windowManager, "frame_sp_summary", 4, 12, 84, 10, true);
    std::string summary =
        TRF("career_sponsors_summary", {std::to_string(save->activeSponsors.size()),
                                        std::to_string(save->availableSponsorOffers.size()),
                                        std::to_string(save->finances.sponsorIncome)});
    Gui2Caption* summaryCap =
        new Gui2Caption(windowManager, "caption_sp_summary", 2, 2, 80, 4, summary);
    summaryFrame->AddView(summaryCap);
    summaryCap->Show();
    root->AddView(summaryFrame);
    summaryFrame->Show();

    int nextY = 24;

    using Align = Gui2Table::Align;
    const std::vector<Gui2Table::Column> columns = {
        {TR("career_col_sponsor"), 30, Align::Left},
        {TR("career_col_type"), 18, Align::Left},
        {TR("career_col_revenue"), 16, Align::Right},
        {TR("career_col_years_left"), 16, Align::Center}};

    if (!save->activeSponsors.empty()) {
      const int visible = std::min(4, static_cast<int>(save->activeSponsors.size()));
      const int sHeight = static_cast<int>(8 + (visible + 1) * 3.2f);
      Gui2Frame* activeFrame =
          new Gui2Frame(windowManager, "frame_sp_active", 4, nextY, 84, sHeight, true);

      Gui2Caption* activeTitle = new Gui2Caption(windowManager, "caption_sp_active", 2, 1, 80, 2.4,
                                                 TR("career_sponsors_active_hint"));
      activeFrame->AddView(activeTitle);
      activeTitle->Show();

      auto* table = new Gui2Table(windowManager, "table_sp_active", 2, 4.5f, columns, 3.2f, 4);
      std::vector<std::string> sponsorNames;
      for (const auto& sp : save->activeSponsors) {
        Gui2Table::Row row;
        row.cells = {sp.sponsorName, sp.type, FormatOwnerMoney(sp.annualRevenue),
                     CareerCommon::FormatYears(sp.yearsRemaining)};
        // Deals in their final year need renewing or replacing.
        if (sp.yearsRemaining <= 1) {
          row.hasZone = true;
          row.zoneColor = Vector3(230, 170, 50);
        }
        table->AddRow(row);
        sponsorNames.push_back(sp.sponsorName);
      }
      table->SetSelectable(true);
      table->sig_OnActivate.connect([this, sponsorNames](int index) {
        if (index >= 0 && index < static_cast<int>(sponsorNames.size()))
          TerminateDeal(sponsorNames[index]);
      });
      table->Build();
      activeFrame->AddView(table);
      table->Show();
      if (!m_firstTable)
        m_firstTable = table;

      root->AddView(activeFrame);
      activeFrame->Show();

      nextY += sHeight + 1;
    }

    Gui2Frame* offersFrame =
        new Gui2Frame(windowManager, "frame_sp_offers", 4, nextY, 84, 86 - nextY, true);

    Gui2Caption* offersTitle = new Gui2Caption(windowManager, "caption_sp_offers", 2, 1, 80, 2.4,
                                               TR("career_sponsors_offers_hint"));
    offersFrame->AddView(offersTitle);
    offersTitle->Show();

    if (save->availableSponsorOffers.empty()) {
      Gui2Caption* noOffers = new Gui2Caption(windowManager, "caption_sp_none", 2, 4.5, 80, 2.4,
                                              TR("career_sponsors_none"));
      offersFrame->AddView(noOffers);
      noOffers->Show();
    } else {
      std::vector<Gui2Table::Column> offerColumns = columns;
      offerColumns[0].width_percent = 26;
      offerColumns[1].width_percent = 16;
      offerColumns[2].width_percent = 14;
      offerColumns[3].width_percent = 12;
      offerColumns.push_back({TR("career_col_rep"), 12, Align::Center});
      const int rows = std::max(1, static_cast<int>((86 - nextY - 6) / 3.2f) - 1);
      auto* table =
          new Gui2Table(windowManager, "table_sp_offers", 2, 4.5f, offerColumns, 3.2f, rows);
      for (const auto& sp : save->availableSponsorOffers) {
        Gui2Table::Row row;
        row.cells = {sp.sponsorName, sp.type, FormatOwnerMoney(sp.annualRevenue),
                     CareerCommon::FormatYears(sp.yearsRemaining),
                     std::to_string(sp.reputationRequirement)};
        // Grey out offers the club's reputation cannot yet unlock.
        if (sp.reputationRequirement > save->reputation) {
          row.hasTextColor = true;
          row.textColor = Vector3(140, 140, 150);
        } else {
          row.hasZone = true;
          row.zoneColor = Vector3(80, 200, 110);
        }
        table->AddRow(row);
      }
      table->SetSelectable(true);
      table->sig_OnActivate.connect([this](int index) { AcceptDeal(index); });
      table->Build();
      offersFrame->AddView(table);
      table->Show();
      if (!m_firstTable)
        m_firstTable = table;
    }

    root->AddView(offersFrame);
    offersFrame->Show();
  }

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_sp_back", 30, 88, 32, 3.6f, TR("career_back_owner_hub"));
  btnBack->sig_OnClick.connect([this](...) { CreatePage(e_PageID_OwnerHub); });
  root->AddView(btnBack);
  btnBack->Show();

  if (m_firstTable)
    m_firstTable->SetFocus();
  else
    btnBack->SetFocus();
  this->Show();
}

OwnerSponsorsPage::~OwnerSponsorsPage() {}

void OwnerSponsorsPage::AcceptDeal(int index) {
  CareerDatabase::GetInstance().AcceptSponsorDeal(index);
  CreatePage(e_PageID_OwnerSponsors);
}

void OwnerSponsorsPage::TerminateDeal(const std::string& sponsorName) {
  CareerDatabase::GetInstance().TerminateSponsorDeal(sponsorName);
  CreatePage(e_PageID_OwnerSponsors);
}

// ---------------------------------------------------------------------------
// OwnerBoardRoomPage
// ---------------------------------------------------------------------------

OwnerBoardRoomPage::OwnerBoardRoomPage(Gui2WindowManager* windowManager,
                                       const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();

  Gui2Frame* root = new Gui2Frame(windowManager, "frame_board_root", 4, 3, 92, 94, true);
  this->AddView(root);
  root->Show();

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_boardroom", 10, 3, 80, 3,
                                       TR("career_boardroom_title"));
  root->AddView(title);
  title->Show();

  Gui2Caption* topLine =
      new Gui2Caption(windowManager, "caption_board_topline", 6, 8, 82, 2, BuildOwnerTopLine(save));
  root->AddView(topLine);
  topLine->Show();

  if (save) {
    Gui2Frame* brFrame = new Gui2Frame(windowManager, "frame_br_overview", 4, 12, 84, 10, true);

    std::string confStr = TRF(
        "career_board_conf",
        {std::to_string(save->boardConfidence), CareerDatabase::GetInstance().GetReputationStatus(),
         std::to_string(save->reputation), std::to_string(save->clubPrestige)});
    Gui2Caption* confLabel =
        new Gui2Caption(windowManager, "caption_br_conf", 2, 2, 80, 4, confStr);
    brFrame->AddView(confLabel);
    confLabel->Show();

    root->AddView(brFrame);
    brFrame->Show();

    int objHeight = 6 + std::max(1, static_cast<int>(save->boardObjectives.size())) * 3;
    Gui2Frame* objFrame = new Gui2Frame(windowManager, "frame_br_obj", 4, 24, 84, objHeight, true);

    Gui2Caption* objTitle = new Gui2Caption(windowManager, "caption_br_objtitle", 2, 1, 80, 2,
                                            TR("career_board_objectives"));
    objFrame->AddView(objTitle);
    objTitle->Show();

    int subY = 4;
    for (int i = 0; i < static_cast<int>(save->boardObjectives.size()); i++) {
      const auto& obj = save->boardObjectives[i];
      std::string status =
          obj.completed ? "[" + TR("career_complete") + "]" : "[" + TR("career_in_progress") + "]";
      std::string line = "  " + status + " " + obj.description + " " +
                         TRF("career_board_objective", {std::to_string(obj.reputationReward),
                                                        std::to_string(obj.confidencePenalty)});
      Gui2Caption* objLine = new Gui2Caption(windowManager, "caption_br_obj_" + std::to_string(i),
                                             2, subY, 78, 3, line);
      objFrame->AddView(objLine);
      objLine->Show();
      subY += 3;
    }
    root->AddView(objFrame);
    objFrame->Show();

    int yPos = 24 + objHeight + 2;
    Gui2Frame* evtFrame =
        new Gui2Frame(windowManager, "frame_br_evt", 4, yPos, 84, 88 - yPos, true);

    Gui2Caption* evtTitle = new Gui2Caption(windowManager, "caption_br_evttitle", 2, 1, 80, 2,
                                            TR("career_board_events"));
    evtFrame->AddView(evtTitle);
    evtTitle->Show();

    subY = 4;
    auto events = CareerDatabase::GetInstance().GetRecentEvents(8);
    for (int i = 0; i < static_cast<int>(events.size()); i++) {
      const auto& evt = events[i];
      std::string prefix = evt.isMajor ? "*** " : "  ";
      std::string repStr =
          evt.reputationImpact > 0   ? " (+" + std::to_string(evt.reputationImpact) + ")"
          : evt.reputationImpact < 0 ? " (" + std::to_string(evt.reputationImpact) + ")"
                                     : "";
      Gui2Caption* evtLine = new Gui2Caption(windowManager, "caption_br_evt_" + std::to_string(i),
                                             2, subY, 78, 2, prefix + evt.description + repStr);
      evtFrame->AddView(evtLine);
      evtLine->Show();
      subY += 2;
    }

    root->AddView(evtFrame);
    evtFrame->Show();
  }

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_br_back", 30, 88, 32, 3.6f, TR("career_back_owner_hub"));
  btnBack->sig_OnClick.connect([this](...) { CreatePage(e_PageID_OwnerHub); });
  root->AddView(btnBack);
  btnBack->Show();

  btnBack->SetFocus();
  this->Show();
}

OwnerBoardRoomPage::~OwnerBoardRoomPage() {}
