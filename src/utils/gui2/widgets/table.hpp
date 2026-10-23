// A data table in the broadcast style of football games: a header bar,
// banded rows, an optional coloured zone strip per row (title, Europe,
// relegation) and a highlighted row for the user's club or player. Columns
// align left, centre or right. With SetSelectable(true) the up/down keys move
// a selection bar (scrolling when there are more rows than fit) and Activate
// reports the row through sig_OnActivate.

#ifndef _HPP_GUI2_VIEW_TABLE
#define _HPP_GUI2_VIEW_TABLE

#include <string>
#include <vector>

#include "../view.hpp"
#include "caption.hpp"
#include "image.hpp"

namespace blunted {

class Gui2Table : public Gui2View {
public:
  enum class Align { Left, Center, Right };
  struct Column {
    std::string title;
    float width_percent;
    Align align = Align::Left;
  };
  struct Row {
    std::vector<std::string> cells;
    bool highlight = false;     // the user's club or player
    bool hasZone = false;       // draw a coloured strip at the left edge
    Vector3 zoneColor;          // when hasZone
    bool hasTextColor = false;  // otherwise the style's bright colour
    Vector3 textColor;
  };

  Gui2Table(Gui2WindowManager* windowManager, const std::string& name, float x_percent,
            float y_percent, const std::vector<Column>& columns, float rowHeight_percent,
            int maxVisibleRows);
  virtual ~Gui2Table();

  void AddRow(const Row& row);
  int GetRowCount() const { return static_cast<int>(rows.size()); }

  // Must be called after the rows are added.
  void Build();

  void SetSelectable(bool onOff) { isSelectable = onOff; }
  int GetSelectedRow() const { return selectedRow; }
  void SetSelectedRow(int row);

  virtual void Redraw();
  virtual void ProcessWindowingEvent(WindowingEvent* event);
  virtual void OnGainFocus();
  virtual void OnLoseFocus();

  boost::signals2::signal<void(int)> sig_OnActivate;

protected:
  void PlaceCell(Gui2Caption* caption, int column, float y_percent);
  // Shortens text with "..." to fit the column at full text size; a caption
  // would shrink it instead, which made long names unreadable.
  std::string FitText(const std::string& text, int column);

  std::vector<Column> columns;
  std::vector<Row> rows;
  float rowHeight_percent;
  int maxVisibleRows;
  int offsetRow = 0;
  int selectedRow = 0;

  Gui2Image* background = nullptr;
  Gui2Caption* measure = nullptr;  // hidden, natural width
  std::vector<Gui2Caption*> headerCaptions;
  std::vector<std::vector<Gui2Caption*>> cellCaptions;  // [visible row][column]
};

}  // namespace blunted

#endif
