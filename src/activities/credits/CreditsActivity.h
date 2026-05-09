#pragma once

#include "MappedInputManager.h"
#include "activities/Activity.h"
#include "util/ButtonNavigator.h"

class CreditsActivity final : public Activity {
 public:
  explicit CreditsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Credits", renderer, mappedInput) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  bool skipLoopDelay() override { return true; }
  void render(RenderLock&&) override;

 private:
  enum ViewMode { SOUP, DATED };
  ViewMode currentView = SOUP;

  int soupScrollOffset = 0;
  int listScrollOffset = 0;
  static constexpr int SCROLL_PX_STEP = 20;

  ButtonNavigator buttonNavigator;

  void goBack() { finish(); }
  void toggleView();
  int getContentTop() const;
  int getContentBottom() const;
  int getVisibleLines() const;
  int getSoupTotalHeight() const;
};