#ifndef CAREER_TRAINING_HPP
#define CAREER_TRAINING_HPP

#include <string>

#include "../../data/careerdata.hpp"
#include "career_common.hpp"

namespace blunted {
namespace CareerTraining {

// Runs a squad training session (spends a point, lifts match form).
bool TrainSquad(CareerSave& save, CareerCommon::CareerEvents& events);

// Runs a position-specific focus session (spends a point, may raise OVR).
bool TrainFocus(CareerSave& save, CareerCommon::CareerEvents& events, const std::string& focusArea);

// Sets the active match strategy.
void SetStrategy(CareerSave& save, CareerCommon::CareerEvents& events, const std::string& strategy);

// Spends transfer budget to scout a random youth prospect.
void ScoutYouthPlayer(CareerSave& save, CareerCommon::CareerEvents& events);

// Promotes a named academy prospect into the senior roster.
void PromoteYouthPlayer(CareerSave& save, CareerCommon::CareerEvents& events,
                        const std::string& playerName);

}  // namespace CareerTraining
}  // namespace blunted

#endif  // CAREER_TRAINING_HPP
