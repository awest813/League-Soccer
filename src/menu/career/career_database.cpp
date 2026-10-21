#include "career_database.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "career_board.hpp"
#include "career_finance.hpp"
#include "career_hub_model.hpp"
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

bool CareerDatabase::SetPendingFixture(bool isHome, int userTeamDBID, int opponentTeamDBID,
                                       const std::string& opponentName, int expectedSeason,
                                       int expectedWeek) {
  if (!m_activeSave || !CanPlayCareerMatch(*m_activeSave))
    return false;
  const auto& season = m_activeSave->season;
  if ((expectedSeason >= 0 && expectedSeason != season.currentSeason) ||
      (expectedWeek >= 0 && expectedWeek != season.currentWeek) ||
      season.currentWeek > season.maxWeeks)
    return false;
  m_pendingFixture.hasFixture = true;
  m_pendingFixture.isHome = isHome;
  m_pendingFixture.season = season.currentSeason;
  m_pendingFixture.week = season.currentWeek;
  m_pendingFixture.userTeamDBID = userTeamDBID;
  m_pendingFixture.opponentTeamDBID = opponentTeamDBID;
  m_pendingFixture.opponentName = opponentName;
  return true;
}

bool CareerDatabase::HasPendingFixture() const {
  return m_pendingFixture.hasFixture;
}

const CareerPendingFixture& CareerDatabase::GetPendingFixture() const {
  return m_pendingFixture;
}

void CareerDatabase::ClearPendingFixture() {
  m_pendingFixture = CareerPendingFixture{};
}

bool CareerDatabase::ConsumePlayedFixture(int matchGoals0, int matchGoals1,
                                          const std::vector<int>& scorerIDs0,
                                          const std::vector<int>& scorerIDs1) {
  if (!m_pendingFixture.hasFixture || !m_activeSave)
    return false;
  const auto fixture = m_pendingFixture;
  // Credit the user's scorers who are in the career squad; others (database
  // players the career has since sold, for example) are skipped.
  std::vector<std::string> scorers;
  for (int scorerID : fixture.isHome ? scorerIDs0 : scorerIDs1) {
    if (scorerID <= 0)
      continue;
    for (const auto& player : m_activeSave->roster) {
      if (player.databaseID == scorerID) {
        scorers.push_back(player.name);
        break;
      }
    }
  }
  if (!CompleteFixture(fixture.season, fixture.week, fixture.isHome, fixture.userTeamDBID,
                       fixture.opponentTeamDBID, fixture.opponentName,
                       fixture.isHome ? matchGoals0 : matchGoals1,
                       fixture.isHome ? matchGoals1 : matchGoals0, scorers))
    return false;
  ClearPendingFixture();
  return true;
}

bool CareerDatabase::CommitLifecycle(const std::function<void()>& mutate) {
  if (!m_activeSave || m_saveDirectory.empty() || m_inLifecycle)
    return false;
  const CareerSave before = *m_activeSave;
  const auto bidsBefore = m_activeBids;
  const auto targetsBefore = m_transferTargets;
  const auto rngBefore = CareerCommon::Rng();
  m_inLifecycle = true;
  bool saved = false;
  try {
    mutate();
    // The primary save is the commit point used by Continue. Events must never
    // flush partially applied season rewards or match development to disk.
    saved = CareerPersistence::Save(*m_activeSave, m_activeBids, m_transferTargets,
                                    GetSlotPath(m_activeSlot));
  } catch (...) {
    saved = false;
  }
  m_inLifecycle = false;
  if (!saved) {
    *m_activeSave = before;
    m_activeBids = bidsBefore;
    m_transferTargets = targetsBefore;
    CareerCommon::Rng() = rngBefore;
    return false;
  }
  AutoSave();
  return true;
}

bool CareerDatabase::CompleteFixture(int season, int week, bool isHome, int userTeamID,
                                     int opponentTeamID, const std::string& opponentName,
                                     int userGoals, int opponentGoals,
                                     const std::vector<std::string>& scorers) {
  if (!m_activeSave || userGoals < 0 || opponentGoals < 0 ||
      season != m_activeSave->season.currentSeason || week != m_activeSave->season.currentWeek ||
      week < 1 || week > m_activeSave->season.maxWeeks || userTeamID == opponentTeamID ||
      opponentTeamID <= 0 || userTeamID <= 0 ||
      (m_activeSave->club.clubID > 0 && userTeamID != m_activeSave->club.clubID))
    return false;
  const int fixtureID = week * 100;
  for (const auto& fixture : m_activeSave->season.fixtures)
    if (fixture.fixtureID == fixtureID && fixture.season == season && fixture.played)
      return false;
  const bool completed = CommitLifecycle([&] {
    auto& save = *m_activeSave;
    FixtureResult result{fixtureID,
                         isHome ? userTeamID : opponentTeamID,
                         isHome ? opponentTeamID : userTeamID,
                         isHome ? userGoals : opponentGoals,
                         isHome ? opponentGoals : userGoals,
                         true,
                         season};
    auto existing = std::find_if(
        save.season.fixtures.begin(), save.season.fixtures.end(),
        [&](const FixtureResult& f) { return f.fixtureID == fixtureID && f.season == season; });
    if (existing == save.season.fixtures.end())
      save.season.fixtures.push_back(result);
    else
      *existing = result;
    if (!CanManageTeam(save) && !save.roster.empty()) {
      int fitness = 0;
      for (const auto& player : save.roster)
        fitness += player.fitness;
      const auto plan = fitness / static_cast<int>(save.roster.size()) < 70
                            ? CareerTrainingPlan::RECOVERY
                            : CareerTrainingPlan::BALANCED;
      SetTrainingPlan(plan, CareerActionSource::AI);
    }
    CareerSim::ApplyMatchResult(save, *this, userGoals, opponentGoals, opponentName, scorers);
    ++save.season.currentWeek;
    save.season.inPreseason = false;
  });
  if (completed)
    ClearPendingFixture();
  return completed;
}

bool CareerDatabase::Initialize(const std::string& saveDir) {
  m_saveDirectory = saveDir;
  return true;
}

std::string CareerDatabase::GetSlotPath(int slotIndex) const {
  std::string dir = m_saveDirectory.empty() ? "." : m_saveDirectory;
  if (slotIndex == -1)
    return dir + "/career_autosave.save";
  if (slotIndex == 0)
    return dir + "/career.save";
  return dir + "/career_slot_" + std::to_string(slotIndex) + ".save";
}

bool CareerDatabase::HasSaveFile() const {
  if (m_saveDirectory.empty())
    return false;
  return HasSaveSlot(0) || HasSaveSlot(-1);
}

bool CareerDatabase::HasSaveSlot(int slotIndex) const {
  if (m_saveDirectory.empty())
    return false;
  std::string path = GetSlotPath(slotIndex);
  CareerPersistence::CareerSaveSummary summary;
  return CareerPersistence::ReadSummary(path, summary) && summary.isValid;
}

bool CareerDatabase::LoadCareerSave(const std::string& saveName) {
  if (m_saveDirectory.empty())
    return false;
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
  if (m_saveDirectory.empty())
    return false;
  std::string path = GetSlotPath(slotIndex);
  CareerSave loaded;
  std::vector<TransferBid> loadedBids;
  std::vector<TransferTarget> loadedTargets;
  if (!CareerPersistence::Load(loaded, loadedBids, loadedTargets, path))
    return false;
  m_activeSave = std::make_unique<CareerSave>(loaded);
  m_activeBids = loadedBids;
  ClearPendingFixture();
  m_transferTargets = loadedTargets;
  // The autosave is shared by every slot; a career restored from it commits
  // to the default slot.
  m_activeSlot = slotIndex < 0 ? 0 : slotIndex;
  printf("[career] Loaded slot %d from %s\n", slotIndex, path.c_str());
  return true;
}

bool CareerDatabase::SaveCareerData() {
  return m_inLifecycle || SaveCareerSlot(m_activeSlot);
}

bool CareerDatabase::SaveCareerSlot(int slotIndex) {
  if (!m_activeSave || m_saveDirectory.empty())
    return false;
  std::string path = GetSlotPath(slotIndex);
  bool success = CareerPersistence::Save(*m_activeSave, m_activeBids, m_transferTargets, path);
  if (success && slotIndex >= 0)
    m_activeSlot = slotIndex;
  if (success)
    printf("[career] Saved slot %d to %s\n", slotIndex, path.c_str());
  return success;
}

bool CareerDatabase::DeleteCareerSlot(int slotIndex) {
  if (m_saveDirectory.empty() || !HasSaveSlot(slotIndex))
    return false;
  std::string path = GetSlotPath(slotIndex);
  namespace fs = std::filesystem;
  std::error_code ec;
  fs::remove(path, ec);
  fs::remove(path + ".bak", ec);
  fs::remove(path + ".tmp", ec);
  printf("[career] Deleted slot %d (%s)\n", slotIndex, path.c_str());
  return true;
}

bool CareerDatabase::AutoSave() {
  if (m_inLifecycle)
    return true;
  if (!m_activeSave || m_saveDirectory.empty())
    return false;
  return SaveCareerSlot(-1);
}

bool CareerDatabase::GetSlotSummary(int slotIndex,
                                    CareerPersistence::CareerSaveSummary& outSummary) const {
  if (m_saveDirectory.empty())
    return false;
  std::string path = GetSlotPath(slotIndex);
  return CareerPersistence::ReadSummary(path, outSummary);
}

bool CareerDatabase::CreateNewCareer(const std::string& careerName, const std::string& mode,
                                     const std::string& managerName) {
  CareerMode role;
  bool handsOn;
  if (!DecodeCareerRole(mode, role, handsOn, true))
    return false;
  m_activeSave = std::make_unique<CareerSave>();
  m_activeSave->name = careerName;
  m_activeSave->managerName = managerName;
  m_activeSave->club.clubName = careerName;
  m_activeSave->mode = role;
  m_activeSave->handsOnManagement = handsOn;
  m_activeSlot = 0;
  ClearPendingFixture();
  m_activeBids.clear();
  m_transferTargets.clear();
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
  m_activeSave->trainingPoints = (m_activeSave->mode == CareerMode::COACH) ? 20 : 10;
  m_activeSave->stadium.name = careerName + " Stadium";
  CareerFinance::InitializeOwnerData(*m_activeSave);
  CareerBoard::GenerateBoardObjectives(*m_activeSave);
  CareerSponsors::GenerateSponsorOffers(*m_activeSave);
  CareerTransfers::SeedFreeAgents(*m_activeSave);
  bool saved = SaveCareerData();
  AutoSave();
  return saved;
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

void CareerDatabase::RecruitFreeAgent(const std::string& playerName, CareerActionSource source) {
  if (!m_activeSave || (source == CareerActionSource::USER && !CanManageClub(*m_activeSave))) {
    return;
  }
  if (m_activeSave)
    CareerTransfers::RecruitFreeAgent(*m_activeSave, *this, playerName);
}

void CareerDatabase::ScoutYouthPlayer(CareerActionSource source) {
  if (!m_activeSave || (source == CareerActionSource::USER && !CanManageClub(*m_activeSave))) {
    return;
  }
  if (m_activeSave)
    CareerTraining::ScoutYouthPlayer(*m_activeSave, *this);
}

void CareerDatabase::PromoteYouthPlayer(const std::string& playerName, CareerActionSource source) {
  if (!m_activeSave || (source == CareerActionSource::USER && !CanManageClub(*m_activeSave))) {
    return;
  }
  if (m_activeSave)
    CareerTraining::PromoteYouthPlayer(*m_activeSave, *this, playerName);
}

void CareerDatabase::ModifyBudget(long long transferDelta, long long wageDelta,
                                  CareerActionSource source) {
  if (!m_activeSave || (source == CareerActionSource::USER && !CanManageClub(*m_activeSave))) {
    return;
  }
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
  return m_activeSave && CanManageTeam(*m_activeSave) &&
         CareerTraining::TrainSquad(*m_activeSave, *this);
}

bool CareerDatabase::SetHandsOnManagement(bool enabled) {
  if (!m_activeSave || !CanManageClub(*m_activeSave))
    return false;
  const bool previous = m_activeSave->handsOnManagement;
  m_activeSave->handsOnManagement = enabled;
  if (SaveCareerData())
    return true;
  m_activeSave->handsOnManagement = previous;
  return false;
}

bool CareerDatabase::SetTrainingPlan(CareerTrainingPlan plan, CareerActionSource source) {
  if (!m_activeSave || (source == CareerActionSource::USER && !CanManageTeam(*m_activeSave)) ||
      static_cast<int>(plan) < 0 || static_cast<int>(plan) > 2)
    return false;
  const auto previous = m_activeSave->trainingPlan;
  m_activeSave->trainingPlan = plan;
  if (SaveCareerData())
    return true;
  m_activeSave->trainingPlan = previous;
  return false;
}

bool CareerDatabase::TrainFocus(const std::string& focusArea) {
  if (!m_activeSave)
    return false;
  if (focusArea == "Individual") {
    // Individual sessions target exactly one identified footballer. Without
    // this check a save with no (or an ambiguous) controlled identity would
    // train every player whose database ID matches, e.g. all signings at 0.
    if (!ControlledCareerPlayer(*m_activeSave))
      return false;
  } else if (m_activeSave->mode == CareerMode::PLAYER || !CanManageTeam(*m_activeSave)) {
    return false;
  }
  return CareerTraining::TrainFocus(*m_activeSave, *this, focusArea);
}

bool CareerDatabase::MotivatePlayer(const std::string& playerName, CareerActionSource source) {
  if (!m_activeSave || (source == CareerActionSource::USER && !CanManageTeam(*m_activeSave))) {
    return false;
  }
  return m_activeSave && CareerTraining::MotivatePlayer(*m_activeSave, *this, playerName);
}

bool CareerDatabase::DrillPlayer(const std::string& playerName, CareerActionSource source) {
  if (!m_activeSave || (source == CareerActionSource::USER && !CanManageTeam(*m_activeSave))) {
    return false;
  }
  return m_activeSave && CareerTraining::DrillPlayer(*m_activeSave, *this, playerName);
}

void CareerDatabase::SetStrategy(const std::string& strategy, CareerActionSource source) {
  if (!m_activeSave || (source == CareerActionSource::USER && !CanManageTeam(*m_activeSave))) {
    return;
  }
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
  // Red Up (Top) > Orange Diagonal (Good) > Green Right (Normal) > Blue Diagonal (Poor) > Purple
  // Down (Terrible)
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
  if (!m_activeSave || m_activeSave->seasonForm.empty())
    return "[ - - - - - ]";
  // The club's real recent league results, oldest first.
  const std::string& form = m_activeSave->seasonForm;
  const size_t shown = std::min(form.size(), static_cast<size_t>(std::max(0, count)));
  std::string guide;
  for (size_t i = form.size() - shown; i < form.size(); ++i)
    guide += std::string("[") + form[i] + "] ";
  return guide;
}

std::vector<std::string> CareerDatabase::GetNewsHeadlines(int count) const {
  if (!m_activeSave)
    return {"Transfer window opens with record activity across top divisions."};

  std::vector<std::string> headlines;
  // 1. Match result / form headline
  int played = m_activeSave->seasonWins + m_activeSave->seasonDraws + m_activeSave->seasonLosses;
  if (played == 0) {
    headlines.push_back("PRE-SEASON: " + m_activeSave->name +
                        " gears up for ambitious campaign in " + m_activeSave->club.leagueName +
                        ".");
  } else if (m_activeSave->seasonWins > m_activeSave->seasonLosses * 2) {
    headlines.push_back("MEDIA SPOTLIGHT: Pundits praise " + m_activeSave->name +
                        "'s tactical fluidity and dominant run of form.");
  } else if (m_activeSave->seasonLosses > m_activeSave->seasonWins) {
    headlines.push_back("PRESSURE BUILDS: Manager " + m_activeSave->managerName +
                        " calls for resilience amid testing fixture schedule.");
  } else {
    headlines.push_back("COMPETITIVE RACE: " + m_activeSave->name +
                        " stays in contention as mid-table battle intensifies.");
  }

  // 2. Squad / Youth headline
  if (!m_activeSave->youthAcademy.empty()) {
    headlines.push_back("ACADEMY REPORT: Scouts spotlight " + m_activeSave->youthAcademy[0].name +
                        " as a potential future star.");
  } else if (!m_activeSave->roster.empty()) {
    headlines.push_back("SQUAD FOCUS: " + m_activeSave->roster[0].name +
                        " maintaining peak match condition ahead of next clash.");
  }

  // 3. Board / Club operations headline
  if (m_activeSave->boardConfidence >= 75) {
    headlines.push_back(
        "BOARD CONFIDENCE: Club hierarchy 'delighted' with current management and financial "
        "health.");
  } else {
    headlines.push_back(
        "BOARD NOTICE: Club leadership expects strong performance in upcoming league fixtures.");
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
  std::string danger = ((week % 3) == 0)
                           ? "★★★★★ High Danger"
                           : (((week % 2) == 0) ? "★★★☆☆ Moderate Threat" : "★★★★☆ Solid Defense");
  return opp + " | Venue: " + venue + "\nThreat Rating: " + danger +
         " | Expected Strategy: Balanced Press";
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
  CareerSim::ProcessPlayerGrowth(player, m_activeSave.get());
}

void CareerDatabase::UpdatePlayerValue(PlayerCareerState& player) {
  CareerSim::UpdatePlayerValue(player);
}

int CareerDatabase::EstimateLeaguePosition(int wins, int draws, int losses) {
  return CareerSim::EstimateLeaguePosition(wins, draws, losses);
}

std::vector<CareerSim::CareerLeagueTableRow> CareerDatabase::GetLeagueStandings(
    const std::vector<std::pair<int, std::string>>& leagueClubs) const {
  if (!m_activeSave)
    return {};
  // The first look at the table fixes the club list for the season.
  CareerSim::EnsureLeagueTable(*m_activeSave, leagueClubs);
  return CareerSim::GenerateLeagueStandings(*m_activeSave, leagueClubs);
}

std::vector<CareerSim::CareerTopScorer> CareerDatabase::GetTopScorers() const {
  if (!m_activeSave)
    return {};
  return CareerSim::GetTopScorers(*m_activeSave);
}

bool CareerDatabase::ExtendContract(const std::string& playerName) {
  if (!m_activeSave || !CanManageClub(*m_activeSave))
    return false;
  auto player = std::find_if(m_activeSave->roster.begin(), m_activeSave->roster.end(),
                             [&](const PlayerCareerState& p) { return p.name == playerName; });
  if (player == m_activeSave->roster.end())
    return false;
  return CommitLifecycle([&] {
    player->contract.yearsRemaining += 2;
    player->wage = player->wage * 110 / 100;
    player->contract.wage = player->wage;
    player->morale = std::min(100, player->morale + 20);
    AddEvent("contract", playerName + " signed a 2-year extension with a 10% wage increase.", 3,
             false);
  });
}

bool CareerDatabase::ToggleTransferList(const std::string& playerName) {
  if (!m_activeSave)
    return false;
  auto player = std::find_if(m_activeSave->roster.begin(), m_activeSave->roster.end(),
                             [&](const PlayerCareerState& p) { return p.name == playerName; });
  if (player == m_activeSave->roster.end())
    return false;
  const bool ownPlayer = m_activeSave->mode == CareerMode::PLAYER &&
                         m_activeSave->controlledEntityID > 0 &&
                         player->databaseID == m_activeSave->controlledEntityID;
  if (!CanManageClub(*m_activeSave) && !ownPlayer)
    return false;
  return CommitLifecycle([&] {
    player->contract.transferListed = !player->contract.transferListed;
    AddEvent("transfer",
             playerName + (player->contract.transferListed ? " requested a transfer."
                                                           : " withdrew the transfer request."),
             0, false);
  });
}

bool CareerDatabase::CanAdvanceSeason() const {
  return m_activeSave && m_activeSave->season.maxWeeks > 0 && !m_pendingFixture.hasFixture &&
         (m_activeSave->season.currentWeek > m_activeSave->season.maxWeeks ||
          m_activeSave->seasonWins + m_activeSave->seasonDraws + m_activeSave->seasonLosses >=
              m_activeSave->season.maxWeeks);
}

bool CareerDatabase::AdvanceSeason(int expectedSeason) {
  if (!CanAdvanceSeason() ||
      (expectedSeason >= 0 && expectedSeason != m_activeSave->season.currentSeason))
    return false;
  return CommitLifecycle([&] {
    CareerFinance::ProcessSeasonFinances(*m_activeSave);
    // Judge the season before the rollover resets its record and applies
    // prize-money and title reputation.
    CareerBoard::EvaluateBoardObjectives(*m_activeSave, *this);
    CareerSim::AdvanceSeason(*m_activeSave, *this, m_activeBids, m_transferTargets);
    CareerSponsors::GenerateSponsorOffers(*m_activeSave);
    CareerBoard::GenerateBoardObjectives(*m_activeSave);
    m_activeSave->season.fixtures.clear();
    m_activeSave->finance.transferBudget = m_activeSave->transferBudget;
    m_activeSave->finance.wageBudget = m_activeSave->wageBudget;
  });
}

void CareerDatabase::ReleasePlayer(const std::string& playerName, CareerActionSource source) {
  if (!m_activeSave || (source == CareerActionSource::USER && !CanManageClub(*m_activeSave))) {
    return;
  }
  if (m_activeSave)
    CareerTransfers::ReleasePlayer(*m_activeSave, *this, playerName);
}

void CareerDatabase::RecordMatchStats(const std::string& playerName, int goals, int assists) {
  if (m_activeSave)
    CareerSim::RecordMatchStats(*m_activeSave, playerName, goals, assists);
}

void CareerDatabase::PopulateTransferMarket() {
  if (m_activeSave)
    CareerTransfers::PopulateTransferMarket(m_transferTargets, m_activeSave->club.clubID);
}

std::vector<TransferTarget> CareerDatabase::GetTransferTargets() const {
  return m_transferTargets;
}

TransferBid CareerDatabase::PlaceBid(const std::string& playerName, long long bidAmount,
                                     int offeredWage, int contractYears,
                                     CareerActionSource source) {
  if (!m_activeSave || (source == CareerActionSource::USER && !CanManageClub(*m_activeSave))) {
    TransferBid denied;
    denied.status = BidStatus::REJECTED;
    return denied;
  }
  if (!m_activeSave)
    return TransferBid();
  return CareerTransfers::PlaceBid(*m_activeSave, *this, m_transferTargets, m_activeBids,
                                   playerName, bidAmount, offeredWage, contractYears);
}

void CareerDatabase::WithdrawBid(const std::string& playerName, CareerActionSource source) {
  if (!m_activeSave || (source == CareerActionSource::USER && !CanManageClub(*m_activeSave))) {
    return;
  }
  CareerTransfers::WithdrawBid(m_activeBids, playerName);
}

void CareerDatabase::ProcessPendingBids() {
  if (m_activeSave)
    CareerTransfers::ProcessPendingBids(*m_activeSave, *this, m_transferTargets, m_activeBids);
}

std::string CareerDatabase::GetBidStatusString(BidStatus status) const {
  return CareerTransfers::GetBidStatusString(status);
}

bool CareerDatabase::CompleteTransfer(const std::string& playerName, CareerActionSource source) {
  if (!m_activeSave || (source == CareerActionSource::USER && !CanManageClub(*m_activeSave))) {
    return false;
  }
  if (!m_activeSave)
    return false;
  return CareerTransfers::CompleteTransfer(*m_activeSave, *this, m_transferTargets, m_activeBids,
                                           playerName);
}

void CareerDatabase::InitializeOwnerData() {
  if (m_activeSave)
    CareerFinance::InitializeOwnerData(*m_activeSave);
}

void CareerDatabase::UpgradeStadium(int upgradeIndex, CareerActionSource source) {
  if (!m_activeSave || (source == CareerActionSource::USER && !CanManageClub(*m_activeSave))) {
    return;
  }
  if (m_activeSave)
    CareerFinance::UpgradeStadium(*m_activeSave, *this, upgradeIndex);
}

void CareerDatabase::RenameStadium(const std::string& newName, CareerActionSource source) {
  if (!m_activeSave || (source == CareerActionSource::USER && !CanManageClub(*m_activeSave))) {
    return;
  }
  if (m_activeSave)
    CareerFinance::RenameStadium(*m_activeSave, newName);
}

void CareerDatabase::RepairStadium(int amount, CareerActionSource source) {
  if (!m_activeSave || (source == CareerActionSource::USER && !CanManageClub(*m_activeSave))) {
    return;
  }
  if (m_activeSave)
    CareerFinance::RepairStadium(*m_activeSave, amount);
}

void CareerDatabase::SetTicketPrice(int price, CareerActionSource source) {
  if (!m_activeSave || (source == CareerActionSource::USER && !CanManageClub(*m_activeSave))) {
    return;
  }
  if (m_activeSave)
    CareerFinance::SetTicketPrice(*m_activeSave, price);
}

void CareerDatabase::HireStaff(const StaffMember& member, CareerActionSource source) {
  if (!m_activeSave || (source == CareerActionSource::USER && !CanManageClub(*m_activeSave))) {
    return;
  }
  if (m_activeSave)
    CareerStaff::HireStaff(*m_activeSave, member);
}

void CareerDatabase::FireStaff(const std::string& staffName, CareerActionSource source) {
  if (!m_activeSave || (source == CareerActionSource::USER && !CanManageClub(*m_activeSave))) {
    return;
  }
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

bool CareerDatabase::AcceptSponsorDeal(int dealIndex, CareerActionSource source) {
  if (!m_activeSave || (source == CareerActionSource::USER && !CanManageClub(*m_activeSave))) {
    return false;
  }
  return m_activeSave && CareerSponsors::AcceptSponsorDeal(*m_activeSave, *this, dealIndex);
}

void CareerDatabase::TerminateSponsorDeal(const std::string& sponsorName,
                                          CareerActionSource source) {
  if (!m_activeSave || (source == CareerActionSource::USER && !CanManageClub(*m_activeSave))) {
    return;
  }
  if (m_activeSave)
    CareerSponsors::TerminateSponsorDeal(*m_activeSave, *this, sponsorName);
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

void CareerDatabase::InvestInFanBase(long long amount, CareerActionSource source) {
  if (!m_activeSave || (source == CareerActionSource::USER && !CanManageClub(*m_activeSave))) {
    return;
  }
  if (m_activeSave)
    CareerFinance::InvestInFanBase(*m_activeSave, amount);
}

void CareerDatabase::InvestInPrestige(long long amount, CareerActionSource source) {
  if (!m_activeSave || (source == CareerActionSource::USER && !CanManageClub(*m_activeSave))) {
    return;
  }
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

}  // namespace blunted
