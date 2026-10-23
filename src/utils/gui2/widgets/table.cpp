#include "table.hpp"

#include <algorithm>

#include "../windowmanager.hpp"

namespace blunted {

namespace {

constexpr float kCellPadding = 0.6f;  // percent of screen width
constexpr float kTextScale = 0.72f;   // text height relative to the row
constexpr float kZoneStripWidth = 0.35f;

}  // namespace

Gui2Table::Gui2Table(Gui2WindowManager* windowManager, const std::string& name, float x_percent,
                     float y_percent, const std::vector<Column>& columns, float rowHeight_percent,
                     int maxVisibleRows)
    : Gui2View(windowManager, name, x_percent, y_percent, 0, 0),
      columns(columns),
      rowHeight_percent(rowHeight_percent),
      maxVisibleRows(std::max(1, maxVisibleRows)) {
  for (const auto& column : columns)
    width_percent += column.width_percent;
}

Gui2Table::~Gui2Table() {}

void Gui2Table::AddRow(const Row& row) {
  rows.push_back(row);
}

void Gui2Table::Build() {
  const int visibleRows = std::min(maxVisibleRows, static_cast<int>(rows.size()));
  height_percent = rowHeight_percent * (visibleRows + 1);

  background = new Gui2Image(windowManager, name + "_bg", 0, 0, width_percent, height_percent);
  AddView(background);
  background->Show();

  const float textHeight = rowHeight_percent * kTextScale;
  const float textInset = (rowHeight_percent - textHeight) * 0.5f;
  measure = new Gui2Caption(windowManager, name + "_measure", 0, 0, 0, textHeight, " ");
  AddView(measure);
  measure->Hide();
  for (size_t c = 0; c < columns.size(); c++) {
    auto* caption =
        new Gui2Caption(windowManager, name + "_h" + std::to_string(c), 0, 0,
                        columns[c].width_percent - kCellPadding * 2, textHeight, columns[c].title);
    caption->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
    AddView(caption);
    PlaceCell(caption, static_cast<int>(c), textInset);
    caption->Show();
    headerCaptions.push_back(caption);
  }
  for (int r = 0; r < visibleRows; r++) {
    std::vector<Gui2Caption*> line;
    for (size_t c = 0; c < columns.size(); c++) {
      auto* caption =
          new Gui2Caption(windowManager, name + "_c" + std::to_string(r) + "_" + std::to_string(c),
                          0, 0, columns[c].width_percent - kCellPadding * 2, textHeight, " ");
      AddView(caption);
      caption->Show();
      line.push_back(caption);
    }
    cellCaptions.push_back(line);
  }
  Redraw();
}

void Gui2Table::PlaceCell(Gui2Caption* caption, int column, float y_percent) {
  float x = 0;
  for (int c = 0; c < column; c++)
    x += columns[c].width_percent;
  const float available = columns[column].width_percent - kCellPadding * 2;
  const float textWidth = std::min(available, caption->GetTextWidthPercent());
  float offset = 0;
  if (columns[column].align == Align::Right)
    offset = available - textWidth;
  else if (columns[column].align == Align::Center)
    offset = (available - textWidth) * 0.5f;
  caption->SetPosition(x + kCellPadding + offset, y_percent);
}

std::string Gui2Table::FitText(const std::string& text, int column) {
  if (!measure || text.empty())
    return text;
  const float available = columns[column].width_percent - kCellPadding * 2;
  measure->SetCaption(text);
  if (measure->GetTextWidthPercent() <= available)
    return text;
  std::string shortened = text;
  while (!shortened.empty()) {
    // Drop one UTF-8 character (and any trailing space) at a time.
    do {
      shortened.pop_back();
    } while (!shortened.empty() && (static_cast<unsigned char>(shortened.back()) & 0xC0) == 0x80);
    while (!shortened.empty() && shortened.back() == ' ')
      shortened.pop_back();
    measure->SetCaption(shortened + "...");
    if (measure->GetTextWidthPercent() <= available)
      return shortened + "...";
  }
  return text;
}

void Gui2Table::SetSelectedRow(int row) {
  if (rows.empty())
    return;
  selectedRow = std::clamp(row, 0, static_cast<int>(rows.size()) - 1);
  if (selectedRow < offsetRow)
    offsetRow = selectedRow;
  if (selectedRow >= offsetRow + maxVisibleRows)
    offsetRow = selectedRow - maxVisibleRows + 1;
  Redraw();
}

void Gui2Table::Redraw() {
  if (!background)
    return;
  int x, y, w, h;
  windowManager->GetCoordinates(x_percent, y_percent, width_percent, height_percent, x, y, w, h);
  int rx, ry, rw, rowPixels;
  windowManager->GetCoordinates(0, 0, 0, rowHeight_percent, rx, ry, rw, rowPixels);
  int sx, sy, stripPixels, sh;
  windowManager->GetCoordinates(0, 0, kZoneStripWidth, 0, sx, sy, stripPixels, sh);
  stripPixels = std::max(3, stripPixels);

  auto* style = windowManager->GetStyle();
  const Vector3 gold = style->GetColor(e_DecorationType_Bright2);
  auto& image = background->GetImage2D();
  image->DrawRectangle(0, 0, w, h, Vector3(0, 0, 0), 0);

  // Header bar with a gold rule underneath.
  image->DrawRectangle(0, 0, w, rowPixels, Vector3(28, 44, 82), 235);
  image->DrawRectangle(0, rowPixels - 2, w, 2, gold, 255);

  const float textHeight = rowHeight_percent * kTextScale;
  const float textInset = (rowHeight_percent - textHeight) * 0.5f;
  for (size_t v = 0; v < cellCaptions.size(); v++) {
    const int index = offsetRow + static_cast<int>(v);
    const int top = rowPixels * static_cast<int>(v + 1);
    const int height = (v + 1 == cellCaptions.size()) ? h - top : rowPixels;
    const Row& row = rows[index];
    const bool selected = isSelectable && IsFocussed() && index == selectedRow;

    // Alternating bands; the user's row tinted gold; the selection solid.
    if (selected)
      image->DrawRectangle(0, top, w, height, gold, 255);
    else if (row.section)
      image->DrawRectangle(0, top, w, height, Vector3(28, 44, 82), 235);
    else if (row.highlight)
      image->DrawRectangle(0, top, w, height, Vector3(120, 100, 30), 200);
    else
      image->DrawRectangle(0, top, w, height,
                           index % 2 == 0 ? Vector3(20, 32, 62) : Vector3(13, 22, 46), 210);
    if (row.hasZone)
      image->DrawRectangle(0, top, stripPixels, height, row.zoneColor, 255);

    const Vector3 textColor = row.hasTextColor ? row.textColor
                              : row.section    ? gold
                                               : style->GetColor(e_DecorationType_Bright1);
    for (size_t c = 0; c < columns.size(); c++) {
      Gui2Caption* caption = cellCaptions[v][c];
      caption->SetCaption(c < row.cells.size() && !row.cells[c].empty()
                              ? FitText(row.cells[c], static_cast<int>(c))
                              : " ");
      caption->SetColor(selected ? style->GetColor(e_DecorationType_Bright1) : textColor);
      PlaceCell(caption, static_cast<int>(c), rowHeight_percent * (v + 1) + textInset);
    }
  }
  image->OnChange();
}

void Gui2Table::ProcessWindowingEvent(WindowingEvent* event) {
  if (!isSelectable || rows.empty()) {
    event->Ignore();
    return;
  }
  const Vector3 direction = event->GetDirection();
  if (direction.coords[1] < -0.75f && selectedRow > 0) {
    SetSelectedRow(selectedRow - 1);
  } else if (direction.coords[1] > 0.75f && selectedRow + 1 < static_cast<int>(rows.size())) {
    SetSelectedRow(selectedRow + 1);
  } else if (event->IsActivate()) {
    windowManager->PlayClickSound();
    sig_OnActivate(selectedRow);
  } else {
    // Past the first or last row, or sideways: let the page move focus on.
    event->Ignore();
  }
}

void Gui2Table::OnGainFocus() {
  Redraw();
}

void Gui2Table::OnLoseFocus() {
  Redraw();
}

}  // namespace blunted
