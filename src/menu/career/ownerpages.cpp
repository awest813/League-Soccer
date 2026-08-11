#include "ownerpages.hpp"

#include <algorithm>

#include "../../data/careerdata.hpp"
#include "../../main.hpp"
#include "../../utils/gui2/widgets/frame.hpp"  // For card layout
#include "../pagefactory.hpp"
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

std::string BuildOwnerFinanceLine(const CareerSave* save) {
  if (!save)
    return TR("career_no_financial_data");
  return TRF("career_owner_finance_line",
             {FormatOwnerMoney(save->finances.netWorth), FormatOwnerMoney(save->transferBudget),
              std::to_string(save->seasonWins), std::to_string(save->seasonDraws),
              std::to_string(save->seasonLosses)});
}

}  // namespace

// ---------------------------------------------------------------------------
// OwnerHubPage - Modernized Executive Dashboard
// ---------------------------------------------------------------------------

OwnerHubPage::OwnerHubPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();

  Gui2Frame* root = new Gui2Frame(windowManager, "frame_owner_root", 3, 2, 94, 96, true);
  this->AddView(root);
  root->Show();

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_ownerhub", 3, 2, 88, 3, TR("career_owner_title"));
  root->AddView(title);
  title->Show();

  Gui2Caption* topLine =
      new Gui2Caption(windowManager, "caption_owner_topline", 3, 6, 88, 2, BuildOwnerTopLine(save));
  root->AddView(topLine);
  topLine->Show();

  Gui2Caption* financeLine = new Gui2Caption(windowManager, "caption_owner_finline", 3, 8, 88, 2,
                                             BuildOwnerFinanceLine(save));
  root->AddView(financeLine);
  financeLine->Show();

  if (save) {
    int contentX = 31;
    int contentW = 60;

    Gui2Frame* finFrame =
        new Gui2Frame(windowManager, "frame_oh_fin", contentX, 13, contentW, 12, true);

    Gui2Caption* finTitle = new Gui2Caption(windowManager, "cap_oh_fintitle", 2, 1, contentW - 4, 2,
                                            TR("career_owner_fin_snapshot"));
    finFrame->AddView(finTitle);
    finTitle->Show();

    std::string finStr =
        TRF("career_owner_fin_body",
            {std::to_string(save->finances.netWorth), std::to_string(save->transferBudget),
             CareerDatabase::GetInstance().GetFinancialHealthString(),
             std::to_string(save->finances.ticketPrice)});
    Gui2Caption* finBody =
        new Gui2Caption(windowManager, "cap_oh_finbody", 2, 4, contentW - 4, 6, finStr);
    finFrame->AddView(finBody);
    finBody->Show();

    root->AddView(finFrame);
    finFrame->Show();

    Gui2Frame* boardFrame =
        new Gui2Frame(windowManager, "frame_oh_brd", contentX, 27, contentW, 11, true);

    Gui2Caption* boardTitle = new Gui2Caption(windowManager, "cap_oh_brdtitle", 2, 1, contentW - 4,
                                              2, TR("career_owner_club_status"));
    boardFrame->AddView(boardTitle);
    boardTitle->Show();

    std::string boardStr =
        TRF("career_owner_board_body",
            {std::to_string(save->boardConfidence), std::to_string(save->fanBase),
             std::to_string(save->stadium.fanSatisfaction)});
    Gui2Caption* boardBody =
        new Gui2Caption(windowManager, "cap_oh_brdbody", 2, 4, contentW - 4, 6, boardStr);
    boardFrame->AddView(boardBody);
    boardBody->Show();

    root->AddView(boardFrame);
    boardFrame->Show();

    Gui2Frame* infFrame =
        new Gui2Frame(windowManager, "frame_oh_inf", contentX, 41, contentW, 12, true);

    Gui2Caption* infTitle = new Gui2Caption(windowManager, "cap_oh_inftitle", 2, 1, contentW - 4, 2,
                                            TR("career_owner_infrastructure"));
    infFrame->AddView(infTitle);
    infTitle->Show();

    std::string infStr =
        TRF("career_owner_inf_body",
            {save->stadium.name, std::to_string(save->stadium.capacity),
             std::to_string(save->activeSponsors.size()), std::to_string(save->staff.size()),
             std::to_string(save->stadium.upgrades.size())});
    Gui2Caption* infBody =
        new Gui2Caption(windowManager, "cap_oh_infbody", 2, 4, contentW - 4, 6, infStr);
    infFrame->AddView(infBody);
    infBody->Show();

    root->AddView(infFrame);
    infFrame->Show();
  }

  Gui2Frame* navFrame = new Gui2Frame(windowManager, "frame_oh_nav", 3, 13, 25, 66, true);

  Gui2Caption* navTitle =
      new Gui2Caption(windowManager, "cap_oh_navtitle", 1, 1, 22, 2, TR("career_owner_management"));
  navFrame->AddView(navTitle);
  navTitle->Show();

  Gui2Grid* navGrid = new Gui2Grid(windowManager, "oh_nav_grid", 1, 3, 23, 58);

  Gui2Button* btnStadium =
      new Gui2Button(windowManager, "btn_oh_stadium", 0, 0, 22, 3, TR("career_owner_stadium_nav"));
  Gui2Button* btnFinances = new Gui2Button(windowManager, "btn_oh_finances", 0, 0, 22, 3,
                                           TR("career_owner_finances_nav"));
  Gui2Button* btnSponsors = new Gui2Button(windowManager, "btn_oh_sponsors", 0, 0, 22, 3,
                                           TR("career_owner_sponsors_nav"));
  Gui2Button* btnStaff =
      new Gui2Button(windowManager, "btn_oh_staff", 0, 0, 22, 3, TR("career_owner_staff_nav"));
  Gui2Button* btnBoard =
      new Gui2Button(windowManager, "btn_oh_board", 0, 0, 22, 3, TR("career_owner_boardroom_nav"));

  Gui2Caption* navSep1 =
      new Gui2Caption(windowManager, "cap_oh_sep1", 0, 0, 22, 2, TR("career_section_squad"));
  Gui2Button* btnTransfers = new Gui2Button(windowManager, "btn_oh_transfers", 0, 0, 22, 3,
                                            TR("career_owner_transfers_nav"));
  Gui2Button* btnSquad =
      new Gui2Button(windowManager, "btn_oh_squad", 0, 0, 22, 3, TR("career_squad_title"));
  Gui2Button* btnTraining =
      new Gui2Button(windowManager, "btn_oh_training", 0, 0, 22, 3, TR("career_training_title"));
  Gui2Button* btnFreeAgency =
      new Gui2Button(windowManager, "btn_oh_freeagency", 0, 0, 22, 3, TR("career_fa_title"));
  Gui2Button* btnYouth =
      new Gui2Button(windowManager, "btn_oh_youth", 0, 0, 22, 3, TR("career_youth_title"));

  Gui2Button* btnSeason =
      new Gui2Button(windowManager, "btn_oh_season", 0, 0, 22, 4, TR("career_advance_season"));
  Gui2Button* btnMatchday =
      new Gui2Button(windowManager, "btn_oh_matchday", 0, 0, 22, 4, TR("career_play_matchday"));

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_oh_back_main", 0, 0, 22, 4, TR("career_menu_back_modes"));
  btnBack->sig_OnClick.connect([this](...) { CreatePage(e_PageID_CareerMenu); });

  btnStadium->sig_OnClick.connect([this](...) { GoStadium(); });
  btnFinances->sig_OnClick.connect([this](...) { GoFinances(); });
  btnStaff->sig_OnClick.connect([this](...) { GoStaffManagement(); });
  btnSponsors->sig_OnClick.connect([this](...) { GoSponsors(); });
  btnBoard->sig_OnClick.connect([this](...) { GoBoardRoom(); });
  btnSeason->sig_OnClick.connect([this](...) { GoSeason(); });
  btnMatchday->sig_OnClick.connect([this](...) { GoMatchday(); });
  btnTransfers->sig_OnClick.connect([this](...) { GoTransferMarket(); });
  btnSquad->sig_OnClick.connect([this](...) { GoSquad(); });
  btnTraining->sig_OnClick.connect([this](...) { GoTraining(); });
  btnFreeAgency->sig_OnClick.connect([this](...) { GoFreeAgency(); });
  btnYouth->sig_OnClick.connect([this](...) { GoYouthAcademy(); });

  int row = 0;
  navGrid->AddView(btnStadium, row++, 0);
  navGrid->AddView(btnFinances, row++, 0);
  navGrid->AddView(btnSponsors, row++, 0);
  navGrid->AddView(btnStaff, row++, 0);
  navGrid->AddView(btnBoard, row++, 0);

  // Section separator
  navGrid->AddView(navSep1, row++, 0);

  navGrid->AddView(btnTransfers, row++, 0);
  navGrid->AddView(btnSquad, row++, 0);
  navGrid->AddView(btnTraining, row++, 0);
  navGrid->AddView(btnFreeAgency, row++, 0);
  navGrid->AddView(btnYouth, row++, 0);

  // Section separator
  Gui2Caption* navSep2 =
      new Gui2Caption(windowManager, "cap_oh_sep2", 0, 0, 22, 2, TR("career_section_season"));
  navGrid->AddView(navSep2, row++, 0);

  navGrid->AddView(btnSeason, row++, 0);
  navGrid->AddView(btnMatchday, row++, 0);

  // Back lives in the same grid so it is reachable by arrows/d-pad too.
  navGrid->AddView(btnBack, row++, 0);

  navGrid->UpdateLayout(0.5);
  navFrame->AddView(navGrid);
  navGrid->Show();

  root->AddView(navFrame);
  navFrame->Show();

  btnMatchday->SetFocus();
  this->Show();
}

OwnerHubPage::~OwnerHubPage() {}

void OwnerHubPage::GoStadium() {
  CreatePage(e_PageID_OwnerStadium);
}
void OwnerHubPage::GoFinances() {
  CreatePage(e_PageID_OwnerFinances);
}
void OwnerHubPage::GoStaffManagement() {
  CreatePage(e_PageID_OwnerStaff);
}
void OwnerHubPage::GoSponsors() {
  CreatePage(e_PageID_OwnerSponsors);
}
void OwnerHubPage::GoBoardRoom() {
  CreatePage(e_PageID_OwnerBoardRoom);
}
void OwnerHubPage::GoTransferMarket() {
  CreatePage(e_PageID_CareerTransferMarket);
}
void OwnerHubPage::GoSquad() {
  CreatePage(e_PageID_CareerSquadRoster);
}
void OwnerHubPage::GoTraining() {
  CreatePage(e_PageID_CareerTraining);
}
void OwnerHubPage::GoFreeAgency() {
  CreatePage(e_PageID_CareerFreeAgency);
}
void OwnerHubPage::GoYouthAcademy() {
  CreatePage(e_PageID_CareerYouthAcademy);
}
void OwnerHubPage::GoSeason() {
  CreatePage(e_PageID_CareerSeason);
}
void OwnerHubPage::GoMatchday() {
  CreatePage(e_PageID_CareerMatchday);
}

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
      new Gui2Button(windowManager, "btn_stad_back", 26, 89, 40, 3, TR("career_back_owner_hub"));
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

    Gui2Button* btnTicketUp = new Gui2Button(windowManager, "btn_ticket_up", 0, 0, 38, 2.5,
                                             TRF("career_ticket_inc", {std::to_string(10)}));
    btnTicketUp->sig_OnClick.connect(
        [this, currentPrice](...) { SetTicketPrice(currentPrice + 10); });
    actGrid->AddView(btnTicketUp, row, 0);

    Gui2Button* btnTicketDown = new Gui2Button(windowManager, "btn_ticket_down", 0, 0, 38, 2.5,
                                               TRF("career_ticket_dec", {std::to_string(10)}));
    btnTicketDown->sig_OnClick.connect(
        [this, currentPrice](...) { SetTicketPrice(currentPrice - 10); });
    actGrid->AddView(btnTicketDown, row++, 1);

    Gui2Button* btnFanInvest =
        new Gui2Button(windowManager, "btn_fan_invest", 0, 0, 38, 2.5, TR("career_invest_fan"));
    btnFanInvest->sig_OnClick.connect([this](...) { InvestFanBase(); });
    actGrid->AddView(btnFanInvest, row, 0);

    Gui2Button* btnPrestige = new Gui2Button(windowManager, "btn_prestige_invest", 0, 0, 38, 2.5,
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
      new Gui2Button(windowManager, "btn_fin_back", 26, 89, 40, 3, TR("career_back_owner_hub"));
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

    Gui2Caption* header =
        new Gui2Caption(windowManager, "caption_staff_hdr", 2, 2, 80, 2, TR("career_staff_header"));
    staffFrame->AddView(header);
    header->Show();

    Gui2Grid* grid = new Gui2Grid(windowManager, "staff_grid", 2, 5, 80, 40);
    int row = 0;
    for (const auto& s : save->staff) {
      const std::string rowLabel = TRF(
          "career_staff_row", {s.name, s.role, std::to_string(s.skill), FormatOwnerMoney(s.salary),
                               std::to_string(s.contractYearsRemaining), std::to_string(s.morale)});
      Gui2Button* btn = new Gui2Button(windowManager, "btn_staff_" + std::to_string(row), 0, 0, 79,
                                       2.5, "[" + TR("career_fire") + "] " + rowLabel);
      std::string staffName = s.name;
      btn->sig_OnClick.connect([this, staffName](...) { FireStaff(staffName); });
      grid->AddView(btn, row++, 0);
    }
    grid->UpdateLayout(0.5);
    staffFrame->AddView(grid);
    grid->Show();

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

  Gui2Button* btnHire =
      new Gui2Button(windowManager, "btn_staff_hire", 8, 87, 34, 3, TR("career_browse_candidates"));
  btnHire->sig_OnClick.connect([this](...) { GoHirePage(); });
  root->AddView(btnHire);
  btnHire->Show();

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_staff_back", 48, 87, 34, 3, TR("career_back_owner_hub"));
  btnBack->sig_OnClick.connect([this](...) { CreatePage(e_PageID_OwnerHub); });
  root->AddView(btnBack);
  btnBack->Show();

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

  Gui2Caption* header =
      new Gui2Caption(windowManager, "caption_hire_hdr", 2, 2, 80, 2, TR("career_hire_header"));
  hireFrame->AddView(header);
  header->Show();

  Gui2Grid* grid = new Gui2Grid(windowManager, "hire_grid", 2, 5, 80, 50);
  int row = 0;
  for (int i = 0; i < static_cast<int>(m_candidates.size()); i++) {
    const auto& c = m_candidates[i];
    const std::string rowLabel =
        TRF("career_hire_row", {c.name, c.role, std::to_string(c.skill), FormatOwnerMoney(c.salary),
                                std::to_string(c.contractYearsRemaining)});
    Gui2Button* btn = new Gui2Button(windowManager, "btn_hire_" + std::to_string(i), 0, 0, 79, 2.5,
                                     "[" + TR("career_hire") + "] " + rowLabel);
    btn->sig_OnClick.connect([this, i](...) { HireCandidate(i); });
    grid->AddView(btn, row++, 0);
  }
  grid->UpdateLayout(0.5);
  hireFrame->AddView(grid);
  grid->Show();

  root->AddView(hireFrame);
  hireFrame->Show();

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_hire_back", 30, 86, 32, 3, TR("career_back_staff"));
  btnBack->sig_OnClick.connect([this](...) { CreatePage(e_PageID_OwnerStaff); });
  root->AddView(btnBack);
  btnBack->Show();

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

    if (!save->activeSponsors.empty()) {
      int sHeight = 4 + save->activeSponsors.size() * 3;
      Gui2Frame* activeFrame =
          new Gui2Frame(windowManager, "frame_sp_active", 4, nextY, 84, sHeight, true);

      Gui2Caption* activeTitle = new Gui2Caption(windowManager, "caption_sp_active", 2, 1, 80, 2,
                                                 TR("career_sponsors_active"));
      activeFrame->AddView(activeTitle);
      activeTitle->Show();

      Gui2Grid* activeGrid = new Gui2Grid(windowManager, "sp_active_grid", 2, 4, 80, sHeight - 4);
      int row = 0;
      for (const auto& sp : save->activeSponsors) {
        std::string label =
            TRF("career_sponsor_row", {sp.sponsorName, sp.type, std::to_string(sp.annualRevenue),
                                       std::to_string(sp.yearsRemaining)});
        Gui2Button* btn = new Gui2Button(windowManager, "btn_sp_term_" + std::to_string(row), 0, 0,
                                         79, 2.5, "[" + TR("career_terminate") + "] " + label);
        std::string spName = sp.sponsorName;
        btn->sig_OnClick.connect([this, spName](...) { TerminateDeal(spName); });
        activeGrid->AddView(btn, row++, 0);
      }
      activeGrid->UpdateLayout(0.5);
      activeFrame->AddView(activeGrid);
      activeGrid->Show();

      root->AddView(activeFrame);
      activeFrame->Show();

      nextY += sHeight + 1;
    }

    Gui2Frame* offersFrame =
        new Gui2Frame(windowManager, "frame_sp_offers", 4, nextY, 84, 88 - nextY, true);

    Gui2Caption* offersTitle = new Gui2Caption(windowManager, "caption_sp_offers", 2, 1, 80, 2,
                                               TR("career_sponsors_offers"));
    offersFrame->AddView(offersTitle);
    offersTitle->Show();

    if (save->availableSponsorOffers.empty()) {
      Gui2Caption* noOffers = new Gui2Caption(windowManager, "caption_sp_none", 2, 4, 80, 2,
                                              TR("career_sponsors_none"));
      offersFrame->AddView(noOffers);
      noOffers->Show();
    } else {
      Gui2Grid* offersGrid =
          new Gui2Grid(windowManager, "sp_offers_grid", 2, 4, 80, 88 - nextY - 4);
      int row = 0;
      for (int i = 0; i < static_cast<int>(save->availableSponsorOffers.size()); i++) {
        const auto& sp = save->availableSponsorOffers[i];
        std::string label =
            TRF("career_sponsor_offer_row",
                {sp.sponsorName, sp.type, std::to_string(sp.annualRevenue),
                 std::to_string(sp.yearsRemaining), std::to_string(sp.reputationRequirement)});
        Gui2Button* btn = new Gui2Button(windowManager, "btn_sp_acc_" + std::to_string(i), 0, 0, 79,
                                         2.5, "[" + TR("career_accept") + "] " + label);
        btn->sig_OnClick.connect([this, i](...) { AcceptDeal(i); });
        offersGrid->AddView(btn, row++, 0);
      }
      offersGrid->UpdateLayout(0.5);
      offersFrame->AddView(offersGrid);
      offersGrid->Show();
    }

    root->AddView(offersFrame);
    offersFrame->Show();
  }

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_sp_back", 30, 90, 32, 3, TR("career_back_owner_hub"));
  btnBack->sig_OnClick.connect([this](...) { CreatePage(e_PageID_OwnerHub); });
  root->AddView(btnBack);
  btnBack->Show();

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
      Gui2Caption* evtLine =
          new Gui2Caption(windowManager, "caption_br_evt_" + std::to_string(i), 2, subY, 78, 2,
                          prefix + "[" + evt.type + "] " + evt.description + repStr);
      evtFrame->AddView(evtLine);
      evtLine->Show();
      subY += 2;
    }

    root->AddView(evtFrame);
    evtFrame->Show();
  }

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_br_back", 30, 90, 32, 3, TR("career_back_owner_hub"));
  btnBack->sig_OnClick.connect([this](...) { CreatePage(e_PageID_OwnerHub); });
  root->AddView(btnBack);
  btnBack->Show();

  btnBack->SetFocus();
  this->Show();
}

OwnerBoardRoomPage::~OwnerBoardRoomPage() {}
