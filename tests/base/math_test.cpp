#include <gtest/gtest.h>

#include "base/math/bluntmath.hpp"
#include "base/math/matrix3.hpp"
#include "base/math/quaternion.hpp"
#include "base/math/vector3.hpp"

namespace {

constexpr float kEpsilon = 1e-4f;

void ExpectVectorNear(const blunted::Vector3& actual, const blunted::Vector3& expected,
                      float epsilon = kEpsilon) {
  EXPECT_NEAR(actual.coords[0], expected.coords[0], epsilon);
  EXPECT_NEAR(actual.coords[1], expected.coords[1], epsilon);
  EXPECT_NEAR(actual.coords[2], expected.coords[2], epsilon);
}

void ExpectMatrixNear(const blunted::Matrix3& actual, const blunted::Matrix3& expected,
                      float epsilon = kEpsilon) {
  for (int i = 0; i < 9; ++i) {
    EXPECT_NEAR(actual.elements[i], expected.elements[i], epsilon);
  }
}

void ExpectQuaternionEquivalent(blunted::Quaternion actual, blunted::Quaternion expected,
                                float epsilon = kEpsilon) {
  actual.Normalize();
  expected.Normalize();
  actual.MakeSameNeighborhood(expected);
  for (int i = 0; i < 4; ++i) {
    EXPECT_NEAR(actual.elements[i], expected.elements[i], epsilon);
  }
}

}  // namespace

TEST(BluntMathTest, ClampAndNormalizeRange) {
  EXPECT_FLOAT_EQ(blunted::clamp(-2.0f, -1.0f, 4.0f), -1.0f);
  EXPECT_FLOAT_EQ(blunted::clamp(2.5f, -1.0f, 4.0f), 2.5f);
  EXPECT_FLOAT_EQ(blunted::clamp(10.0f, -1.0f, 4.0f), 4.0f);

  EXPECT_NEAR(blunted::NormalizedClamp(3.0f, 1.0f, 5.0f), 0.5f, kEpsilon);
  EXPECT_NEAR(blunted::NormalizedClamp(-8.0f, -4.0f, 4.0f), 0.0f, kEpsilon);
}

TEST(BluntMathTest, ModulatesValuesAndComputesParityHelpers) {
  EXPECT_EQ(blunted::pot(5), 8);
  EXPECT_EQ(blunted::pot(16), 16);
  EXPECT_EQ(blunted::signSide(-0.25f), -1);
  EXPECT_EQ(blunted::signSide(0.0f), 1);
  EXPECT_TRUE(blunted::is_odd(7));
  EXPECT_FALSE(blunted::is_odd(12));

  EXPECT_NEAR(blunted::ModulateIntoRange(-blunted::pi, blunted::pi, 3.0f * blunted::pi),
              blunted::pi, kEpsilon);
  EXPECT_NEAR(blunted::ModulateIntoRange(0.0f, 10.0f, -3.0f), 7.0f, kEpsilon);
}

TEST(Vector3Test, ComputesCrossDotAndDistance) {
  const blunted::Vector3 x_axis(1.0f, 0.0f, 0.0f);
  const blunted::Vector3 y_axis(0.0f, 1.0f, 0.0f);
  const blunted::Vector3 diagonal(1.0f, 2.0f, 2.0f);

  ExpectVectorNear(x_axis.GetCrossProduct(y_axis), blunted::Vector3(0.0f, 0.0f, 1.0f));
  EXPECT_NEAR(diagonal.GetDotProduct(blunted::Vector3(2.0f, 0.0f, 1.0f)), 4.0f, kEpsilon);
  EXPECT_NEAR(diagonal.GetDistance(blunted::Vector3(1.0f, 2.0f, -1.0f)), 3.0f, kEpsilon);
}

TEST(Vector3Test, NormalizesAndFallsBackFromZeroVector) {
  blunted::Vector3 value(3.0f, 4.0f, 0.0f);
  value.Normalize();

  EXPECT_NEAR(value.GetLength(), 1.0f, kEpsilon);
  ExpectVectorNear(value, blunted::Vector3(0.6f, 0.8f, 0.0f));

  const blunted::Vector3 fallback(0.0f, 1.0f, 0.0f);
  const blunted::Vector3 normalized_zero = blunted::Vector3().GetNormalized(fallback);
  ExpectVectorNear(normalized_zero, fallback);
}

TEST(Vector3Test, RotatesInTwoDimensionsWithoutChangingHeight) {
  const blunted::Vector3 rotated =
      blunted::Vector3(1.0f, 0.0f, 2.0f).GetRotated2D(blunted::pi / 2.0f);
  ExpectVectorNear(rotated, blunted::Vector3(0.0f, 1.0f, 2.0f));
}

TEST(Matrix3Test, IdentityAndTransposeBehaveAsExpected) {
  const blunted::Vector3 value(2.0f, -3.0f, 4.0f);
  ExpectVectorNear(blunted::Matrix3::IDENTITY * value, value);

  blunted::Matrix3 matrix(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
  matrix.Transpose();

  ExpectMatrixNear(matrix, blunted::Matrix3(1.0f, 4.0f, 7.0f, 2.0f, 5.0f, 8.0f, 3.0f, 6.0f, 9.0f));
}

TEST(Matrix3Test, MultipliesMatricesAndVectors) {
  const blunted::Matrix3 matrix(1.0f, 2.0f, 3.0f, 0.0f, 1.0f, 4.0f, 5.0f, 6.0f, 0.0f);
  const blunted::Vector3 result = matrix * blunted::Vector3(1.0f, 2.0f, 3.0f);
  ExpectVectorNear(result, blunted::Vector3(14.0f, 14.0f, 17.0f));

  const blunted::Matrix3 product = matrix * blunted::Matrix3::IDENTITY;
  ExpectMatrixNear(product, matrix);
}

TEST(QuaternionTest, AngleAxisRotationRotatesVectors) {
  blunted::Quaternion rotation;
  rotation.SetAngleAxis(blunted::pi / 2.0f, blunted::Vector3(0.0f, 0.0f, 1.0f));

  const blunted::Vector3 rotated = rotation * blunted::Vector3(1.0f, 0.0f, 0.0f);
  ExpectVectorNear(rotated, blunted::Vector3(0.0f, 1.0f, 0.0f));
}

TEST(QuaternionTest, InverseAndMatrixRoundTripPreserveOrientation) {
  blunted::Quaternion rotation;
  rotation.SetAngles(0.3f, -0.4f, 0.8f);
  rotation.Normalize();

  const blunted::Quaternion identity = (rotation * rotation.GetInverse()).GetNormalized();
  ExpectQuaternionEquivalent(identity, blunted::Quaternion());

  blunted::Matrix3 matrix;
  rotation.ConstructMatrix(matrix);
  blunted::Quaternion round_trip;
  round_trip.Set(matrix);
  ExpectQuaternionEquivalent(round_trip, rotation);
}
