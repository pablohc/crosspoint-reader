#pragma once

#include <string>

#include "../Activity.h"

class Bitmap;

struct BookOverlayInfo {
  std::string title;
  std::string author;
  std::string progressText;
  std::string chapterName;
  std::string progressSuffix;
};

class SleepActivity final : public Activity {
 public:
  explicit SleepActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Sleep", renderer, mappedInput) {}
  void onEnter() override;

 private:
  void renderLogoSleepScreen() const;
  bool renderCustomSleepScreen() const;
  bool renderCoverSleepScreen() const;
  void renderBitmapSleepScreen(const Bitmap& bitmap, bool allowCrop) const;
  void renderBitmapSleepScreen(const Bitmap& bitmap, const BookOverlayInfo& overlayInfo) const;
  void renderBlankSleepScreen() const;
  BookOverlayInfo getBookOverlayInfo(const std::string& bookPath) const;
};
