#include "career_database.hpp"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <fstream>
#include <functional>
#include <random>
#include <sstream>

namespace {

std::mt19937& CareerRng() {
  static std::mt19937 rng(static_cast<unsigned int>(std::time(nullptr)));
  return rng;
}

void SeedCareerRng(unsigned int seed) {
  CareerRng().seed(seed);
}

int ClampInt(int value, int minValue, int maxValue) {
  return std::max(minValue, std::min(maxValue, value));
}

int RandomInt(int minValue, int maxValue) {
  std::uniform_int_distribution<int> dist(minValue, maxValue);
  return dist(CareerRng());
}

// Exception-safe numeric parsing for save-file fields. A corrupt or
// hand-edited save must never crash the game on load; bad fields fall back to
// a default and parsing continues.
int SafeStoi(const std::string& s, int fallback = 0) {
  try {
    return std::stoi(s);
  } catch (const std::exception&) {
    return fallback;
  }
}

long long SafeStoll(const std::string& s, long long fallback = 0) {
  try {
    return std::stoll(s);
  } catch (const std::exception&) {
    return fallback;
  }
}

float SafeStof(const std::string& s, float fallback = 0.0f) {
  try {
    return std::stof(s);
  } catch (const std::exception&) {
    return fallback;
  }
}

// Splits a '|'-delimited record into its fields (empty fields preserved).
std::vector<std::string> SplitPipes(const std::string& s) {
  std::vector<std::string> tokens;
  size_t start = 0;
  while (true) {
    size_t bar = s.find('|', start);
    if (bar == std::string::npos) {
      tokens.push_back(s.substr(start));
      break;
    }
    tokens.push_back(s.substr(start, bar - start));
    start = bar + 1;
  }
  return tokens;
}

// Strips the field separator and newlines from free-text fields so they cannot
// corrupt the pipe-delimited, line-based save format.
std::string Sanitize(const std::string& s) {
  std::string out = s;
  for (char& c : out) {
    if (c == '|' || c == '\n' || c == '\r')
      c = ' ';
  }
  return out;
}

// Serializes a player to the pipe-delimited record used for roster, free agents
// and youth prospects.
std::string PlayerToRecord(const PlayerCareerState& p) {
  std::ostringstream os;
  os << Sanitize(p.name) << "|" << Sanitize(p.position) << "|" << p.age << "|" << p.ovr << "|"
     << p.pot << "|" << p.value << "|" << p.wage << "|" << p.morale << "|" << p.matchForm << "|"
     << p.fitness << "|" << p.careerGoals << "|" << p.careerAssists << "|" << p.matchesPlayed;
  return os.str();
}

// Parses a player record. Trailing fields are optional so older 7-field saves
// still load, with newer fields keeping their struct defaults.
PlayerCareerState PlayerFromRecord(const std::string& val) {
  std::vector<std::string> t = SplitPipes(val);
  PlayerCareerState p;
  if (t.size() > 0)
    p.name = t[0];
  if (t.size() > 1)
    p.position = t[1];
  if (t.size() > 2)
    p.age = SafeStoi(t[2]);
  if (t.size() > 3)
    p.ovr = static_cast<int>(SafeStof(t[3]));
  if (t.size() > 4)
    p.pot = static_cast<int>(SafeStof(t[4]));
  if (t.size() > 5)
    p.value = SafeStoll(t[5]);
  if (t.size() > 6)
    p.wage = SafeStoll(t[6]);
  if (t.size() > 7)
    p.morale = SafeStoi(t[7], p.morale);
  if (t.size() > 8)
    p.matchForm = SafeStoi(t[8], p.matchForm);
  if (t.size() > 9)
    p.fitness = SafeStoi(t[9], p.fitness);
  if (t.size() > 10)
    p.careerGoals = SafeStoi(t[10]);
  if (t.size() > 11)
    p.careerAssists = SafeStoi(t[11]);
  if (t.size() > 12)
    p.matchesPlayed = SafeStoi(t[12]);
  p.preferredPosition = p.position;
  return p;
}

}  // namespace

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
  if (!LoadFromFile(path))
    return false;
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
  InitializeOwnerData();
  GenerateBoardObjectives();
  GenerateSponsorOffers();
  return SaveCareerData();
}

bool CareerDatabase::SaveCareerData() {
  if (!m_activeSave || m_saveDirectory.empty())
    return false;
  std::string path = m_saveDirectory + "/career.save";
  return SaveToFile(path);
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
  if (!m_activeSave)
    return;
  auto it =
      std::find_if(m_activeSave->freeAgents.begin(), m_activeSave->freeAgents.end(),
                   [&playerName](const PlayerCareerState& p) { return p.name == playerName; });
  if (it == m_activeSave->freeAgents.end())
    return;
  if (m_activeSave->wageBudget < it->wage) {
    AddEvent("financial", "Failed to sign " + playerName + " due to wage budget limits", -1, false);
    return;
  }
  m_activeSave->wageBudget -= it->wage;
  m_activeSave->finance.wageBudget = m_activeSave->wageBudget;
  m_activeSave->roster.push_back(*it);
  m_activeSave->freeAgents.erase(it);
  AddEvent("recruitment", "Signed free agent " + playerName, 2, false);
  ModifyBoardConfidence(1);
}

void CareerDatabase::ScoutYouthPlayer() {
  if (!m_activeSave)
    return;
  int scoutCost = 50000 * m_activeSave->scoutingNetworkLevel;
  if (m_activeSave->transferBudget < scoutCost)
    return;
  m_activeSave->transferBudget -= scoutCost;
  m_activeSave->finance.transferBudget = m_activeSave->transferBudget;

  static const std::vector<std::string> firstNames = {"Leo", "Kai", "Ravi", "Mateo", "Yuki"};
  static const std::vector<std::string> lastNames = {"Martinez", "Tanaka", "Okafor", "Silva",
                                                     "Kim"};
  static const std::vector<std::string> positions = {"CF", "CM", "CB", "AM", "GK"};
  static std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution<int> fd(0, firstNames.size() - 1);
  std::uniform_int_distribution<int> ld(0, lastNames.size() - 1);
  std::uniform_int_distribution<int> pd(0, positions.size() - 1);
  std::uniform_int_distribution<int> ad(15, 18);

  PlayerCareerState youth;
  youth.name = firstNames[fd(rng)] + " " + lastNames[ld(rng)];
  youth.position = positions[pd(rng)];
  youth.preferredPosition = youth.position;
  youth.age = ad(rng);
  youth.ovr = 50 + (rng() % 10);
  youth.pot = 70 + (rng() % 15);
  youth.wage = 500;
  youth.value = 100000;
  youth.isYouth = true;
  m_activeSave->youthAcademy.push_back(youth);
  AddEvent("scouting", "Scout returned with prospect: " + youth.name + " (" + youth.position + ")",
           0, false);
}

void CareerDatabase::PromoteYouthPlayer(const std::string& playerName) {
  if (!m_activeSave)
    return;
  auto it =
      std::find_if(m_activeSave->youthAcademy.begin(), m_activeSave->youthAcademy.end(),
                   [&playerName](const PlayerCareerState& p) { return p.name == playerName; });
  if (it == m_activeSave->youthAcademy.end())
    return;
  PlayerCareerState promoted = *it;
  promoted.contract.yearsRemaining = 4;
  promoted.isYouth = false;
  promoted.morale = 85;
  promoted.matchForm = 55;
  m_activeSave->roster.push_back(promoted);
  m_activeSave->youthAcademy.erase(it);
  AddEvent("academy", "Promoted academy player " + playerName + " to senior squad.", 1, false);
}

void CareerDatabase::ModifyBudget(long long transferDelta, long long wageDelta) {
  if (!m_activeSave)
    return;
  m_activeSave->transferBudget += transferDelta;
  m_activeSave->wageBudget += wageDelta;
  m_activeSave->finance.transferBudget = m_activeSave->transferBudget;
  m_activeSave->finance.wageBudget = m_activeSave->wageBudget;
}

void CareerDatabase::ModifyBoardConfidence(int delta) {
  if (!m_activeSave)
    return;
  m_activeSave->boardConfidence = std::max(0, std::min(100, m_activeSave->boardConfidence + delta));
  m_activeSave->board.confidence = m_activeSave->boardConfidence;
}

bool CareerDatabase::TrainSquad() {
  if (!m_activeSave || m_activeSave->trainingPoints <= 0)
    return false;
  m_activeSave->trainingPoints--;
  for (auto& player : m_activeSave->roster)
    player.matchForm = std::min(100, player.matchForm + 3);
  AddEvent("training", "Conducted squad training session", 1, false);
  return true;
}

bool CareerDatabase::TrainFocus(const std::string& focusArea) {
  if (!m_activeSave || m_activeSave->trainingPoints <= 0)
    return false;
  m_activeSave->trainingPoints--;
  int playersImproved = 0;
  for (auto& player : m_activeSave->roster) {
    if ((focusArea == "Attacking" || focusArea == "Shooting") &&
        (player.preferredPosition == "CF" || player.preferredPosition == "AM")) {
      player.ovr++;
      playersImproved++;
    } else if (focusArea == "Defending" &&
               (player.preferredPosition == "CB" || player.preferredPosition == "LB" ||
                player.preferredPosition == "RB")) {
      player.ovr++;
      playersImproved++;
    }
    player.matchForm = std::min(100, player.matchForm + 3);
  }
  AddEvent("training",
           "Focused training on " + focusArea + " (" + std::to_string(playersImproved) +
               " players improved)",
           1, false);
  return true;
}

void CareerDatabase::SetStrategy(const std::string& strategy) {
  if (!m_activeSave)
    return;
  m_activeSave->activeStrategy = strategy;
  AddEvent("strategy", "Changed team strategy to " + strategy, 0, false);
}

int CareerDatabase::GetReputation() const {
  return m_activeSave ? m_activeSave->reputation : 0;
}

std::string CareerDatabase::GetReputationStatus() const {
  if (!m_activeSave)
    return "Unknown";
  int rep = m_activeSave->reputation;
  if (rep >= 80)
    return "Legendary";
  if (rep >= 50)
    return "Respected";
  if (rep >= 20)
    return "Known";
  if (rep >= -20)
    return "Unproven";
  if (rep >= -50)
    return "Controversial";
  return "Notorious";
}

std::string CareerDatabase::GetMoraleString(int morale) const {
  if (morale >= 80)
    return "Happy";
  if (morale >= 40)
    return "Content";
  return "Unhappy";
}

std::string CareerDatabase::GetFormString(int form) const {
  if (form >= 80)
    return "Excellent";
  if (form >= 40)
    return "Good";
  return "Poor";
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
  int growthChance = 15;
  if (player.age <= 21)
    growthChance = 55;
  else if (player.age <= 24)
    growthChance = 35;
  else if (player.age >= 31)
    growthChance = 8;

  if (player.matchForm >= 75)
    growthChance += 10;
  if (player.morale >= 75)
    growthChance += 5;

  if (RandomInt(1, 100) <= growthChance) {
    if (player.ovr < player.pot) {
      player.ovr = std::min(player.ovr + 1, player.pot);
    } else if (player.age >= 32 && RandomInt(1, 100) <= 35) {
      player.ovr = std::max(40, player.ovr - 1);
    }
  }

  player.matchForm = ClampInt(player.matchForm - RandomInt(2, 8), 0, 100);
  player.morale = ClampInt(player.morale + RandomInt(-3, 4), 0, 100);
  player.fitness = ClampInt(player.fitness - RandomInt(0, 5), 55, 100);
}

void CareerDatabase::UpdatePlayerValue(PlayerCareerState& player) {
  long long ageModifier = 120;
  if (player.age >= 30)
    ageModifier = 85;
  else if (player.age <= 21)
    ageModifier = 135;

  long long formModifier = 80 + ClampInt(player.matchForm, 0, 100) / 5;
  long long potentialModifier = 100 + std::max(0, player.pot - player.ovr);
  long long baseValue = static_cast<long long>(player.ovr) * player.ovr * 4000;
  player.value =
      std::max(50000LL, (baseValue * ageModifier * formModifier * potentialModifier) / 1200000LL);
  player.wage = std::max(500LL, player.value / 1200LL);
}

int CareerDatabase::EstimateLeaguePosition(int wins, int draws, int losses) {
  const int played = wins + draws + losses;
  if (played <= 0)
    return 10;
  const int points = wins * 3 + draws;
  // Project points onto a 38-match season, then map onto finish bands.
  const float projected = (static_cast<float>(points) / static_cast<float>(played)) * 38.0f;
  if (projected >= 90.0f)
    return 1;
  if (projected >= 78.0f)
    return 2;
  if (projected >= 70.0f)
    return 4;
  if (projected >= 60.0f)
    return 7;
  if (projected >= 52.0f)
    return 10;
  if (projected >= 45.0f)
    return 12;
  if (projected >= 38.0f)
    return 15;
  if (projected >= 30.0f)
    return 17;
  return 19;
}

void CareerDatabase::AdvanceSeason() {
  if (!m_activeSave)
    return;

  SeasonRecord record;
  record.season = m_activeSave->season.currentSeason;
  record.teamID = m_activeSave->club.clubID;
  if (m_activeSave->seasonWins > 0 || m_activeSave->seasonDraws > 0 ||
      m_activeSave->seasonLosses > 0) {
    record.wins = m_activeSave->seasonWins;
    record.draws = m_activeSave->seasonDraws;
    record.losses = m_activeSave->seasonLosses;
    record.goalsFor = m_activeSave->seasonGoalsFor;
    record.goalsAgainst = m_activeSave->seasonGoalsAgainst;
  } else {
    record.wins = RandomInt(8, 28);
    record.draws = RandomInt(4, 12);
    record.losses = std::max(0, 38 - record.wins - record.draws);
    record.goalsFor = RandomInt(30, 85);
    record.goalsAgainst = RandomInt(20, 70);
  }
  record.leaguePosition = EstimateLeaguePosition(record.wins, record.draws, record.losses);
  record.wonTitle = (record.leaguePosition == 1);
  m_activeSave->history.push_back(record);

  for (auto& player : m_activeSave->roster) {
    player.age++;
    if (player.contract.yearsRemaining > 0)
      player.contract.yearsRemaining--;
    ProcessPlayerGrowth(player);
    UpdatePlayerValue(player);
    player.matchesPlayed = 0;
    player.careerGoals = std::max(0, player.careerGoals);
    player.careerAssists = std::max(0, player.careerAssists);
  }

  for (auto& player : m_activeSave->staff) {
    if (player.contractYearsRemaining > 0)
      player.contractYearsRemaining--;
  }
  m_activeSave->staff.erase(
      std::remove_if(m_activeSave->staff.begin(), m_activeSave->staff.end(),
                     [](const StaffMember& member) { return member.contractYearsRemaining <= 0; }),
      m_activeSave->staff.end());

  for (auto& sponsor : m_activeSave->activeSponsors) {
    if (sponsor.yearsRemaining > 0)
      sponsor.yearsRemaining--;
  }
  m_activeSave->activeSponsors.erase(
      std::remove_if(m_activeSave->activeSponsors.begin(), m_activeSave->activeSponsors.end(),
                     [](const SponsorDeal& sponsor) { return sponsor.yearsRemaining <= 0; }),
      m_activeSave->activeSponsors.end());

  for (auto& upgrade : m_activeSave->stadium.upgrades) {
    if (upgrade.seasonsRemaining > 0) {
      upgrade.seasonsRemaining--;
      if (upgrade.seasonsRemaining == 0) {
        m_activeSave->stadium.capacity += upgrade.capacityIncrease;
        m_activeSave->stadium.matchDayRevenue += upgrade.revenueBonus;
        AddEvent("stadium", "Completed stadium upgrade: " + upgrade.name, 1, false);
      }
    }
  }

  m_activeSave->season.currentSeason++;
  m_activeSave->currentSeason = m_activeSave->season.currentSeason;
  m_activeSave->season.currentWeek = 1;
  m_activeSave->season.inPreseason = true;
  m_activeSave->season.transferWindowOpen = true;
  m_activeSave->trainingPoints = 10;
  m_activeSave->availableSponsorOffers.clear();
  m_activeBids.clear();
  m_transferTargets.clear();
  m_activeSave->seasonWins = 0;
  m_activeSave->seasonDraws = 0;
  m_activeSave->seasonLosses = 0;
  m_activeSave->seasonGoalsFor = 0;
  m_activeSave->seasonGoalsAgainst = 0;
  AddEvent("season", "Advanced to season " + std::to_string(m_activeSave->season.currentSeason), 1,
           true);
  SaveCareerData();
}

void CareerDatabase::ReleasePlayer(const std::string& playerName) {
  if (!m_activeSave)
    return;
  auto it = std::find_if(
      m_activeSave->roster.begin(), m_activeSave->roster.end(),
      [&playerName](const PlayerCareerState& player) { return player.name == playerName; });
  if (it == m_activeSave->roster.end())
    return;

  m_activeSave->wageBudget += it->wage;
  m_activeSave->finance.wageBudget = m_activeSave->wageBudget;
  PlayerCareerState released = *it;
  released.transferStatus = TransferStatus::NONE;
  m_activeSave->freeAgents.push_back(released);
  m_activeSave->roster.erase(it);
  AddEvent("squad", "Released player " + playerName, -1, false);
  ModifyBoardConfidence(-1);
}

void CareerDatabase::RecordMatchStats(const std::string& playerName, int goals, int assists) {
  if (!m_activeSave)
    return;
  auto it = std::find_if(
      m_activeSave->roster.begin(), m_activeSave->roster.end(),
      [&playerName](const PlayerCareerState& player) { return player.name == playerName; });
  if (it == m_activeSave->roster.end())
    return;

  it->matchesPlayed++;
  it->careerGoals += std::max(0, goals);
  it->careerAssists += std::max(0, assists);
  // Keep per-match form gains modest so a hot streak cannot snowball the
  // whole squad's simulated strength across a 38-game season.
  it->matchForm = ClampInt(it->matchForm + goals * 3 + assists * 2 + 1, 0, 100);
  it->morale = ClampInt(it->morale + goals * 2 + assists * 1 + 1, 0, 100);
  UpdatePlayerValue(*it);
}

void CareerDatabase::PopulateTransferMarket() {
  if (!m_activeSave)
    return;
  if (!m_transferTargets.empty())
    return;

  static const std::vector<std::string> firstNames = {"Alex", "Bruno", "Marco", "Noah",
                                                      "Theo", "Rayan", "Luis",  "Evan"};
  static const std::vector<std::string> lastNames = {"Silva",  "Rossi",   "Meyer", "Costa",
                                                     "Santos", "Fischer", "Lopez", "Ibrahim"};
  static const std::vector<std::string> positions = {"GK", "CB", "LB", "RB", "DM",
                                                     "CM", "AM", "CF", "ST"};

  m_transferTargets.clear();
  for (int i = 0; i < 18; ++i) {
    TransferTarget target;
    target.name = firstNames[RandomInt(0, static_cast<int>(firstNames.size()) - 1)] + " " +
                  lastNames[RandomInt(0, static_cast<int>(lastNames.size()) - 1)] + " " +
                  std::to_string(i + 1);
    target.preferredPosition = positions[RandomInt(0, static_cast<int>(positions.size()) - 1)];
    target.age = RandomInt(18, 31);
    target.overallRating = RandomInt(62, 84);
    target.potentialRating =
        std::max(target.overallRating, target.overallRating + RandomInt(1, 10));
    target.value = static_cast<long long>(target.overallRating) * target.overallRating * 4500;
    target.askingPrice = target.value + target.value * RandomInt(10, 30) / 100;
    target.wage = std::max(1500LL, target.value / 1400LL);
    target.teamID = 1000 + i;
    target.isListed = true;
    m_transferTargets.push_back(target);
  }
}

std::vector<TransferTarget> CareerDatabase::GetTransferTargets() const {
  return m_transferTargets;
}

TransferBid CareerDatabase::PlaceBid(const std::string& playerName, long long bidAmount,
                                     int offeredWage, int contractYears) {
  TransferBid bid;
  bid.playerName = playerName;
  bid.bidAmount = bidAmount;
  bid.offeredWage = offeredWage;
  bid.contractYears = contractYears;
  bid.agentFee = std::max(25000LL, bidAmount / 20);
  bid.status = BidStatus::REJECTED;

  if (!m_activeSave)
    return bid;

  auto targetIt = std::find_if(
      m_transferTargets.begin(), m_transferTargets.end(),
      [&playerName](const TransferTarget& target) { return target.name == playerName; });
  if (targetIt == m_transferTargets.end())
    return bid;

  long long totalCost = bidAmount + bid.agentFee;
  if (totalCost > m_activeSave->transferBudget || offeredWage > m_activeSave->wageBudget) {
    AddEvent("transfer", "Bid rejected for " + playerName + " due to budget limits", -1, false);
    return bid;
  }

  bid.status = BidStatus::PENDING;
  m_activeBids.push_back(bid);
  AddEvent("transfer", "Placed bid for " + playerName, 0, false);
  return bid;
}

void CareerDatabase::WithdrawBid(const std::string& playerName) {
  auto it =
      std::find_if(m_activeBids.begin(), m_activeBids.end(),
                   [&playerName](const TransferBid& bid) { return bid.playerName == playerName; });
  if (it == m_activeBids.end())
    return;
  it->status = BidStatus::WITHDRAWN;
}

void CareerDatabase::ProcessPendingBids() {
  if (!m_activeSave)
    return;

  for (auto& bid : m_activeBids) {
    if (bid.status != BidStatus::PENDING)
      continue;

    auto targetIt = std::find_if(
        m_transferTargets.begin(), m_transferTargets.end(),
        [&bid](const TransferTarget& target) { return target.name == bid.playerName; });
    if (targetIt == m_transferTargets.end()) {
      bid.status = BidStatus::REJECTED;
      continue;
    }

    long long requiredPrice = targetIt->askingPrice;
    if (bid.negotiationRounds >= 2)
      requiredPrice = requiredPrice * 90 / 100;

    if (bid.bidAmount >= requiredPrice && bid.offeredWage >= targetIt->wage * 9 / 10) {
      bid.status = BidStatus::ACCEPTED;
      AddEvent("transfer", "Bid accepted for " + bid.playerName, 1, false);
    } else {
      bid.status = BidStatus::REJECTED;
      AddEvent("transfer", "Bid rejected for " + bid.playerName, -1, false);
    }
  }
}

std::string CareerDatabase::GetBidStatusString(BidStatus status) const {
  switch (status) {
    case BidStatus::PENDING:
      return "Pending";
    case BidStatus::ACCEPTED:
      return "Accepted";
    case BidStatus::REJECTED:
      return "Rejected";
    case BidStatus::WITHDRAWN:
      return "Withdrawn";
  }
  return "Pending";
}

bool CareerDatabase::CompleteTransfer(const std::string& playerName) {
  if (!m_activeSave)
    return false;

  auto bidIt =
      std::find_if(m_activeBids.begin(), m_activeBids.end(), [&playerName](const TransferBid& bid) {
        return bid.playerName == playerName && bid.status == BidStatus::ACCEPTED;
      });
  if (bidIt == m_activeBids.end())
    return false;

  auto targetIt = std::find_if(
      m_transferTargets.begin(), m_transferTargets.end(),
      [&playerName](const TransferTarget& target) { return target.name == playerName; });
  if (targetIt == m_transferTargets.end())
    return false;

  long long totalCost = bidIt->bidAmount + bidIt->agentFee;
  if (totalCost > m_activeSave->transferBudget || bidIt->offeredWage > m_activeSave->wageBudget)
    return false;

  PlayerCareerState player;
  player.name = targetIt->name;
  player.preferredPosition = targetIt->preferredPosition;
  player.position = targetIt->preferredPosition;
  player.ovr = targetIt->overallRating;
  player.pot = targetIt->potentialRating;
  player.age = targetIt->age;
  player.value = targetIt->value;
  player.wage = bidIt->offeredWage;
  player.contract.yearsRemaining = bidIt->contractYears;
  player.contract.transferListed = false;
  player.morale = 70;
  player.fitness = 95;
  player.matchForm = 60;

  m_activeSave->transferBudget -= totalCost;
  m_activeSave->wageBudget -= bidIt->offeredWage;
  m_activeSave->finance.transferBudget = m_activeSave->transferBudget;
  m_activeSave->finance.wageBudget = m_activeSave->wageBudget;
  m_activeSave->finances.transferSpending += bidIt->bidAmount;
  m_activeSave->roster.push_back(player);

  m_transferTargets.erase(targetIt);
  m_activeBids.erase(std::remove_if(m_activeBids.begin(), m_activeBids.end(),
                                    [&playerName](const TransferBid& bid) {
                                      return bid.playerName == playerName;
                                    }),
                     m_activeBids.end());

  AddEvent("transfer", "Completed transfer for " + playerName, 2, true);
  ModifyBoardConfidence(1);
  return true;
}

void CareerDatabase::InitializeOwnerData() {
  if (!m_activeSave)
    return;

  if (m_activeSave->stadium.name.empty())
    m_activeSave->stadium.name = m_activeSave->name + " Stadium";
  if (m_activeSave->stadium.availableUpgrades.empty()) {
    m_activeSave->stadium.availableUpgrades.push_back(
        {"Expand North Stand", "Adds a new upper tier.", 12000000, 2, 2, 8000, 1500000});
    m_activeSave->stadium.availableUpgrades.push_back(
        {"Hospitality Suites", "Improves VIP match-day revenue.", 6500000, 1, 1, 0, 2200000});
    m_activeSave->stadium.availableUpgrades.push_back({"Training Complex",
                                                       "Supports player development and prestige.",
                                                       9000000, 2, 2, 0, 1000000});
  }

  if (m_activeSave->staff.empty()) {
    m_activeSave->staff.push_back(StaffMember("Avery Cole", "Assistant Coach", 68, 850000, 3));
    m_activeSave->staff.push_back(StaffMember("Nina Petrov", "Head Scout", 72, 950000, 3));
    m_activeSave->staff.push_back(StaffMember("Marcus Reed", "Physio", 70, 780000, 2));
  }

  long long playerWages = 0;
  for (const auto& player : m_activeSave->roster)
    playerWages += player.wage;
  long long staffWages = 0;
  for (const auto& member : m_activeSave->staff)
    staffWages += member.salary;

  m_activeSave->finances.playerWages = playerWages;
  m_activeSave->finances.staffWages = staffWages;
  m_activeSave->finances.matchDayIncome = m_activeSave->stadium.matchDayRevenue;
  m_activeSave->finances.sponsorIncome = 0;
  m_activeSave->finances.stadiumCosts = m_activeSave->stadium.maintenanceCost;
  m_activeSave->finances.totalRevenue =
      m_activeSave->finances.matchDayIncome + m_activeSave->finances.sponsorIncome +
      m_activeSave->finances.merchandiseIncome + m_activeSave->finances.tvRevenue +
      m_activeSave->finances.transferIncome;
  m_activeSave->finances.totalExpenses =
      m_activeSave->finances.playerWages + m_activeSave->finances.staffWages +
      m_activeSave->finances.stadiumCosts + m_activeSave->finances.transferSpending;
}

void CareerDatabase::UpgradeStadium(int upgradeIndex) {
  if (!m_activeSave)
    return;
  if (upgradeIndex < 0 ||
      upgradeIndex >= static_cast<int>(m_activeSave->stadium.availableUpgrades.size()))
    return;

  const StadiumUpgrade upgrade = m_activeSave->stadium.availableUpgrades[upgradeIndex];
  if (upgrade.cost > m_activeSave->finances.netWorth)
    return;

  m_activeSave->finances.netWorth -= upgrade.cost;
  m_activeSave->finances.stadiumCosts += upgrade.cost / std::max(1, upgrade.buildTimeSeasons);
  m_activeSave->stadium.upgrades.push_back(upgrade);
  m_activeSave->stadium.availableUpgrades.erase(m_activeSave->stadium.availableUpgrades.begin() +
                                                upgradeIndex);
  AddEvent("stadium", "Started stadium upgrade: " + upgrade.name, 1, false);
}

void CareerDatabase::RenameStadium(const std::string& newName) {
  if (!m_activeSave || newName.empty())
    return;
  m_activeSave->stadium.name = newName;
}

void CareerDatabase::RepairStadium(int amount) {
  if (!m_activeSave)
    return;
  int repairAmount = std::max(1, amount);
  long long repairCost = 50000LL * std::max(1, repairAmount / 10);
  if (repairCost > m_activeSave->finances.netWorth)
    return;

  m_activeSave->finances.netWorth -= repairCost;
  m_activeSave->stadium.condition =
      ClampInt(m_activeSave->stadium.condition + repairAmount, 0, 100);
  m_activeSave->stadium.fanSatisfaction =
      ClampInt(m_activeSave->stadium.fanSatisfaction + repairAmount / 2, 0, 100);
}

void CareerDatabase::SetTicketPrice(int price) {
  if (!m_activeSave)
    return;
  m_activeSave->finances.ticketPrice = ClampInt(price, 10, 200);
  int delta = m_activeSave->finances.ticketPrice - 40;
  m_activeSave->fanBase = ClampInt(m_activeSave->fanBase - (delta / 8), 10, 100);
  m_activeSave->stadium.fanSatisfaction =
      ClampInt(m_activeSave->stadium.fanSatisfaction - (delta / 4), 0, 100);
}

void CareerDatabase::HireStaff(const StaffMember& member) {
  if (!m_activeSave)
    return;
  if (member.salary > m_activeSave->finances.netWorth)
    return;
  m_activeSave->staff.push_back(member);
  m_activeSave->finances.netWorth -= member.salary;
  m_activeSave->finances.staffWages += member.salary;
}

void CareerDatabase::FireStaff(const std::string& staffName) {
  if (!m_activeSave)
    return;
  auto it =
      std::find_if(m_activeSave->staff.begin(), m_activeSave->staff.end(),
                   [&staffName](const StaffMember& member) { return member.name == staffName; });
  if (it == m_activeSave->staff.end())
    return;
  m_activeSave->finances.staffWages = std::max(0LL, m_activeSave->finances.staffWages - it->salary);
  m_activeSave->staff.erase(it);
  ModifyBoardConfidence(-1);
}

void CareerDatabase::GenerateStaffCandidates(std::vector<StaffMember>& candidates) {
  candidates.clear();
  static const std::vector<std::string> firstNames = {
      "Jordan", "Sofia",  "Callum", "Kei",   "Marta",  "Henrik", "Lena",
      "Omar",   "Priya",  "Diego",  "Alina", "Samuel", "Yuki",   "Fabio",
      "Rosa",   "Thomas", "Aisha",  "Liam",  "Nadia",  "Andre",  "Clara"};
  static const std::vector<std::string> lastNames = {
      "Blake",  "Marin",     "Hart",    "Tanaka",    "Costa",  "Lindqvist", "Petrov", "Ali",
      "Sharma", "Fernandez", "Novak",   "Eriksson",  "Rossi",  "Muller",    "Chen",   "Dubois",
      "Park",   "Santos",    "Fischer", "Johansson", "Moreau", "Torres"};
  static const std::vector<std::string> roles = {
      "Assistant Coach", "Head Scout",       "Fitness Coach", "Goalkeeping Coach",
      "Physio",          "Tactical Analyst", "Youth Coach",   "Set Piece Specialist"};

  std::vector<int> usedIndices;
  for (int i = 0; i < 5; ++i) {
    int nameIdx;
    do {
      nameIdx = RandomInt(0, static_cast<int>(firstNames.size()) - 1);
    } while (std::find(usedIndices.begin(), usedIndices.end(), nameIdx) != usedIndices.end());
    usedIndices.push_back(nameIdx);
    int roleIdx = RandomInt(0, static_cast<int>(roles.size()) - 1);
    candidates.push_back(StaffMember(firstNames[nameIdx] + " " + lastNames[nameIdx], roles[roleIdx],
                                     RandomInt(58, 88), RandomInt(500000, 1500000),
                                     RandomInt(2, 4)));
  }
}

void CareerDatabase::GenerateSponsorOffers() {
  if (!m_activeSave)
    return;
  m_activeSave->availableSponsorOffers.clear();
  m_activeSave->availableSponsorOffers.push_back(
      SponsorDeal("Vertex", "Shirt Sponsor", 4500000, 2, 40));
  m_activeSave->availableSponsorOffers.push_back(
      SponsorDeal("Apex Air", "Sleeve Sponsor", 2500000, 3, 55));
  m_activeSave->availableSponsorOffers.push_back(
      SponsorDeal("Northbank", "Training Kit", 1800000, 2, 30));
}

bool CareerDatabase::AcceptSponsorDeal(int dealIndex) {
  if (!m_activeSave)
    return false;
  if (dealIndex < 0 || dealIndex >= static_cast<int>(m_activeSave->availableSponsorOffers.size()))
    return false;

  const SponsorDeal deal = m_activeSave->availableSponsorOffers[dealIndex];
  if (m_activeSave->reputation < deal.reputationRequirement)
    return false;

  m_activeSave->activeSponsors.push_back(deal);
  m_activeSave->finances.sponsorIncome += deal.annualRevenue;
  m_activeSave->finances.netWorth += deal.annualRevenue;
  m_activeSave->availableSponsorOffers.erase(m_activeSave->availableSponsorOffers.begin() +
                                             dealIndex);
  AddEvent("commercial", "Signed sponsor deal with " + deal.sponsorName, 1, false);
  return true;
}

void CareerDatabase::TerminateSponsorDeal(const std::string& sponsorName) {
  if (!m_activeSave)
    return;
  auto it = std::find_if(
      m_activeSave->activeSponsors.begin(), m_activeSave->activeSponsors.end(),
      [&sponsorName](const SponsorDeal& sponsor) { return sponsor.sponsorName == sponsorName; });
  if (it == m_activeSave->activeSponsors.end())
    return;

  m_activeSave->finances.sponsorIncome =
      std::max(0LL, m_activeSave->finances.sponsorIncome - it->annualRevenue);
  m_activeSave->activeSponsors.erase(it);
  ModifyBoardConfidence(-2);
}

void CareerDatabase::ProcessSeasonFinances() {
  if (!m_activeSave)
    return;

  InitializeOwnerData();
  long long seasonMatchRevenue = m_activeSave->stadium.matchDayRevenue * 19LL;
  long long sponsorRevenue = 0;
  for (const auto& sponsor : m_activeSave->activeSponsors)
    sponsorRevenue += sponsor.annualRevenue;
  long long merchandiseRevenue = static_cast<long long>(m_activeSave->fanBase) * 90000LL;

  m_activeSave->finances.matchDayIncome = seasonMatchRevenue;
  m_activeSave->finances.sponsorIncome = sponsorRevenue;
  m_activeSave->finances.merchandiseIncome = merchandiseRevenue;
  m_activeSave->finances.stadiumCosts = m_activeSave->stadium.maintenanceCost;
  m_activeSave->finances.totalRevenue = seasonMatchRevenue + sponsorRevenue + merchandiseRevenue +
                                        m_activeSave->finances.tvRevenue +
                                        m_activeSave->finances.transferIncome;
  m_activeSave->finances.totalExpenses =
      m_activeSave->finances.playerWages + m_activeSave->finances.staffWages +
      m_activeSave->finances.stadiumCosts + m_activeSave->finances.transferSpending;
  long long profit = GetSeasonProfit();
  m_activeSave->finances.netWorth = std::max(0LL, m_activeSave->finances.netWorth + profit);
  m_activeSave->transferBudget = std::max(0LL, m_activeSave->transferBudget + profit / 2);
  m_activeSave->finance.transferBudget = m_activeSave->transferBudget;
}

long long CareerDatabase::GetSeasonProfit() const {
  if (!m_activeSave)
    return 0;
  return m_activeSave->finances.totalRevenue - m_activeSave->finances.totalExpenses;
}

std::string CareerDatabase::GetFinancialHealthString() const {
  if (!m_activeSave)
    return "Unknown";
  long long profit = GetSeasonProfit();
  if (m_activeSave->finances.netWorth >= 150000000 && profit >= 0)
    return "Elite";
  if (m_activeSave->finances.netWorth >= 75000000 && profit >= -5000000)
    return "Stable";
  if (m_activeSave->finances.netWorth >= 25000000)
    return "Tight";
  return "Critical";
}

void CareerDatabase::GenerateBoardObjectives() {
  if (!m_activeSave)
    return;
  m_activeSave->boardObjectives.clear();
  m_activeSave->boardObjectives.push_back({OwnerObjectiveType::FINANCIAL_STABILITY,
                                           "Finish the season with positive net profit", false, 4,
                                           -8});
  m_activeSave->boardObjectives.push_back(
      {OwnerObjectiveType::GROW_FANBASE, "Grow the fan base to at least 60k", false, 3, -6});
  m_activeSave->boardObjectives.push_back(
      {OwnerObjectiveType::PROMOTION, "Reach a top-half finish in the league", false, 5, -10});
}

void CareerDatabase::EvaluateBoardObjectives() {
  if (!m_activeSave)
    return;

  // Prefer live W/D/L while the season is still open. After AdvanceSeason the
  // counters are cleared and history.back() holds the season that just closed.
  int finishPosition = 20;
  const int liveMatches =
      m_activeSave->seasonWins + m_activeSave->seasonDraws + m_activeSave->seasonLosses;
  if (liveMatches > 0) {
    finishPosition = EstimateLeaguePosition(m_activeSave->seasonWins, m_activeSave->seasonDraws,
                                            m_activeSave->seasonLosses);
  } else if (!m_activeSave->history.empty()) {
    finishPosition = m_activeSave->history.back().leaguePosition;
  }

  for (auto& objective : m_activeSave->boardObjectives) {
    bool completed = false;
    switch (objective.type) {
      case OwnerObjectiveType::FINANCIAL_STABILITY:
        completed = GetSeasonProfit() >= 0;
        break;
      case OwnerObjectiveType::GROW_FANBASE:
        completed = m_activeSave->fanBase >= 60;
        break;
      case OwnerObjectiveType::PROMOTION:
        completed = finishPosition <= 10;
        break;
      case OwnerObjectiveType::AVOID_RELEGATION:
        completed = finishPosition <= 17;
        break;
      case OwnerObjectiveType::WIN_TITLE:
        completed = finishPosition == 1;
        break;
    }

    objective.completed = completed;
    if (completed) {
      m_activeSave->reputation =
          ClampInt(m_activeSave->reputation + objective.reputationReward, 0, 100);
      m_activeSave->club.reputation = m_activeSave->reputation;
      ModifyBoardConfidence(3);
    } else {
      ModifyBoardConfidence(objective.confidencePenalty);
    }
  }
}

void CareerDatabase::InvestInFanBase(long long amount) {
  if (!m_activeSave || amount <= 0 || amount > m_activeSave->finances.netWorth)
    return;
  m_activeSave->finances.netWorth -= amount;
  m_activeSave->fanBase =
      ClampInt(m_activeSave->fanBase + static_cast<int>(amount / 1000000LL) * 2, 0, 100);
  m_activeSave->stadium.fanSatisfaction =
      ClampInt(m_activeSave->stadium.fanSatisfaction + 5, 0, 100);
}

void CareerDatabase::InvestInPrestige(long long amount) {
  if (!m_activeSave || amount <= 0 || amount > m_activeSave->finances.netWorth)
    return;
  m_activeSave->finances.netWorth -= amount;
  m_activeSave->clubPrestige =
      ClampInt(m_activeSave->clubPrestige + static_cast<int>(amount / 1000000LL), 0, 100);
  m_activeSave->reputation =
      ClampInt(m_activeSave->reputation + static_cast<int>(amount / 1500000LL), 0, 100);
  m_activeSave->club.reputation = m_activeSave->reputation;
}

SimulatedMatch CareerDatabase::SimulateMatchResult(const std::string& opponentName,
                                                   const std::string& opponentTeamDBID,
                                                   bool isHome) {
  SimulatedMatch result;
  result.opponentName = opponentName;
  if (!m_activeSave)
    return result;

  int teamOVR = 65;
  int opponentOVR = 65;
  int teamMorale = 70;
  int teamForm = 50;
  std::string strategy = m_activeSave->activeStrategy;

  int ovrSum = 0;
  int moraleSum = 0;
  int formSum = 0;
  int count = 0;
  for (const auto& p : m_activeSave->roster) {
    ovrSum += p.ovr;
    moraleSum += p.morale;
    formSum += p.matchForm;
    count++;
  }
  if (count > 0) {
    teamOVR = ovrSum / count;
    teamMorale = moraleSum / count;
    teamForm = formSum / count;
  }

  // Wider, identity-stable opponent pool (roughly 45-88) so weak and elite
  // clubs both meet realistic resistance across a season.
  if (!opponentName.empty()) {
    int seed = static_cast<int>(std::hash<std::string>{}(opponentName) % 1000);
    opponentOVR = 45 + (seed % 44);
  } else if (!opponentTeamDBID.empty()) {
    int idValue = SafeStoi(opponentTeamDBID);
    opponentOVR = 45 + ((idValue % 44) + 44) % 44;
  } else {
    opponentOVR = 55 + RandomInt(0, 30);
  }

  int baseAttack = teamOVR + (teamForm - 50) / 8 + (teamMorale - 50) / 12;
  int baseDefense = teamOVR + (teamForm - 50) / 10 + (teamMorale - 50) / 15;
  int oppAttack = opponentOVR + RandomInt(-2, 4);
  int oppDefense = opponentOVR + RandomInt(-2, 3);

  if (strategy == "Attacking") {
    baseAttack += 4;
    baseDefense -= 3;
  } else if (strategy == "Defensive") {
    baseAttack -= 3;
    baseDefense += 4;
  }

  // homeGoals/awayGoals mean "us" / "them" for ApplyMatchResult bookkeeping.
  // Expected goals are primarily OVR-gap driven so club tiers separate cleanly.
  float venueAttack = isHome ? 1.08f : 0.92f;
  float ourXG = 1.05f * venueAttack + 0.06f * static_cast<float>(baseAttack - oppDefense);
  float theirXG = 1.05f / venueAttack + 0.06f * static_cast<float>(oppAttack - baseDefense);
  ourXG = std::max(0.2f, std::min(3.8f, ourXG));
  theirXG = std::max(0.2f, std::min(3.8f, theirXG));

  std::poisson_distribution<int> ourDist(ourXG);
  std::poisson_distribution<int> theirDist(theirXG);
  int expectedHomeGoals = ourDist(CareerRng());
  int expectedAwayGoals = theirDist(CareerRng());

  result.homeGoals = ClampInt(expectedHomeGoals, 0, 9);
  result.awayGoals = ClampInt(expectedAwayGoals, 0, 7);
  result.homeShots = result.homeGoals + RandomInt(2, 8);
  result.awayShots = result.awayGoals + RandomInt(2, 8);
  result.homePossession = ClampInt(50 + (teamOVR - opponentOVR) + RandomInt(-5, 5) +
                                       (strategy == "Attacking"   ? 5
                                        : strategy == "Defensive" ? -5
                                                                  : 0) +
                                       (isHome ? 3 : -3),
                                   30, 70);
  result.played = true;

  std::vector<int> scorerIndices;
  int rosterSize = static_cast<int>(m_activeSave->roster.size());
  if (rosterSize <= 0)
    return result;
  for (int g = 0; g < result.homeGoals; g++) {
    int attempts = 0;
    int pIdx;
    do {
      pIdx = RandomInt(0, rosterSize - 1);
      attempts++;
    } while (attempts < 20 &&
             std::find(scorerIndices.begin(), scorerIndices.end(), pIdx) != scorerIndices.end() &&
             rosterSize > static_cast<int>(scorerIndices.size()));
    scorerIndices.push_back(pIdx);
    result.scorers.push_back(m_activeSave->roster[pIdx].name);
  }

  return result;
}

void CareerDatabase::SeedRng(unsigned int seed) {
  SeedCareerRng(seed);
}

void CareerDatabase::ApplyMatchResult(int homeGoals, int awayGoals,
                                      const std::string& opponentLabel,
                                      const std::vector<std::string>& scorers) {
  if (!m_activeSave)
    return;

  const bool isWin = homeGoals > awayGoals;
  const bool isDraw = homeGoals == awayGoals;
  m_activeSave->seasonWins += isWin ? 1 : 0;
  m_activeSave->seasonDraws += isDraw ? 1 : 0;
  m_activeSave->seasonLosses += (!isWin && !isDraw) ? 1 : 0;
  m_activeSave->seasonGoalsFor += std::max(0, homeGoals);
  m_activeSave->seasonGoalsAgainst += std::max(0, awayGoals);

  std::string summary =
      m_activeSave->name + " " + std::to_string(homeGoals) + " - " + std::to_string(awayGoals);
  if (!opponentLabel.empty())
    summary += " " + opponentLabel;
  // Draws are reputation-neutral; only decisive results move the needle.
  // Match results are not major legacy events — season advance / transfers are.
  const int reputationDelta = isWin ? 1 : (isDraw ? 0 : -1);
  AddEvent("matchday", summary, reputationDelta, false);
  ModifyBoardConfidence(reputationDelta);

  // Squad-wide form/fitness regression after every match prevents mid-season
  // inflation from turning average clubs into perpetual title winners.
  for (auto& player : m_activeSave->roster) {
    player.matchForm = ClampInt(player.matchForm - RandomInt(1, 3), 25, 100);
    player.fitness = ClampInt(player.fitness - RandomInt(0, 2), 55, 100);
  }

  for (const auto& scorerName : scorers) {
    RecordMatchStats(scorerName, 1, 0);
  }
}

void CareerDatabase::Process3DMatchResult(int homeGoals, int awayGoals) {
  ApplyMatchResult(homeGoals, awayGoals, "(3D match)");
}

bool CareerDatabase::SaveToFile(const std::string& path) const {
  if (!m_activeSave)
    return false;
  std::ofstream file(path);
  if (!file.is_open())
    return false;
  file << "# Career Save: " << m_activeSave->name << "\n";
  file << "mode=" << static_cast<int>(m_activeSave->mode) << "\n";
  file << "name=" << m_activeSave->name << "\n";
  file << "managerName=" << m_activeSave->managerName << "\n";
  file << "clubName=" << m_activeSave->club.clubName << "\n";
  file << "clubID=" << m_activeSave->club.clubID << "\n";
  file << "clubLeague=" << m_activeSave->club.leagueName << "\n";
  file << "reputation=" << m_activeSave->reputation << "\n";
  file << "boardConfidence=" << m_activeSave->boardConfidence << "\n";
  file << "transferBudget=" << m_activeSave->transferBudget << "\n";
  file << "wageBudget=" << m_activeSave->wageBudget << "\n";
  file << "season=" << m_activeSave->season.currentSeason << "\n";
  file << "week=" << m_activeSave->season.currentWeek << "\n";
  file << "strategy=" << m_activeSave->activeStrategy << "\n";
  file << "fanBase=" << m_activeSave->fanBase << "\n";
  file << "clubPrestige=" << m_activeSave->clubPrestige << "\n";
  file << "seasonWins=" << m_activeSave->seasonWins << "\n";
  file << "seasonDraws=" << m_activeSave->seasonDraws << "\n";
  file << "seasonLosses=" << m_activeSave->seasonLosses << "\n";
  file << "seasonGoalsFor=" << m_activeSave->seasonGoalsFor << "\n";
  file << "seasonGoalsAgainst=" << m_activeSave->seasonGoalsAgainst << "\n";
  file << "netWorth=" << m_activeSave->finances.netWorth << "\n";
  file << "ticketPrice=" << m_activeSave->finances.ticketPrice << "\n";
  file << "stadiumCapacity=" << m_activeSave->stadium.capacity << "\n";
  file << "stadiumName=" << m_activeSave->stadium.name << "\n";
  file << "stadiumCondition=" << m_activeSave->stadium.condition << "\n";
  file << "stadiumFanSatisfaction=" << m_activeSave->stadium.fanSatisfaction << "\n";
  file << "controlledEntityID=" << m_activeSave->controlledEntityID << "\n";
  file << "trainingPoints=" << m_activeSave->trainingPoints << "\n";
  file << "scoutingNetworkLevel=" << m_activeSave->scoutingNetworkLevel << "\n";
  file << "objective=" << Sanitize(m_activeSave->objective) << "\n";

  file << "rosterSize=" << m_activeSave->roster.size() << "\n";
  for (size_t i = 0; i < m_activeSave->roster.size(); i++)
    file << "player." << i << "=" << PlayerToRecord(m_activeSave->roster[i]) << "\n";
  for (size_t i = 0; i < m_activeSave->freeAgents.size(); i++)
    file << "freeAgent." << i << "=" << PlayerToRecord(m_activeSave->freeAgents[i]) << "\n";
  for (size_t i = 0; i < m_activeSave->youthAcademy.size(); i++)
    file << "youth." << i << "=" << PlayerToRecord(m_activeSave->youthAcademy[i]) << "\n";

  for (size_t i = 0; i < m_activeSave->staff.size(); i++) {
    const auto& s = m_activeSave->staff[i];
    file << "staff." << i << "=" << Sanitize(s.name) << "|" << Sanitize(s.role) << "|" << s.skill
         << "|" << s.salary << "|" << s.contractYearsRemaining << "|" << s.morale << "\n";
  }
  for (size_t i = 0; i < m_activeSave->activeSponsors.size(); i++) {
    const auto& s = m_activeSave->activeSponsors[i];
    file << "sponsor." << i << "=" << Sanitize(s.sponsorName) << "|" << Sanitize(s.type) << "|"
         << s.annualRevenue << "|" << s.yearsRemaining << "|" << s.reputationRequirement << "\n";
  }
  for (size_t i = 0; i < m_activeSave->recentEvents.size(); i++) {
    const auto& e = m_activeSave->recentEvents[i];
    file << "event." << i << "=" << Sanitize(e.type) << "|" << e.reputationImpact << "|"
         << e.timestamp << "|" << (e.isMajor ? 1 : 0) << "|" << Sanitize(e.description) << "\n";
  }
  for (size_t i = 0; i < m_activeSave->inbox.size(); i++) {
    const auto& m = m_activeSave->inbox[i];
    file << "inbox." << i << "=" << m.id << "|" << static_cast<int>(m.type) << "|" << m.weekCreated
         << "|" << (m.read ? 1 : 0) << "|" << m.relatedPlayerID << "|" << m.relatedTeamID << "|"
         << Sanitize(m.subject) << "|" << Sanitize(m.body) << "\n";
  }
  for (size_t i = 0; i < m_activeSave->history.size(); i++) {
    const auto& h = m_activeSave->history[i];
    file << "history." << i << "=" << h.season << "|" << h.teamID << "|" << h.wins << "|" << h.draws
         << "|" << h.losses << "|" << h.goalsFor << "|" << h.goalsAgainst << "|" << h.leaguePosition
         << "|" << (h.wonTitle ? 1 : 0) << "\n";
  }
  for (size_t i = 0; i < m_activeSave->boardObjectives.size(); i++) {
    const auto& o = m_activeSave->boardObjectives[i];
    file << "boardObjective." << i << "=" << static_cast<int>(o.type) << "|"
         << (o.completed ? 1 : 0) << "|" << o.reputationReward << "|" << o.confidencePenalty << "|"
         << Sanitize(o.description) << "\n";
  }
  for (const auto& kv : m_activeSave->legacyStats)
    file << "legacy." << Sanitize(kv.first) << "=" << kv.second << "\n";
  for (size_t i = 0; i < m_activeBids.size(); i++) {
    const auto& b = m_activeBids[i];
    file << "bid." << i << "=" << Sanitize(b.playerName) << "|" << b.bidAmount << "|"
         << b.offeredWage << "|" << b.contractYears << "|" << b.agentFee << "|"
         << static_cast<int>(b.status) << "|" << b.negotiationRounds << "\n";
  }

  file.close();
  return true;
}

bool CareerDatabase::LoadFromFile(const std::string& path) {
  std::ifstream file(path);
  if (!file.is_open())
    return false;
  m_activeSave = std::make_unique<CareerSave>();
  m_activeBids.clear();
  std::string line;
  while (std::getline(file, line)) {
    if (line.empty() || line[0] == '#')
      continue;
    size_t eq = line.find('=');
    if (eq == std::string::npos)
      continue;
    std::string key = line.substr(0, eq);
    std::string val = line.substr(eq + 1);
    if (key == "name")
      m_activeSave->name = val;
    else if (key == "mode")
      m_activeSave->mode = static_cast<CareerMode>(SafeStoi(val));
    else if (key == "managerName")
      m_activeSave->managerName = val;
    else if (key == "clubName")
      m_activeSave->club.clubName = val;
    else if (key == "clubID")
      m_activeSave->club.clubID = SafeStoi(val);
    else if (key == "clubLeague")
      m_activeSave->club.leagueName = val;
    else if (key == "reputation")
      m_activeSave->reputation = SafeStoi(val);
    else if (key == "boardConfidence")
      m_activeSave->boardConfidence = SafeStoi(val);
    else if (key == "transferBudget")
      m_activeSave->transferBudget = SafeStoll(val);
    else if (key == "wageBudget")
      m_activeSave->wageBudget = SafeStoll(val);
    else if (key == "season")
      m_activeSave->season.currentSeason = SafeStoi(val);
    else if (key == "week")
      m_activeSave->season.currentWeek = SafeStoi(val);
    else if (key == "strategy")
      m_activeSave->activeStrategy = val;
    else if (key == "fanBase")
      m_activeSave->fanBase = SafeStoi(val);
    else if (key == "clubPrestige")
      m_activeSave->clubPrestige = SafeStoi(val);
    else if (key == "seasonWins")
      m_activeSave->seasonWins = SafeStoi(val);
    else if (key == "seasonDraws")
      m_activeSave->seasonDraws = SafeStoi(val);
    else if (key == "seasonLosses")
      m_activeSave->seasonLosses = SafeStoi(val);
    else if (key == "seasonGoalsFor")
      m_activeSave->seasonGoalsFor = SafeStoi(val);
    else if (key == "seasonGoalsAgainst")
      m_activeSave->seasonGoalsAgainst = SafeStoi(val);
    else if (key == "netWorth")
      m_activeSave->finances.netWorth = SafeStoll(val);
    else if (key == "ticketPrice")
      m_activeSave->finances.ticketPrice = SafeStoi(val);
    else if (key == "stadiumCapacity")
      m_activeSave->stadium.capacity = SafeStoi(val);
    else if (key == "stadiumName")
      m_activeSave->stadium.name = val;
    else if (key == "stadiumCondition")
      m_activeSave->stadium.condition = SafeStoi(val, m_activeSave->stadium.condition);
    else if (key == "stadiumFanSatisfaction")
      m_activeSave->stadium.fanSatisfaction = SafeStoi(val, m_activeSave->stadium.fanSatisfaction);
    else if (key == "controlledEntityID")
      m_activeSave->controlledEntityID = SafeStoi(val);
    else if (key == "trainingPoints")
      m_activeSave->trainingPoints = SafeStoi(val, m_activeSave->trainingPoints);
    else if (key == "scoutingNetworkLevel")
      m_activeSave->scoutingNetworkLevel = SafeStoi(val, m_activeSave->scoutingNetworkLevel);
    else if (key == "objective")
      m_activeSave->objective = val;
    else if (key == "rosterSize") { /* size is implied by the player.N rows */
    } else if (key.rfind("player.", 0) == 0) {
      m_activeSave->roster.push_back(PlayerFromRecord(val));
    } else if (key.rfind("freeAgent.", 0) == 0) {
      m_activeSave->freeAgents.push_back(PlayerFromRecord(val));
    } else if (key.rfind("youth.", 0) == 0) {
      m_activeSave->youthAcademy.push_back(PlayerFromRecord(val));
    } else if (key.rfind("staff.", 0) == 0) {
      std::vector<std::string> t = SplitPipes(val);
      StaffMember s;
      if (t.size() > 0)
        s.name = t[0];
      if (t.size() > 1)
        s.role = t[1];
      if (t.size() > 2)
        s.skill = SafeStoi(t[2], s.skill);
      if (t.size() > 3)
        s.salary = SafeStoll(t[3], s.salary);
      if (t.size() > 4)
        s.contractYearsRemaining = SafeStoi(t[4], s.contractYearsRemaining);
      if (t.size() > 5)
        s.morale = SafeStoi(t[5], s.morale);
      m_activeSave->staff.push_back(s);
    } else if (key.rfind("sponsor.", 0) == 0) {
      std::vector<std::string> t = SplitPipes(val);
      SponsorDeal s;
      if (t.size() > 0)
        s.sponsorName = t[0];
      if (t.size() > 1)
        s.type = t[1];
      if (t.size() > 2)
        s.annualRevenue = SafeStoll(t[2]);
      if (t.size() > 3)
        s.yearsRemaining = SafeStoi(t[3]);
      if (t.size() > 4)
        s.reputationRequirement = SafeStoi(t[4]);
      m_activeSave->activeSponsors.push_back(s);
    } else if (key.rfind("event.", 0) == 0) {
      std::vector<std::string> t = SplitPipes(val);
      CareerEvent e;
      if (t.size() > 0)
        e.type = t[0];
      if (t.size() > 1)
        e.reputationImpact = SafeStoi(t[1]);
      if (t.size() > 2)
        e.timestamp = SafeStoll(t[2]);
      if (t.size() > 3)
        e.isMajor = SafeStoi(t[3]) != 0;
      if (t.size() > 4)
        e.description = t[4];
      m_activeSave->recentEvents.push_back(e);
    } else if (key.rfind("inbox.", 0) == 0) {
      std::vector<std::string> t = SplitPipes(val);
      InboxItem m;
      if (t.size() > 0)
        m.id = SafeStoi(t[0]);
      if (t.size() > 1)
        m.type = static_cast<InboxItemType>(SafeStoi(t[1]));
      if (t.size() > 2)
        m.weekCreated = SafeStoi(t[2]);
      if (t.size() > 3)
        m.read = SafeStoi(t[3]) != 0;
      if (t.size() > 4)
        m.relatedPlayerID = SafeStoi(t[4]);
      if (t.size() > 5)
        m.relatedTeamID = SafeStoi(t[5]);
      if (t.size() > 6)
        m.subject = t[6];
      if (t.size() > 7)
        m.body = t[7];
      m_activeSave->inbox.push_back(m);
    } else if (key.rfind("history.", 0) == 0) {
      std::vector<std::string> t = SplitPipes(val);
      SeasonRecord r;
      if (t.size() > 0)
        r.season = SafeStoi(t[0]);
      if (t.size() > 1)
        r.teamID = SafeStoi(t[1]);
      if (t.size() > 2)
        r.wins = SafeStoi(t[2]);
      if (t.size() > 3)
        r.draws = SafeStoi(t[3]);
      if (t.size() > 4)
        r.losses = SafeStoi(t[4]);
      if (t.size() > 5)
        r.goalsFor = SafeStoi(t[5]);
      if (t.size() > 6)
        r.goalsAgainst = SafeStoi(t[6]);
      if (t.size() > 7)
        r.leaguePosition = SafeStoi(t[7]);
      if (t.size() > 8)
        r.wonTitle = SafeStoi(t[8]) != 0;
      m_activeSave->history.push_back(r);
    } else if (key.rfind("boardObjective.", 0) == 0) {
      std::vector<std::string> t = SplitPipes(val);
      OwnerBoardObjective o;
      if (t.size() > 0)
        o.type = static_cast<OwnerObjectiveType>(SafeStoi(t[0]));
      if (t.size() > 1)
        o.completed = SafeStoi(t[1]) != 0;
      if (t.size() > 2)
        o.reputationReward = SafeStoi(t[2], o.reputationReward);
      if (t.size() > 3)
        o.confidencePenalty = SafeStoi(t[3], o.confidencePenalty);
      if (t.size() > 4)
        o.description = t[4];
      m_activeSave->boardObjectives.push_back(o);
    } else if (key.rfind("legacy.", 0) == 0) {
      m_activeSave->legacyStats[key.substr(7)] = SafeStoi(val);
    } else if (key.rfind("bid.", 0) == 0) {
      std::vector<std::string> t = SplitPipes(val);
      TransferBid b;
      if (t.size() > 0)
        b.playerName = t[0];
      if (t.size() > 1)
        b.bidAmount = SafeStoll(t[1]);
      if (t.size() > 2)
        b.offeredWage = SafeStoi(t[2]);
      if (t.size() > 3)
        b.contractYears = SafeStoi(t[3], b.contractYears);
      if (t.size() > 4)
        b.agentFee = SafeStoll(t[4]);
      if (t.size() > 5)
        b.status = static_cast<BidStatus>(SafeStoi(t[5]));
      if (t.size() > 6)
        b.negotiationRounds = SafeStoi(t[6]);
      m_activeBids.push_back(b);
    }
  }
  file.close();

  // Keep the mirrored/derived fields consistent with the loaded top-level
  // values so a loaded save matches the state produced by CreateNewCareer.
  m_activeSave->currentSeason = m_activeSave->season.currentSeason;
  m_activeSave->club.reputation = m_activeSave->reputation;
  m_activeSave->board.confidence = m_activeSave->boardConfidence;
  m_activeSave->finance.transferBudget = m_activeSave->transferBudget;
  m_activeSave->finance.wageBudget = m_activeSave->wageBudget;

  printf("[career] Loaded from %s (%zu roster)\n", path.c_str(), m_activeSave->roster.size());
  return true;
}

}  // namespace blunted
