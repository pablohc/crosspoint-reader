#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <functional>
#include <string>
#include <vector>

#include "activities/ActivityWithSubactivity.h"

class CrossPointSettings;
struct SettingInfo;

class SettingsActivity final : public ActivityWithSubactivity {
  TaskHandle_t displayTaskHandle = nullptr;
  SemaphoreHandle_t renderingMutex = nullptr;
  bool updateRequired = false;
  int selectedCategoryIndex = 0;  // Currently selected category
  int initialCategory = 0;  // Initial category to open (default: 0)
  int initialSettingIndex = -1;  // Initial setting to highlight (-1 = none)
  bool shouldEnterInitialCategory = false;  // Flag to enter initial category on first loop
  bool skipCategoryMenu = false;  // If true, go directly to initialCategory without showing menu
  const std::function<void()> onGoHome;

  static constexpr int categoryCount = 4;
  static const char* categoryNames[categoryCount];

  static void taskTrampoline(void* param);
  [[noreturn]] void displayTaskLoop();
  void render() const;
  void enterCategory(int categoryIndex);

 public:
  explicit SettingsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                            const std::function<void()>& onGoHome, int initialCat = 0, int initialSetting = -1, bool skipMenu = false)
      : ActivityWithSubactivity("Settings", renderer, mappedInput), onGoHome(onGoHome), initialCategory(initialCat), initialSettingIndex(initialSetting), skipCategoryMenu(skipMenu) {}
  void onEnter() override;
  void onExit() override;
  void loop() override;
  
  // Static helper to find Magic Key setting index dynamically
  // Protects against future Controls settings additions
  static int getMagicKeySettingIndex();
};
