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

bool CareerDatabase::HasSaveFile() const {
  if (m_saveDirectory.empty())
    return false;
  std::ifstream file(m_saveDirectory + "/career.save");
  return file.good();
}

bool CareerDatabase::LoadCareerSave(const std::string& saveName) {
  if (m_saveDirectory.empty())
    return false;
  std::string path = m_saveDirectory + "/career.save";
  CareerSave loaded;
  std::vector<TransferBid> loadedBids;
  if (!CareerPersistence::Load(loaded, loadedBids, path))
    return false;
  m_activeSave = std::make_unique<CareerSave>(loaded);
  m_activeBids = loadedBids;
  printf("[career] Loaded save: %s\n", saveName.c_str());
  return true;
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

bool CareerDatabase::SaveCareerData() {
  if (!m_activeSave || m_saveDirectory.empty())
    return false;
  std::string path = m_saveDirectory + "/career.save";
  return CareerPersistence::Save(*m_activeSave, m_activeBids, path);
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
