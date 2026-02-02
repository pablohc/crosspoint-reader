#include "ScreenComponents.h"

#include <GfxRenderer.h>

#include <cstdint>
#include <string>

#include "Battery.h"
#include "fontIds.h"

void ScreenComponents::drawBattery(const GfxRenderer& renderer, const int left, const int top,
                                   const bool showPercentage) {
  // Left aligned battery icon and percentage
  const uint16_t percentage = battery.readPercentage();
  const auto percentageText = showPercentage ? std::to_string(percentage) + "%" : "";
  renderer.drawText(SMALL_FONT_ID, left + 20, top, percentageText.c_str());

  // 1 column on left, 2 columns on right, 5 columns of battery body
  constexpr int batteryWidth = 15;
  constexpr int batteryHeight = 12;
  const int x = left;
  const int y = top + 6;

  // Top line
  renderer.drawLine(x + 1, y, x + batteryWidth - 3, y);
  // Bottom line
  renderer.drawLine(x + 1, y + batteryHeight - 1, x + batteryWidth - 3, y + batteryHeight - 1);
  // Left line
  renderer.drawLine(x, y + 1, x, y + batteryHeight - 2);
  // Battery end
  renderer.drawLine(x + batteryWidth - 2, y + 1, x + batteryWidth - 2, y + batteryHeight - 2);
  renderer.drawPixel(x + batteryWidth - 1, y + 3);
  renderer.drawPixel(x + batteryWidth - 1, y + batteryHeight - 4);
  renderer.drawLine(x + batteryWidth - 0, y + 4, x + batteryWidth - 0, y + batteryHeight - 5);

  // The +1 is to round up, so that we always fill at least one pixel
  int filledWidth = percentage * (batteryWidth - 5) / 100 + 1;
  if (filledWidth > batteryWidth - 5) {
    filledWidth = batteryWidth - 5;  // Ensure we don't overflow
  }

  renderer.fillRect(x + 2, y + 2, filledWidth, batteryHeight - 4);
}

void ScreenComponents::drawBookProgressBar(const GfxRenderer& renderer, const size_t bookProgress) {
  int vieweableMarginTop, vieweableMarginRight, vieweableMarginBottom, vieweableMarginLeft;
  renderer.getOrientedViewableTRBL(&vieweableMarginTop, &vieweableMarginRight, &vieweableMarginBottom,
                                   &vieweableMarginLeft);

  const int progressBarMaxWidth = renderer.getScreenWidth() - vieweableMarginLeft - vieweableMarginRight;
  const int progressBarY = renderer.getScreenHeight() - vieweableMarginBottom - BOOK_PROGRESS_BAR_HEIGHT;
  const int barWidth = progressBarMaxWidth * bookProgress / 100;
  renderer.fillRect(vieweableMarginLeft, progressBarY, barWidth, BOOK_PROGRESS_BAR_HEIGHT, true);
}

int ScreenComponents::drawTabBar(const GfxRenderer& renderer, const int y, const std::vector<TabInfo>& tabs) {
  constexpr int tabPadding = 20;      // Horizontal padding between tabs
  constexpr int leftMargin = 20;      // Left margin for first tab
  constexpr int underlineHeight = 2;  // Height of selection underline
  constexpr int underlineGap = 4;     // Gap between text and underline

  const int lineHeight = renderer.getLineHeight(UI_12_FONT_ID);
  const int tabBarHeight = lineHeight + underlineGap + underlineHeight;

  int currentX = leftMargin;

  for (const auto& tab : tabs) {
    const int textWidth =
        renderer.getTextWidth(UI_12_FONT_ID, tab.label, tab.selected ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR);

    // Draw tab label
    renderer.drawText(UI_12_FONT_ID, currentX, y, tab.label, true,
                      tab.selected ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR);

    // Draw underline for selected tab
    if (tab.selected) {
      renderer.fillRect(currentX, y + lineHeight + underlineGap, textWidth, underlineHeight);
    }

    currentX += textWidth + tabPadding;
  }

  return tabBarHeight;
}

void ScreenComponents::drawPageFraction(const GfxRenderer& renderer, const int currentPage, const int totalPages,
                                        const int contentTop, const int contentHeight) {
  if (totalPages <= 1) {
    return;  // No need for indicator if only one page
  }

  const int screenWidth = renderer.getScreenWidth();
  constexpr int indicatorWidth = 20;
  constexpr int margin = 15;  // Offset from right edge

  const int centerX = screenWidth - indicatorWidth / 2 - margin;
  const int indicatorTop = contentTop + 60;  // Offset to avoid overlapping side button hints
  const int indicatorBottom = contentTop + contentHeight - 30;

  // Draw page fraction only (e.g., "1/3") - arrows removed
  const std::string pageText = std::to_string(currentPage) + "/" + std::to_string(totalPages);
  const int textWidth = renderer.getTextWidth(SMALL_FONT_ID, pageText.c_str());
  const int textX = centerX - textWidth / 2;
  const int textY = (indicatorTop + indicatorBottom) / 2 - renderer.getLineHeight(SMALL_FONT_ID) / 2;

  // Draw 4-direction outline (up, down, left, right)
  renderer.drawText(SMALL_FONT_ID, textX - 1, textY, pageText.c_str(), false);  // left
  renderer.drawText(SMALL_FONT_ID, textX + 1, textY, pageText.c_str(), false);  // right
  renderer.drawText(SMALL_FONT_ID, textX, textY - 1, pageText.c_str(), false);  // up
  renderer.drawText(SMALL_FONT_ID, textX, textY + 1, pageText.c_str(), false);  // down

  // Draw the main text in black (over the outline)
  renderer.drawText(SMALL_FONT_ID, textX, textY, pageText.c_str());
}

void ScreenComponents::drawProgressBar(const GfxRenderer& renderer, const int x, const int y, const int width,
                                       const int height, const size_t current, const size_t total) {
  if (total == 0) {
    return;
  }

  // Use 64-bit arithmetic to avoid overflow for large files
  const int percent = static_cast<int>((static_cast<uint64_t>(current) * 100) / total);

  // Draw outline
  renderer.drawRect(x, y, width, height);

  // Draw filled portion
  const int fillWidth = (width - 4) * percent / 100;
  if (fillWidth > 0) {
    renderer.fillRect(x + 2, y + 2, fillWidth, height - 4);
  }

  // Draw percentage text centered below bar
  const std::string percentText = std::to_string(percent) + "%";
  renderer.drawCenteredText(UI_10_FONT_ID, y + height + 15, percentText.c_str());
}
