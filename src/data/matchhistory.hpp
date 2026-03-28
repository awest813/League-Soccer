#ifndef _HPP_MATCHHISTORY
#define _HPP_MATCHHISTORY

#include <string>
#include <vector>

struct MatchHistoryEntry {
  int id;
  std::string timestamp;
  std::string team1_name, team2_name;
  int score1, score2;
  int match_time_ms;
  float possession1_pct, possession2_pct;
  int shots1, shots2;
  int shots_on_target1, shots_on_target2;
  int passes1, passes2;
  int passes_completed1, passes_completed2;
  int fouls1, fouls2;
};

class MatchHistory {
 public:
  static void EnsureTable();
  static void SaveMatch(const MatchHistoryEntry& entry);
  static std::vector<MatchHistoryEntry> LoadRecent(int limit = 10);

 private:
  static std::string GetDbPath();
};

#endif
