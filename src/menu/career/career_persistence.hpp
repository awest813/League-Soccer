#ifndef CAREER_PERSISTENCE_HPP
#define CAREER_PERSISTENCE_HPP

#include <string>
#include <vector>

#include "../../data/careerdata.hpp"

namespace blunted {
namespace CareerPersistence {

struct CareerSaveSummary {
  int schemaVersion = 1;
  std::string name;
  std::string clubName;
  std::string managerName;
  CareerMode mode = CareerMode::OWNER_GM;
  int season = 1;
  int week = 1;
  long long transferBudget = 0;
  int reputation = 50;
  int boardConfidence = 75;
  std::string timestamp;
  bool isValid = false;
};

// Serializes the active career, its transfer bids and the transfer market the
// bids are against to the on-disk SQLite save file. Returns false on failure.
bool Save(const CareerSave& save, const std::vector<TransferBid>& bids,
          const std::vector<TransferTarget>& targets, const std::string& path);
// Same, without a transfer market (pending bids cannot complete after a load).
bool Save(const CareerSave& save, const std::vector<TransferBid>& bids, const std::string& path);

// Replaces the given save, bid list and transfer market with the contents of
// the on-disk save file. New saves are read back from SQLite; legacy plain-text
// save files are still parsed for backward compatibility. Returns false if the
// file cannot be opened or contains no recognizable career data.
bool Load(CareerSave& save, std::vector<TransferBid>& bids, std::vector<TransferTarget>& targets,
          const std::string& path);
bool Load(CareerSave& save, std::vector<TransferBid>& bids, const std::string& path);

// Reads lightweight metadata from the save file's career_meta table in sub-millisecond
// time without parsing the entire player roster or payload.
bool ReadSummary(const std::string& path, CareerSaveSummary& outSummary);

}  // namespace CareerPersistence
}  // namespace blunted

#endif  // CAREER_PERSISTENCE_HPP
