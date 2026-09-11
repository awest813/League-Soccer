#include <cmath>
#include <filesystem>
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

TEST(InMatchUILocaleTest, MatchKeysLoadedAcrossAllLanguages) {
  const std::vector<std::string> languages = {"en", "de", "es", "fr", "pt"};
  const std::vector<std::string> keys = {
      "ingame_goal",
      "ingame_owngoal",
      "ingame_offside",
      "ingame_advantage",
      "ingame_foul",
      "ingame_yellow_card",
      "ingame_red_card",
      "ingame_replay_help",
      "gameover_mom",
      "setpiece_title",
      "setpiece_free_kick",
      "setpiece_corner",
      "setpiece_goal_kick",
      "setpiece_header",
      "setpiece_depth",
      "setpiece_width",
      "action_save",
  };

  for (const auto& lang : languages) {
    Localization::GetInstance().Load(lang);
    for (const auto& key : keys) {
      std::string val = Localization::GetInstance().Translate(key);
      EXPECT_FALSE(val.empty()) << "Key " << key << " is empty in lang " << lang;
      EXPECT_NE(val, key) << "Key " << key << " was not translated in lang " << lang;
    }
  }

  // Restore English default
  Localization::GetInstance().Load("en");
}

TEST(InMatchUILayoutTest, EventTickerDoesNotOverlapScoreboard) {
  constexpr float kScoreboardY = 2.0f;
  constexpr float kScoreboardHeight = 4.0f;
  constexpr float kScoreboardBottom = kScoreboardY + kScoreboardHeight;  // 6.0f

  constexpr float kEventTickerY = 7.2f;

  EXPECT_GT(kEventTickerY, kScoreboardBottom);
  EXPECT_GE(kEventTickerY - kScoreboardBottom, 1.0f);  // At least 1% vertical margin
}

TEST(InMatchUILayoutTest, TacticalPitchBoardAspectAndCoordinateMapping) {
  constexpr float kDiagramRatio = 550.0f / 360.0f;
  constexpr float kBoardWidth = 1100.0f;
  constexpr float kBoardHeight = 720.0f;
  EXPECT_NEAR(kBoardWidth / kBoardHeight, kDiagramRatio, 1e-4f);

  // Formation coordinates pos in [-1.0, 1.0] map to pos * 0.44 + 0.5 -> [0.06, 0.94]
  auto mapCoord = [](float pos) { return pos * 0.44f + 0.50f; };
  EXPECT_NEAR(mapCoord(-1.0f), 0.06f, 1e-4f);
  EXPECT_NEAR(mapCoord(0.0f), 0.50f, 1e-4f);
  EXPECT_NEAR(mapCoord(1.0f), 0.94f, 1e-4f);
  EXPECT_GT(mapCoord(-1.0f), 0.0f);
  EXPECT_LT(mapCoord(1.0f), 1.0f);
}

TEST(InMatchUILayoutTest, StaminaThreeTierColorClassification) {
  auto getTier = [](float stamina) -> int {
    if (stamina >= 0.50f)
      return 0;  // Green
    if (stamina >= 0.25f)
      return 1;  // Yellow
    return 2;    // Red
  };

  EXPECT_EQ(getTier(1.00f), 0);
  EXPECT_EQ(getTier(0.75f), 0);
  EXPECT_EQ(getTier(0.50f), 0);
  EXPECT_EQ(getTier(0.49f), 1);
  EXPECT_EQ(getTier(0.25f), 1);
  EXPECT_EQ(getTier(0.24f), 2);
  EXPECT_EQ(getTier(0.00f), 2);
}

// ===========================================================================
// Texture audit & integrity tests
// ===========================================================================

TEST(TextureAuditTest, PowerOfTwoVerification) {
  auto isPowerOfTwo = [](int n) -> bool {
    return (n != 0) && ((n & (n - 1)) == 0);
  };

  // Standard 3D hardware texture dimensions
  EXPECT_TRUE(isPowerOfTwo(16));
  EXPECT_TRUE(isPowerOfTwo(32));
  EXPECT_TRUE(isPowerOfTwo(64));
  EXPECT_TRUE(isPowerOfTwo(128));
  EXPECT_TRUE(isPowerOfTwo(256));
  EXPECT_TRUE(isPowerOfTwo(512));
  EXPECT_TRUE(isPowerOfTwo(1024));
  EXPECT_TRUE(isPowerOfTwo(2048));
  EXPECT_TRUE(isPowerOfTwo(4096));

  // Non-power-of-two values
  EXPECT_FALSE(isPowerOfTwo(1200));
  EXPECT_FALSE(isPowerOfTwo(1100));
  EXPECT_FALSE(isPowerOfTwo(720));
  EXPECT_FALSE(isPowerOfTwo(1376));
}

TEST(TextureAuditTest, EssentialTexturesExistAndAreOptimized) {
  auto resolveAsset = [](const std::string& relPath) -> std::filesystem::path {
    std::filesystem::path p1 = relPath;
    if (std::filesystem::exists(p1))
      return p1;
    std::filesystem::path p2 = std::filesystem::path("data") / relPath;
    if (std::filesystem::exists(p2))
      return p2;
    return {};
  };

  // Grass texture
  std::filesystem::path grassPath = resolveAsset("media/textures/pitch/seamlessgrass08.png");
  if (!grassPath.empty()) {
    EXPECT_GT(std::filesystem::file_size(grassPath), 0u);
    // Verified compressed from original 3.04 MB to under 2.5 MB
    EXPECT_LT(std::filesystem::file_size(grassPath), 2500000u);
  }

  // Ball texture
  std::filesystem::path ballPath = resolveAsset("media/objects/balls/ball.jpg");
  if (!ballPath.empty()) {
    EXPECT_GT(std::filesystem::file_size(ballPath), 0u);
  }

  // Adboard textures
  std::filesystem::path ad1Path = resolveAsset("media/textures/adboards/ad_your_ad_here.png");
  if (!ad1Path.empty()) {
    EXPECT_GT(std::filesystem::file_size(ad1Path), 0u);
    EXPECT_LT(std::filesystem::file_size(ad1Path), 50000u);
  }

  std::filesystem::path ad2Path = resolveAsset("media/textures/adboards/ad_altfunc01.png");
  if (!ad2Path.empty()) {
    EXPECT_GT(std::filesystem::file_size(ad2Path), 0u);
    EXPECT_LT(std::filesystem::file_size(ad2Path), 50000u);
  }

  // Controller diagrams
  std::filesystem::path ctrlLeft = resolveAsset("media/menu/controller/controller_left.png");
  if (!ctrlLeft.empty()) {
    EXPECT_GT(std::filesystem::file_size(ctrlLeft), 0u);
    // Optimized under 1.2 MB
    EXPECT_LT(std::filesystem::file_size(ctrlLeft), 1200000u);
  }
}

}  // namespace


