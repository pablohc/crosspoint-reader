#include "CreditsActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>
#include <Logging.h>

#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "fontIds.h"
#include "generated/Contributors.h"
#include "util/ButtonNavigator.h"

static constexpr int SIDE_MARGIN = 10;
static constexpr int GAP_WIDTH = 10;

struct SoupLine {
  int start;
  int end;
};

static SoupLine findNextSoupLine(const GfxRenderer& renderer, const char* soup, int soupLen, int startIdx,
                                 int usableWidth) {
  int charIdx = startIdx;
  int lineWidth = 0;
  while (charIdx < soupLen) {
    const char c[2] = {soup[charIdx], '\0'};
    int charW = renderer.getTextWidth(UI_10_FONT_ID, c);
    if (lineWidth + charW > usableWidth && charIdx > startIdx) break;
    lineWidth += charW;
    charIdx++;
  }
  int lineLen = charIdx - startIdx;
  if (lineLen > 0 && lineLen <= 255) {
    char buf[256];
    memcpy(buf, &soup[startIdx], lineLen);
    buf[lineLen] = '\0';
    while (lineLen > 1 && renderer.getTextWidth(UI_10_FONT_ID, buf) > usableWidth) {
      lineLen--;
      buf[lineLen] = '\0';
      charIdx--;
    }
  }
  return {startIdx, charIdx};
}

void CreditsActivity::onEnter() {
  Activity::onEnter();

  soupScrollOffset = 0;
  listScrollOffset = 0;
  currentView = SOUP;

  requestUpdate();
}

void CreditsActivity::onExit() { Activity::onExit(); }

void CreditsActivity::loop() {
  if (mappedInput.wasPressed(MappedInputManager::Button::Back)) {
    goBack();
    return;
  }

  if (mappedInput.wasPressed(MappedInputManager::Button::Confirm)) {
    toggleView();
    return;
  }

  if (currentView == SOUP) {
    int totalHeight = getSoupTotalHeight();
    int contentHeight = getContentBottom() - getContentTop();
    if (totalHeight > contentHeight) {
      buttonNavigator.onNextRelease([this] {
        soupScrollOffset += SCROLL_PX_STEP;
        requestUpdate();
      });
      buttonNavigator.onPreviousRelease([this] {
        soupScrollOffset = max(0, soupScrollOffset - SCROLL_PX_STEP);
        requestUpdate();
      });
    }
  } else {
    const int maxOffset = max(0, static_cast<int>(CONTRIBUTORS_COUNT) - getVisibleLines());
    if (maxOffset > 0) {
      buttonNavigator.onNextRelease([this, maxOffset] {
        listScrollOffset = min(listScrollOffset + 1, maxOffset);
        requestUpdate();
      });
      buttonNavigator.onPreviousRelease([this] {
        listScrollOffset = max(0, listScrollOffset - 1);
        requestUpdate();
      });
    }
  }
}

int CreditsActivity::getContentTop() const {
  const auto& metrics = UITheme::getInstance().getMetrics();
  return metrics.topPadding + metrics.headerHeight + 5;
}

int CreditsActivity::getContentBottom() const {
  const auto& metrics = UITheme::getInstance().getMetrics();
  return renderer.getScreenHeight() - metrics.buttonHintsHeight - 5;
}

int CreditsActivity::getVisibleLines() const {
  return (getContentBottom() - getContentTop()) / renderer.getLineHeight(UI_10_FONT_ID);
}

static int getSoupLength() {
  int len = 0;
  while (CONTRIBUTORS_SOUP[len] != '\0') len++;
  return len;
}

int CreditsActivity::getSoupTotalHeight() const {
  const int lineHeight = renderer.getLineHeight(UI_10_FONT_ID);
  const int usableWidth = renderer.getScreenWidth() - SIDE_MARGIN * 2;
  const int soupLen = getSoupLength();
  int lines = 0;
  int charIdx = 0;

  while (charIdx < soupLen) {
    SoupLine line = findNextSoupLine(renderer, CONTRIBUTORS_SOUP, soupLen, charIdx, usableWidth);
    charIdx = line.end;
    lines++;
  }

  return lines * lineHeight;
}

void CreditsActivity::render(RenderLock&&) {
  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();

  const auto& metrics = UITheme::getInstance().getMetrics();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight},
                 currentView == SOUP ? "Contributor Soup" : "First Contribution");

  // cppcheck-suppress knownConditionTrueFalse; CONTRIBUTORS_COUNT varies at runtime
  if (CONTRIBUTORS_COUNT == 0) {
    renderer.drawCenteredText(UI_10_FONT_ID, pageHeight / 2, "No data", true);
    const auto labels = mappedInput.mapLabels("Back", "", "", "");
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
    renderer.displayBuffer();
    return;
  }

  const int contentTop = getContentTop();
  const int contentBottom = getContentBottom();
  const int lineHeight = renderer.getLineHeight(UI_10_FONT_ID);
  const int contentHeight = contentBottom - contentTop;

  bool canScroll = false;

  if (currentView == SOUP) {
    const int soupLen = getSoupLength();
    const int usableWidth = pageWidth - SIDE_MARGIN * 2;

    int totalHeight = getSoupTotalHeight();
    canScroll = totalHeight > contentHeight;
    if (!canScroll) soupScrollOffset = 0;

    int y = contentTop - soupScrollOffset;
    int charIdx = 0;

    while (charIdx < soupLen && y < contentBottom + lineHeight) {
      SoupLine line = findNextSoupLine(renderer, CONTRIBUTORS_SOUP, soupLen, charIdx, usableWidth);

      if (y + lineHeight > contentTop && y < contentBottom) {
        int lineLen = line.end - line.start;
        char lineBuf[256];
        if (lineLen > 255) lineLen = 255;
        memcpy(lineBuf, &CONTRIBUTORS_SOUP[line.start], lineLen);
        lineBuf[lineLen] = '\0';
        renderer.drawText(UI_10_FONT_ID, SIDE_MARGIN, y, lineBuf);
      }
      charIdx = line.end;
      y += lineHeight;
    }
  } else {
    const int visibleLines = getVisibleLines();
    const int centerX = pageWidth / 2;

    canScroll = static_cast<int>(CONTRIBUTORS_COUNT) > visibleLines;

    for (int row = 0; row < visibleLines; row++) {
      const int idx = listScrollOffset + row;
      // cppcheck-suppress knownConditionTrueFalse; CONTRIBUTORS_COUNT varies at runtime
      if (idx < 0 || idx >= static_cast<int>(CONTRIBUTORS_COUNT)) break;

      const int y = contentTop + row * lineHeight;

      const char* name = CONTRIBUTORS[idx].name;
      const char* date = CONTRIBUTORS[idx].date;

      int nameWidth = renderer.getTextWidth(UI_10_FONT_ID, name);
      int dateWidth = renderer.getTextWidth(UI_10_FONT_ID, date);

      int nameX = centerX - GAP_WIDTH / 2 - nameWidth;
      int dateX = centerX + GAP_WIDTH / 2;

      if (nameX < SIDE_MARGIN) nameX = SIDE_MARGIN;
      if (dateX + dateWidth > pageWidth - SIDE_MARGIN) dateX = pageWidth - SIDE_MARGIN - dateWidth;

      renderer.drawText(UI_10_FONT_ID, nameX, y, name);
      renderer.drawText(UI_10_FONT_ID, dateX, y, date);
    }
  }

  // cppcheck-suppress knownConditionTrueFalse; currentView changes via toggleView()
  const char* confirmLabel = currentView == SOUP ? "Timeline" : "Soup";
  if (canScroll) {
    const auto labels = mappedInput.mapLabels(tr(STR_BACK), confirmLabel, tr(STR_DIR_UP), tr(STR_DIR_DOWN));
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  } else {
    const auto labels = mappedInput.mapLabels(tr(STR_BACK), confirmLabel, "", "");
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  }
  renderer.displayBuffer();
}

void CreditsActivity::toggleView() {
  if (currentView == SOUP) {
    currentView = DATED;
    listScrollOffset = 0;
  } else {
    currentView = SOUP;
    soupScrollOffset = 0;
  }
  requestUpdate();
}