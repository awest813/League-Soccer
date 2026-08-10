#ifndef CAREER_BOARD_HPP
#define CAREER_BOARD_HPP

#include "../../data/careerdata.hpp"
#include "career_common.hpp"

namespace blunted {
namespace CareerBoard {

// Seeds the default board objectives for the upcoming season.
void GenerateBoardObjectives(CareerSave& save);

// Evaluates the board objectives against the current season and applies the
// reputation / confidence consequences.
void EvaluateBoardObjectives(CareerSave& save, CareerCommon::CareerEvents& events);

}  // namespace CareerBoard
}  // namespace blunted

#endif  // CAREER_BOARD_HPP
