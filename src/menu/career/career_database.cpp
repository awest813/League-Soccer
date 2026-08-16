#include "career_database.hpp"

#include <algorithm>
#include <fstream>
#include <string>
#include <vector>

#include "career_board.hpp"
#include "career_finance.hpp"
#include "career_persistence.hpp"
#include "career_sim.hpp"
#include "career_sponsors.hpp"
#include "career_staff.hpp"
#include "career_training.hpp"
#include "career_transfers.hpp"
#include "utils/localization.hpp"

namespace blunted {

CareerDatabase::CareerDatabase() {}
CareerDatabase::~CareerDatabase() {}

bool CareerDatabase::Initialize(const std::string& saveDir) {
  m_saveDirectory = saveDir;
  return true;
}

std::string CareerDatabase::GetSlotPath(int slotIndex) const {
  if (m_saveDirectory.empty())
    return "career.save";
  if (slotIndex == -1)
    return m_saveDirectory + "/career_autosave.save";
  if (slotIndex == 0)
    return m_saveDirectory + "/career.save";
  return m_saveDirectory + "/career_slot_" + std::to_string(slotIndex) + ".save";
}

bool CareerDatabase::HasSaveFile() const {
  return HasSaveSlot(0) || HasSaveSlot(-1);
}

bool CareerDatabase::HasSaveSlot(int slotIndex) const {
  std::string path = GetSlotPath(slotIndex);
  CareerPersistence::CareerSaveSummary summary;
  return CareerPersistence::ReadSummary(path, summary) && summary.isValid;
}

bool CareerDatabase::LoadCareerSave(const std::string& saveName) {
  if (LoadCareerSlot(0)) {
    printf("[career] Loaded default save: %s\n", saveName.c_str());
    return true;
  }
  if (LoadCareerSlot(-1)) {
    printf("[career] Loaded autosave fallback: %s\n", saveName.c_str());
    return true;
  }
  return false;
}

bool CareerDatabase::LoadCareerSlot(int slotIndex) {
  std::string path = GetSlotPath(slotIndex);
  CareerSave loaded;
  std::vector<TransferBid> loadedBids;
  if (!CareerPersistence::Load(loaded, loadedBids, path))
    return false;
  m_activeSave = std::make_unique<CareerSave>(loaded);
  m_activeBids = loadedBids;
  printf("[career] Loaded slot %d from %s\n", slotIndex, path.c_str());
  return true;
}

bool CareerDatabase::SaveCareerData() {
  return SaveCareerSlot(0);
}

bool CareerDatabase::SaveCareerSlot(int slotIndex) {
  if (!m_activeSave)
    return false;
  std::string path = GetSlotPath(slotIndex);
  bool success = CareerPersistence::Save(*m_activeSave, m_activeBids, path);
  if (success)
    printf("[career] Saved slot %d to %s\n", slotIndex, path.c_str());
  return success;
}

bool CareerDatabase::AutoSave() {
  if (!m_activeSave)
    return false;
  return SaveCareerSlot(-1);
}

bool CareerDatabase::GetSlotSummary(int slotIndex, CareerPersistence::CareerSaveSummary& outSummary) const {
  std::string path = GetSlotPath(slotIndex);
  return CareerPersistence::ReadSummary(path, outSummary);
}

bool CareerDatabase::CreateNewCareer(const std::string& careerName, const std::string& mode,
                                     const std::string& managerName) {
  m_activeSave = std::make_unique<CareerSave>();
  m_activeSave->name = careerName;
  m_activeSave->managerName = managerName;
  m_activeSave->club.clubName = careerName;
  if (mode == "player")
    m_activeSave->mode = CareerMode::PLAYER;
  else if (mode == "mygm")
    m_activeSave->mode = CareerMode::GM;
  else if (mode == "mycoach")
    m_activeSave->mode = CareerMode::COACH;
  else if (mode == "owner")
    m_activeSave->mode = CareerMode::OWNER;
  else
    m_activeSave->mode = CareerMode::MANAGER;
  m_activeSave->reputation = 50;
  m_activeSave->club.reputation = 50;
  m_activeSave->boardConfidence = 75;
  m_activeSave->board.confidence = 75;
  m_activeSave->transferBudget = 15000000;
  m_activeSave->wageBudget = 250000;
  m_activeSave->finance.transferBudget = m_activeSave->transferBudget;
  m_activeSave->finance.wageBudget = m_activeSave->wageBudget;
  m_activeSave->club.leagueName = "Default League";
  m_activeSave->season.currentSeason = 1;
  m_activeSave->currentSeason = 1;
  m_activeSave->season.currentWeek = 1;
  m_activeSave->season.inPreseason = true;
  m_activeSave->season.maxWeeks = 38;
  m_activeSave->season.transferWindowOpen = true;
  m_activeSave->stadium.name = careerName + " Stadium";
  CareerFinance::InitializeOwnerData(*m_activeSave);
  CareerBoard::GenerateBoardObjectives(*m_activeSave);
  CareerSponsors::GenerateSponsorOffers(*m_activeSave);
  CareerTransfers::SeedFreeAgents(*m_activeSave);
  return SaveCareerData();
}

void CareerDatabase::AddEvent(const std::string& eventType, const std::string& description,
                              int reputationDelta, bool isMajor) {
  if (!m_activeSave)
    return;
  m_activeSave->reputation =
      std::max(-100, std::min(100, m_activeSave->reputation + reputationDelta));
  m_activeSave->club.reputation = m_activeSave->reputation;
  m_activeSave->recentEvents.emplace_back(eventType, eventType + ": " + description,
                                          reputationDelta, 0, isMajor);
  if (m_activeSave->recentEvents.size() > 50)
    m_activeSave->recentEvents.erase(m_activeSave->recentEvents.begin());
  if (isMajor) {
    m_activeSave->legacyStats[eventType]++;
    // Persist on major milestones only. Routine matchday chatter used to flush
    // the save file on every simulated fixture (thousands of writes per season).
    SaveCareerData();
  }
}

void CareerDatabase::RecruitFreeAgent(const std::string& playerName) {
  if (m_activeSave)
    CareerTransfers::RecruitFreeAgent(*m_activeSave, *this, playerName);
}

void CareerDatabase::ScoutYouthPlayer() {
  if (m_activeSave)
    CareerTraining::ScoutYouthPlayer(*m_activeSave, *this);
}

void CareerDatabase::PromoteYouthPlayer(const std::string& playerName) {
  if (m_activeSave)
    CareerTraining::PromoteYouthPlayer(*m_activeSave, *this, playerName);
}

void CareerDatabase::ModifyBudget(long long transferDelta, long long wageDelta) {
  if (m_activeSave)
    CareerFinance::ModifyBudget(*m_activeSave, transferDelta, wageDelta);
}

void CareerDatabase::ModifyBoardConfidence(int delta) {
  if (!m_activeSave)
    return;
  m_activeSave->boardConfidence = std::max(0, std::min(100, m_activeSave->boardConfidence + delta));
  m_activeSave->board.confidence = m_activeSave->boardConfidence;
}

bool CareerDatabase::TrainSquad() {
  return m_activeSave && CareerTraining::TrainSquad(*m_activeSave, *this);
}

bool CareerDatabase::TrainFocus(const std::string& focusArea) {
  return m_activeSave && CareerTraining::TrainFocus(*m_activeSave, *this, focusArea);
}

void CareerDatabase::SetStrategy(const std::string& strategy) {
  if (m_activeSave)
    CareerTraining::SetStrategy(*m_activeSave, *this, strategy);
}

int CareerDatabase::GetReputation() const {
  return m_activeSave ? m_activeSave->reputation : 0;
}

std::string CareerDatabase::GetReputationStatus() const {
  if (!m_activeSave)
    return TR("career_rep_unknown");
  int rep = m_activeSave->reputation;
  if (rep >= 80)
    return TR("career_rep_legendary");
  if (rep >= 50)
    return TR("career_rep_respected");
  if (rep >= 20)
    return TR("career_rep_known");
  if (rep >= -20)
    return TR("career_rep_unproven");
  if (rep >= -50)
    return TR("career_rep_controversial");
  return TR("career_rep_notorious");
}

std::string CareerDatabase::GetMoraleString(int morale) const {
  if (morale >= 80)
    return TR("career_morale_happy");
  if (morale >= 40)
    return TR("career_morale_content");
  return TR("career_morale_unhappy");
}

std::string CareerDatabase::GetFormString(int form) const {
  if (form >= 80)
    return TR("career_form_excellent");
  if (form >= 40)
    return TR("career_form_good");
  return TR("career_form_poor");
}

std::string CareerDatabase::GetConditionArrow(int form) const {
  // Classic PES 5/6 Condition Arrow indicators:
  // Red Up (Top) > Orange Diagonal (Good) > Green Right (Normal) > Blue Diagonal (Poor) > Purple Down (Terrible)
  if (form >= 85)
    return "[^] TOP";
  if (form >= 65)
    return "[/] GOOD";
  if (form >= 40)
    return "[>] NORM";
  if (form >= 20)
    return "[\\] POOR";
  return "[v] BAD";
}

std::string CareerDatabase::GetFormGuideString(int count) const {
  if (!m_activeSave)
    return "[ - - - - - ]";
  std::string guide = "";
  int wins = m_activeSave->seasonWins;
  int draws = m_activeSave->seasonDraws;
  int losses = m_activeSave->seasonLosses;
  int total = wins + draws + losses;
  if (total == 0)
    return "[ - - - - - ]";

  // Build a realistic recent form sequence from the season record
  std::vector<std::string> recent;
  for (int i = 0; i < count; i++) {
    int seed = (m_activeSave->season.currentWeek * 7 + i * 13) % 100;
    if (wins > 0 && (seed < (wins * 100 / total))) {
      recent.push_back("[W]");
    } else if (draws > 0 && (seed < ((wins + draws) * 100 / total))) {
      recent.push_back("[D]");
    } else {
      recent.push_back("[L]");
    }
  }
  for (const auto& r : recent) {
    guide += r + " ";
  }
  return guide;
}

std::vector<std::string> CareerDatabase::GetNewsHeadlines(int count) const {
  if (!m_activeSave)
    return {"Transfer window opens with record activity across top divisions."};

  std::vector<std::string> headlines;
  // 1. Match result / form headline
  int played = m_activeSave->seasonWins + m_activeSave->seasonDraws + m_activeSave->seasonLosses;
  if (played == 0) {
    headlines.push_back("PRE-SEASON: " + m_activeSave->name + " gears up for ambitious campaign in " + m_activeSave->club.leagueName + ".");
  } else if (m_activeSave->seasonWins > m_activeSave->seasonLosses * 2) {
    headlines.push_back("MEDIA SPOTLIGHT: Pundits praise " + m_activeSave->name + "'s tactical fluidity and dominant run of form.");
  } else if (m_activeSave->seasonLosses > m_activeSave->seasonWins) {
    headlines.push_back("PRESSURE BUILDS: Manager " + m_activeSave->managerName + " calls for resilience amid testing fixture schedule.");
  } else {
    headlines.push_back("COMPETITIVE RACE: " + m_activeSave->name + " stays in contention as mid-table battle intensifies.");
  }

  // 2. Squad / Youth headline
  if (!m_activeSave->youthAcademy.empty()) {
    headlines.push_back("ACADEMY REPORT: Scouts spotlight " + m_activeSave->youthAcademy[0].name + " as a potential future star.");
  } else if (!m_activeSave->roster.empty()) {
    headlines.push_back("SQUAD FOCUS: " + m_activeSave->roster[0].name + " maintaining peak match condition ahead of next clash.");
  }

  // 3. Board / Club operations headline
  if (m_activeSave->boardConfidence >= 75) {
    headlines.push_back("BOARD CONFIDENCE: Club hierarchy 'delighted' with current management and financial health.");
  } else {
    headlines.push_back("BOARD NOTICE: Club leadership expects strong performance in upcoming league fixtures.");
  }

  while (static_cast<int>(headlines.size()) > count) {
    headlines.pop_back();
  }
  return headlines;
}

std::string CareerDatabase::GetNextOpponentPreview(int week) const {
  static const std::vector<std::string> opponentNames = {
      "FC United",     "Athletic Club", "Wanderers FC",      "Real Deportivo", "Inter Milano",
      "Bayern Munich", "FC Barcelona",  "Chelsea FC",        "Arsenal FC",     "Juventus Turin",
      "AC Milan",      "Liverpool FC",  "Borussia Dortmund", "Paris SG",       "Ajax Amsterdam",
      "Porto FC",      "Benfica",       "Sporting CP",       "Napoli",         "Atletico Madrid",
      "Tottenham"};
  int opponentIdx = (week * 3) % static_cast<int>(opponentNames.size());
  std::string opp = opponentNames[opponentIdx];
  bool isHome = (week % 2) == 0;
  std::string venue = isHome ? "Home (Your Stadium)" : "Away (" + opp + " Arena)";
  std::string danger = ((week % 3) == 0) ? "★★★★★ High Danger" : (((week % 2) == 0) ? "★★★☆☆ Moderate Threat" : "★★★★☆ Solid Defense");
  return opp + " | Venue: " + venue + "\nThreat Rating: " + danger + " | Expected Strategy: Balanced Press";
}

int CareerDatabase::GetLegacyStat(const std::string& statName) const {
  if (!m_activeSave)
    return 0;
  auto it = m_activeSave->legacyStats.find(statName);
  return it != m_activeSave->legacyStats.end() ? it->second : 0;
}

std::vector<CareerEvent> CareerDatabase::GetRecentEvents(int limit) const {
  if (!m_activeSave)
    return {};
  std::vector<CareerEvent> res;
  for (auto it = m_activeSave->recentEvents.rbegin();
       it != m_activeSave->recentEvents.rend() && static_cast<int>(res.size()) < limit; ++it) {
    res.push_back(*it);
  }
  return res;
}

void CareerDatabase::ProcessPlayerGrowth(PlayerCareerState& player) {
  CareerSim::ProcessPlayerGrowth(player);
}

void CareerDatabase::UpdatePlayerValue(PlayerCareerState& player) {
  CareerSim::UpdatePlayerValue(player);
}

int CareerDatabase::EstimateLeaguePosition(int wins, int draws, int losses) {
  return CareerSim::EstimateLeaguePosition(wins, draws, losses);
}

void CareerDatabase::AdvanceSeason() {
  if (!m_activeSave)
    return;
  CareerSim::AdvanceSeason(*m_activeSave, *this, m_activeBids, m_transferTargets);
  SaveCareerData();
}

void CareerDatabase::ReleasePlayer(const std::string& playerName) {
  if (m_activeSave)
    CareerTransfers::ReleasePlayer(*m_activeSave, *this, playerName);
}

void CareerDatabase::RecordMatchStats(const std::string& playerName, int goals, int assists) {
  if (m_activeSave)
    CareerSim::RecordMatchStats(*m_activeSave, playerName, goals, assists);
}

void CareerDatabase::PopulateTransferMarket() {
  if (m_activeSave)
    CareerTransfers::PopulateTransferMarket(m_transferTargets);
}

std::vector<TransferTarget> CareerDatabase::GetTransferTargets() const {
  return m_transferTargets;
}

TransferBid CareerDatabase::PlaceBid(const std::string& playerName, long long bidAmount,
                                     int offeredWage, int contractYears) {
  if (!m_activeSave)
    return TransferBid();
  return CareerTransfers::PlaceBid(*m_activeSave, *this, m_transferTargets, m_activeBids,
                                   playerName, bidAmount, offeredWage, contractYears);
}

void CareerDatabase::WithdrawBid(const std::string& playerName) {
  CareerTransfers::WithdrawBid(m_activeBids, playerName);
}

void CareerDatabase::ProcessPendingBids() {
  if (m_activeSave)
    CareerTransfers::ProcessPendingBids(*m_activeSave, *this, m_transferTargets, m_activeBids);
}

std::string CareerDatabase::GetBidStatusString(BidStatus status) const {
  return CareerTransfers::GetBidStatusString(status);
}

bool CareerDatabase::CompleteTransfer(const std::string& playerName) {
  if (!m_activeSave)
    return false;
  return CareerTransfers::CompleteTransfer(*m_activeSave, *this, m_transferTargets, m_activeBids,
                                           playerName);
}

void CareerDatabase::InitializeOwnerData() {
  if (m_activeSave)
    CareerFinance::InitializeOwnerData(*m_activeSave);
}

void CareerDatabase::UpgradeStadium(int upgradeIndex) {
  if (m_activeSave)
    CareerFinance::UpgradeStadium(*m_activeSave, *this, upgradeIndex);
}

void CareerDatabase::RenameStadium(const std::string& newName) {
  if (m_activeSave)
    CareerFinance::RenameStadium(*m_activeSave, newName);
}

void CareerDatabase::RepairStadium(int amount) {
  if (m_activeSave)
    CareerFinance::RepairStadium(*m_activeSave, amount);
}

void CareerDatabase::SetTicketPrice(int price) {
  if (m_activeSave)
    CareerFinance::SetTicketPrice(*m_activeSave, price);
}

void CareerDatabase::HireStaff(const StaffMember& member) {
  if (m_activeSave)
    CareerStaff::HireStaff(*m_activeSave, member);
}

void CareerDatabase::FireStaff(const std::string& staffName) {
  if (m_activeSave)
    CareerStaff::FireStaff(*m_activeSave, *this, staffName);
}

void CareerDatabase::GenerateStaffCandidates(std::vector<StaffMember>& candidates) {
  CareerStaff::GenerateStaffCandidates(candidates);
}

void CareerDatabase::GenerateSponsorOffers() {
  if (m_activeSave)
    CareerSponsors::GenerateSponsorOffers(*m_activeSave);
}

bool CareerDatabase::AcceptSponsorDeal(int dealIndex) {
  return m_activeSave && CareerSponsors::AcceptSponsorDeal(*m_activeSave, *this, dealIndex);
}

void CareerDatabase::TerminateSponsorDeal(const std::string& sponsorName) {
  if (m_activeSave)
    CareerSponsors::TerminateSponsorDeal(*m_activeSave, *this, sponsorName);
}

void CareerDatabase::ProcessSeasonFinances() {
  if (m_activeSave)
    CareerFinance::ProcessSeasonFinances(*m_activeSave);
}

long long CareerDatabase::GetSeasonProfit() const {
  return m_activeSave ? CareerFinance::GetSeasonProfit(*m_activeSave) : 0;
}

std::string CareerDatabase::GetFinancialHealthString() const {
  return m_activeSave ? CareerFinance::GetFinancialHealthString(*m_activeSave) : "Unknown";
}

void CareerDatabase::GenerateBoardObjectives() {
  if (m_activeSave)
    CareerBoard::GenerateBoardObjectives(*m_activeSave);
}

void CareerDatabase::EvaluateBoardObjectives() {
  if (m_activeSave)
    CareerBoard::EvaluateBoardObjectives(*m_activeSave, *this);
}

void CareerDatabase::InvestInFanBase(long long amount) {
  if (m_activeSave)
    CareerFinance::InvestInFanBase(*m_activeSave, amount);
}

void CareerDatabase::InvestInPrestige(long long amount) {
  if (m_activeSave)
    CareerFinance::InvestInPrestige(*m_activeSave, amount);
}

SimulatedMatch CareerDatabase::SimulateMatchResult(const std::string& opponentName,
                                                   const std::string& opponentTeamDBID,
                                                   bool isHome) {
  if (!m_activeSave)
    return SimulatedMatch{};
  return CareerSim::SimulateMatchResult(*m_activeSave, opponentName, opponentTeamDBID, isHome);
}

void CareerDatabase::SeedRng(unsigned int seed) {
  CareerCommon::SeedRng(seed);
}

void CareerDatabase::ApplyMatchResult(int homeGoals, int awayGoals,
                                      const std::string& opponentLabel,
                                      const std::vector<std::string>& scorers) {
  if (m_activeSave)
    CareerSim::ApplyMatchResult(*m_activeSave, *this, homeGoals, awayGoals, opponentLabel, scorers);
}

void CareerDatabase::Process3DMatchResult(int homeGoals, int awayGoals) {
  if (m_activeSave)
    CareerSim::Process3DMatchResult(*m_activeSave, *this, homeGoals, awayGoals);
}

}  // namespace blunted
