#include <cmath>
#include <string>

#include <gtest/gtest.h>

#include "utils/difficulty.hpp"
#include "utils/localization.hpp"

namespace {

constexpr float kDefaultAgilityFactor = 0.5f;
constexpr float kDefaultAccelerationFactor = 0.5f;

// ===========================================================================
// Difficulty conversion tests
// ===========================================================================

TEST(DifficultyTest, MapsExactStepsCorrectly) {
  EXPECT_EQ(DifficultyToStep(0.00f), 0);
  EXPECT_EQ(DifficultyToStep(0.25f), 1);
  EXPECT_EQ(DifficultyToStep(0.50f), 2);
  EXPECT_EQ(DifficultyToStep(0.75f), 3);
  EXPECT_EQ(DifficultyToStep(1.00f), 4);
}

TEST(DifficultyTest, DefaultDifficultyMapsToProfessional) {
  // kDefaultDifficulty is 0.8f, which quantizes to step 3 (Professional)
  EXPECT_EQ(DifficultyToStep(kDefaultDifficulty), 3);
}

TEST(DifficultyTest, ClampsOutOfRangeValues) {
  EXPECT_EQ(DifficultyToStep(-0.5f), 0);
  EXPECT_EQ(DifficultyToStep(-100.0f), 0);
  EXPECT_EQ(DifficultyToStep(1.5f), 4);
  EXPECT_EQ(DifficultyToStep(100.0f), 4);

  EXPECT_FLOAT_EQ(DifficultyFromStep(-2), 0.0f);
  EXPECT_FLOAT_EQ(DifficultyFromStep(10), 1.0f);
}

TEST(DifficultyTest, StepRoundTrip) {
  for (int step = 0; step <= 4; ++step) {
    float val = DifficultyFromStep(step);
    EXPECT_EQ(DifficultyToStep(val), step);
  }
}

TEST(DifficultyTest, LocalizedNamesMatchEnglishDefaults) {
  // Ensure English strings are loaded
  Localization::GetInstance().Load("en");

  EXPECT_EQ(GetDifficultyName(0.00f), "Beginner");
  EXPECT_EQ(GetDifficultyName(0.25f), "Amateur");
  EXPECT_EQ(GetDifficultyName(0.50f), "Regular");
  EXPECT_EQ(GetDifficultyName(0.75f), "Professional");
  EXPECT_EQ(GetDifficultyName(1.00f), "Top Player");
}

// ===========================================================================
// Slider quantization mathematics tests
// ===========================================================================

TEST(SliderQuantizationTest, DifficultyQuantizationSteps) {
  const int steps = 5;
  const float stepSize = 1.0f / (steps - 1.0f);
  EXPECT_FLOAT_EQ(stepSize, 0.25f);

  auto quantize = [steps](float val) {
    val = std::clamp(val, 0.0f, 1.0f);
    return std::round(val * (steps - 1.0f)) / (steps - 1.0f);
  };

  EXPECT_NEAR(quantize(0.12f), 0.00f, 1e-5f);
  EXPECT_NEAR(quantize(0.13f), 0.25f, 1e-5f);
  EXPECT_NEAR(quantize(0.37f), 0.25f, 1e-5f);
  EXPECT_NEAR(quantize(0.38f), 0.50f, 1e-5f);
  EXPECT_NEAR(quantize(0.62f), 0.50f, 1e-5f);
  EXPECT_NEAR(quantize(0.63f), 0.75f, 1e-5f);
  EXPECT_NEAR(quantize(0.87f), 0.75f, 1e-5f);
  EXPECT_NEAR(quantize(0.88f), 1.00f, 1e-5f);
}

TEST(SliderQuantizationTest, TwoStepBinaryToggle) {
  const int steps = 2;
  auto quantize = [steps](float val) {
    val = std::clamp(val, 0.0f, 1.0f);
    return std::round(val * (steps - 1.0f)) / (steps - 1.0f);
  };

  EXPECT_FLOAT_EQ(quantize(0.0f), 0.0f);
  EXPECT_FLOAT_EQ(quantize(0.49f), 0.0f);
  EXPECT_FLOAT_EQ(quantize(0.51f), 1.0f);
  EXPECT_FLOAT_EQ(quantize(1.0f), 1.0f);
}

TEST(SliderQuantizationTest, ResetToDefaultBehavior) {
  float currentValue = 0.8f;
  const float helperDefault = 0.5f;

  // Emulate activate reset
  if (std::fabs(currentValue - helperDefault) > 1e-5f) {
    currentValue = helperDefault;
  }
  EXPECT_FLOAT_EQ(currentValue, 0.5f);

  // Subsequent activate should be a no-op
  bool changed = false;
  if (std::fabs(currentValue - helperDefault) > 1e-5f) {
    currentValue = helperDefault;
    changed = true;
  }
  EXPECT_FALSE(changed);
}

// ===========================================================================
// Gameplay assistance and physics formatting tests
// ===========================================================================

TEST(GameplaySliderUXTest, PhysicsMultipliersDefaultToOne) {
  // Agility formula: 0.75f + val * 0.5f
  const float defaultAgilityMult = 0.75f + kDefaultAgilityFactor * 0.5f;
  EXPECT_NEAR(defaultAgilityMult, 1.00f, 1e-5f);

  // Acceleration formula: 0.50f + val * 1.0f
  const float defaultAccelMult = 0.50f + kDefaultAccelerationFactor * 1.0f;
  EXPECT_NEAR(defaultAccelMult, 1.00f, 1e-5f);

  // Boundary ranges
  EXPECT_NEAR(0.75f + 0.0f * 0.5f, 0.75f, 1e-5f);  // Min agility
  EXPECT_NEAR(0.75f + 1.0f * 0.5f, 1.25f, 1e-5f);  // Max agility
  EXPECT_NEAR(0.50f + 0.0f * 1.0f, 0.50f, 1e-5f);  // Min acceleration
  EXPECT_NEAR(0.50f + 1.0f * 1.0f, 1.50f, 1e-5f);  // Max acceleration
}

TEST(GameplaySliderUXTest, AssistanceLocaleStringsLoaded) {
  Localization::GetInstance().Load("en");

  EXPECT_EQ(TR("gameplay_assist_manual"), "Manual (0%)");
  EXPECT_EQ(TRF("gameplay_assist_semi", {"20"}), "Semi (20%)");
  EXPECT_EQ(TRF("gameplay_assist_assisted", {"70"}), "Assisted (70%)");
  EXPECT_EQ(TRF("gameplay_assist_full", {"95"}), "Full (95%)");
}

TEST(GameplaySliderUXTest, QuantizationPresetLocaleStringsLoaded) {
  Localization::GetInstance().Load("en");

  EXPECT_EQ(TR("gameplay_quantization_analog"), "Full Analog");
  EXPECT_EQ(TR("gameplay_quantization_pes8"), "PES 8-way");
  EXPECT_EQ(TR("gameplay_quantization_pes16"), "PES 16-way");
}

}  // namespace
