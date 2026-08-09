// written by bastiaan konings schuiling 2008 - 2014
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not
// be used for anything important. i do not offer support, so don't ask. to be used for inspiration
// :)

#include "page.hpp"

#include <cmath>
#include <limits>
#include <vector>

#include "base/utils.hpp"
#include "windowmanager.hpp"

namespace blunted {

namespace {

void CollectSelectableLeaves(Gui2View* view, std::vector<Gui2View*>& result) {
  if (!view || !view->IsVisible())
    return;

  const size_t childCountBefore = result.size();
  const std::vector<Gui2View*> children = view->GetChildren();
  for (Gui2View* child : children)
    CollectSelectableLeaves(child, result);

  // Containers such as grids report themselves selectable so focus can enter
  // them. Prefer their actual controls when they have selectable descendants.
  if (view->IsSelectable() && result.size() == childCountBefore)
    result.push_back(view);
}

bool MoveFocusDirectionally(Gui2WindowManager* windowManager, Gui2View* page,
                            const Vector3& direction) {
  Gui2View* current = windowManager->GetFocus();
  if (!current)
    return false;

  const bool horizontal = std::fabs(direction.coords[0]) > std::fabs(direction.coords[1]);
  const float axis = horizontal ? direction.coords[0] : direction.coords[1];
  if (std::fabs(axis) < 0.75f)
    return false;

  float currentX, currentY, currentW, currentH;
  current->GetDerivedPosition(currentX, currentY);
  current->GetSize(currentW, currentH);
  const float currentCenterX = currentX + currentW * 0.5f;
  const float currentCenterY = currentY + currentH * 0.5f;

  std::vector<Gui2View*> candidates;
  CollectSelectableLeaves(page, candidates);

  Gui2View* best = nullptr;
  float bestScore = std::numeric_limits<float>::max();
  const float sign = axis > 0.0f ? 1.0f : -1.0f;
  for (Gui2View* candidate : candidates) {
    if (candidate == current)
      continue;

    float x, y, w, h;
    candidate->GetDerivedPosition(x, y);
    candidate->GetSize(w, h);
    const float deltaX = x + w * 0.5f - currentCenterX;
    const float deltaY = y + h * 0.5f - currentCenterY;
    const float primary = (horizontal ? deltaX : deltaY) * sign;
    if (primary <= 0.01f)
      continue;

    const float cross = std::fabs(horizontal ? deltaY : deltaX);
    const float score = primary + cross * 2.0f;
    if (score < bestScore) {
      best = candidate;
      bestScore = score;
    }
  }

  if (!best)
    return false;

  best->SetFocus();
  return true;
}

}  // namespace

Gui2Page::Gui2Page(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Frame(windowManager, "page_" + int_to_str(pageData.pageID), 0, 0, 100, 100),
      pageData(pageData) {}

Gui2Page::~Gui2Page() {}

void Gui2Page::GoBack() {
  // moved to View::Exit: sig_OnClose();

  this->Exit();

  windowManager->GetPagePath()->Pop();
  if (windowManager->GetPagePath()->GetPath().size() > 0) {
    Gui2PageData prevPage = windowManager->GetPagePath()->GetLast();
    windowManager->GetPagePath()->Pop();  // pop previous page from path too, since it is going to
                                          // be added with the createpage again
    windowManager->GetPageFactory()->CreatePage(prevPage);
  }  // else: no mo menus :[

  delete this;
  return;
}

void Gui2Page::ProcessWindowingEvent(WindowingEvent* event) {
  if (event->IsEscape()) {
    GoBack();
    return;
  }

  if (MoveFocusDirectionally(windowManager, this, event->GetDirection())) {
    event->Accept();
    return;
  }

  event->Ignore();
}

void Gui2Page::CreatePage(int pageID, void* data) {
  Properties properties;
  CreatePage(pageID, properties, data);
}

void Gui2Page::CreatePage(int pageID, const Properties& properties, void* data) {
  this->Exit();

  windowManager->GetPageFactory()->CreatePage(pageID, properties, data);

  delete this;
}

Gui2PageFactory::Gui2PageFactory() {
  mostRecentlyCreatedPage = 0;
}

Gui2PageFactory::~Gui2PageFactory() {}

void Gui2PageFactory::SetWindowManager(Gui2WindowManager* wm) {
  windowManager = wm;
}

Gui2Page* Gui2PageFactory::CreatePage(int pageID, const Properties& properties, void* data) {
  Gui2PageData pageData;
  pageData.pageID = pageID;
  pageData.properties = std::shared_ptr<Properties>(new Properties(properties));
  pageData.data = data;
  Gui2Page* page = CreatePage(pageData);
  mostRecentlyCreatedPage = page;
  return page;
  // not going to work: pointer is not persistent, while pagedata is. pageData.pagePointer =
  // CreatePage(pageData);
  // printf("page created, id %i\n", pageData.pageID);
  // return pageData.pagePointer;
}

}  // namespace blunted
