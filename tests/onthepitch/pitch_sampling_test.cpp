#include <gtest/gtest.h>

#include "onthepitch/pitchsampling.hpp"

TEST(PitchSampling, InterpolatesInterior) {
  const float tex[] = {0, 10, 20, 30};
  EXPECT_FLOAT_EQ(pitchsampling::BilinearSample(tex, 0.5f, 0.5f, 2, 2), 15);
}

TEST(PitchSampling, UniqueMarkingsClampAtAllBorders) {
  const float tex[] = {0, 10, 20, 30};
  EXPECT_FLOAT_EQ(pitchsampling::BilinearSample(tex, 2, 2, 2, 2), 30);
  EXPECT_FLOAT_EQ(pitchsampling::BilinearSample(tex, -1, -1, 2, 2), 0);
  EXPECT_FLOAT_EQ(pitchsampling::BilinearSample(tex, 1.75f, 0.5f, 2, 2), 20);
  EXPECT_FLOAT_EQ(pitchsampling::BilinearSample(tex, 0.5f, 1.75f, 2, 2), 25);
}

TEST(PitchSampling, GrassRepeatsAcrossPositiveAndNegativeSeams) {
  const float tex[] = {0, 10, 20, 30};
  EXPECT_FLOAT_EQ(pitchsampling::BilinearSample(tex, 2, 2, 2, 2, true), 0);
  EXPECT_FLOAT_EQ(pitchsampling::BilinearSample(tex, -0.5f, -0.5f, 2, 2, true), 15);
  EXPECT_FLOAT_EQ(pitchsampling::BilinearSample(tex, 1.5f, 1.5f, 2, 2, true), 15);
}

TEST(PitchSampling, SingleTexelWorksInBothModes) {
  const float tex[] = {42};
  EXPECT_FLOAT_EQ(pitchsampling::BilinearSample(tex, 9, -3, 1, 1), 42);
  EXPECT_FLOAT_EQ(pitchsampling::BilinearSample(tex, 9, -3, 1, 1, true), 42);
}
