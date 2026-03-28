#pragma once

#include <map>
#include <string>
#include <vector>

struct Scout {
  int scoutID = 0;
  std::string name;
  int skill = 50;   // 0–100
  std::string region;
  int salary = 0;
};

struct ScoutReport {
  int reportID = 0;
  int playerID = 0;
  int scoutID = 0;
  int reportDate = 0;  // encoded as season * 1000 + week
  // Known attributes with uncertainty: actual value ± uncertainty
  std::map<std::string, int> knownAttributes;
  std::map<std::string, int> attributeUncertainty;
  int potential = 0;  // 0–100, scout's estimate
  bool recommendBid = false;
};

class ScoutingNetwork {
 public:
  ScoutingNetwork() = default;

  int HireScout(const Scout& scout);
  void FireScout(int scoutID);
  Scout* GetScout(int scoutID);
  const std::vector<Scout>& GetScouts() const { return m_scouts; }

  // Assign a scout to watch a player; generates a report
  ScoutReport GenerateReport(int scoutID, int playerID, int season, int week);

  // Reveal a specific attribute (reduces uncertainty to 0)
  void RevealAttribute(ScoutReport& report, const std::string& attr);

  const std::vector<ScoutReport>& GetReports() const { return m_reports; }

 private:
  std::vector<Scout> m_scouts;
  std::vector<ScoutReport> m_reports;
  int m_nextScoutID = 1;
  int m_nextReportID = 1;
};
