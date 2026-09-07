#ifndef GF_GUI2_GRIDLAYOUT_HPP
#define GF_GUI2_GRIDLAYOUT_HPP

#include <algorithm>
#include <vector>

namespace blunted {

struct GridLayoutCell {
  int row, column;
  float width, height;
};

struct GridLayout {
  std::vector<float> x, y;
  float width = 0.0f;
  float height = 0.0f;
};

// Column widths stay stable while scrolling; only visible rows contribute height.
inline GridLayout CalculateGridLayout(const std::vector<GridLayoutCell>& cells,
                                     int rows, int columns, int firstRow, int visibleRows,
                                     float left, float right, float top, float bottom) {
  GridLayout result;
  result.x.resize(columns);
  result.y.resize(rows);
  std::vector<float> widths(columns, 0.0f), heights(rows, 0.0f);
  for (const auto& cell : cells) {
    if (cell.row < 0 || cell.row >= rows || cell.column < 0 || cell.column >= columns)
      continue;
    widths[cell.column] = std::max(widths[cell.column], cell.width);
    heights[cell.row] = std::max(heights[cell.row], cell.height);
  }
  for (int column = 0; column < columns; ++column) {
    result.x[column] = result.width + left;
    result.width += left + widths[column] + right;
  }
  const int begin = std::clamp(firstRow, 0, rows);
  const int end = begin + std::min(std::max(0, visibleRows), rows - begin);
  for (int row = begin; row < end; ++row) {
    result.y[row] = result.height + top;
    result.height += top + heights[row] + bottom;
  }
  return result;
}

}  // namespace blunted

#endif
