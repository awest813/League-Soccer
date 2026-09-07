#include <gtest/gtest.h>
#include "utils/gui2/gridlayout.hpp"
#include "utils/gui2/textlayout.hpp"

using namespace blunted;

TEST(MenuGridLayout, GameplayColumnsStayInsidePanel) {
  std::vector<GridLayoutCell> cells;
  for (int row = 0; row < 7; ++row)
    for (int col = 0; col < 2; ++col)
      cells.push_back({row, col, 36.0f, 5.5f});
  const auto layout = CalculateGridLayout(cells, 7, 2, 0, 7, .5f, .5f, .5f, .5f);
  EXPECT_FLOAT_EQ(layout.width, 74.0f);
  EXPECT_FLOAT_EQ(layout.height, 45.5f);
  EXPECT_LT(layout.x[1] + 36.0f, 76.0f);
  EXPECT_LT(layout.y[6] + 5.5f, 68.0f);
}

TEST(MenuGridLayout, ScrollingStartsAtTopWithoutPhantomRow) {
  const std::vector<GridLayoutCell> cells = {{0, 0, 40, 20}, {1, 0, 10, 3}, {2, 0, 15, 4}};
  const auto layout = CalculateGridLayout(cells, 3, 1, 1, 2, .5f, .5f, .5f, .5f);
  EXPECT_FLOAT_EQ(layout.y[1], .5f);
  EXPECT_FLOAT_EQ(layout.y[2], 4.5f);
  EXPECT_FLOAT_EQ(layout.height, 9.0f);
  EXPECT_FLOAT_EQ(layout.width, 41.0f);
}

TEST(MenuGridLayout, HiddenRowsDoNotEnlargeViewport) {
  const auto layout = CalculateGridLayout({{0, 0, 10, 3}, {1, 0, 10, 80}},
                                         2, 1, 0, 1, 0, 0, 0, 0);
  EXPECT_FLOAT_EQ(layout.height, 3.0f);
}

TEST(MenuGridLayout, EmptyGridHasNoPhantomMargins) {
  const auto layout = CalculateGridLayout({}, 0, 0, 0, 12, 1, 1, 1, 1);
  EXPECT_FLOAT_EQ(layout.width, 0.0f);
  EXPECT_FLOAT_EQ(layout.height, 0.0f);
}

TEST(MenuGridLayout, ConfirmationFitsItsFrame) {
  const auto layout = CalculateGridLayout({{0, 0, 44, 3}, {1, 0, 44, 3}, {2, 0, 44, 3}},
                                         3, 1, 0, 3, .5f, .5f, .5f, .5f);
  EXPECT_LE(layout.width, 46.0f);
  EXPECT_LE(layout.height, 16.0f);
}

TEST(MenuTextLayout, LongUnbrokenWordAlwaysMakesProgress) {
  EXPECT_EQ(WrapMenuText("abcdefghijklmnop next", 4),
            (std::vector<std::string>{"abcdefghijklmnop", "next"}));
}

TEST(MenuTextLayout, PreservesParagraphsAndBlankLines) {
  EXPECT_EQ(WrapMenuText("one two\n\nthree", 4),
            (std::vector<std::string>{"one", "two", "", "three"}));
  EXPECT_EQ(WrapMenuText("", 0), (std::vector<std::string>{""}));
}
