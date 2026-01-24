#include "ClearRecentListActivity.h"

#include <GfxRenderer.h>
#include <HardwareSerial.h>
#include <SDCardManager.h>

#include "MappedInputManager.h"
#include "RecentBooksStore.h"
#include "fontIds.h"

void ClearRecentListActivity::taskTrampoline(void* param) {
  auto* self = static_cast<ClearRecentListActivity*>(param);
  self->displayTaskLoop();
}

void ClearRecentListActivity::onEnter() {
  ActivityWithSubactivity::onEnter();

  renderingMutex = xSemaphoreCreateMutex();
  state = WARNING;
  updateRequired = true;

  xTaskCreate(&ClearRecentListActivity::taskTrampoline, "ClearRecentListActivityTask",
              4096,               // Stack size
              this,               // Parameters
              1,                  // Priority
              &displayTaskHandle  // Task handle
  );
}

void ClearRecentListActivity::onExit() {
  ActivityWithSubactivity::onExit();

  xSemaphoreTake(renderingMutex, portMAX_DELAY);
  if (displayTaskHandle) {
    vTaskDelete(displayTaskHandle);
    displayTaskHandle = nullptr;
  }
  vSemaphoreDelete(renderingMutex);
  renderingMutex = nullptr;
}

void ClearRecentListActivity::loop() {
  if (state == WARNING) {
    if (mappedInput.wasPressed(MappedInputManager::Button::Back)) {
      goBack();
      return;
    }
    if (mappedInput.wasPressed(MappedInputManager::Button::Confirm)) {
      state = CLEARING;
      updateRequired = true;
      clearRecentList();
      return;
    }
  } else if (state == SUCCESS || state == FAILED) {
    if (mappedInput.wasPressed(MappedInputManager::Button::Back)) {
      goBack();
      return;
    }
  }
}

void ClearRecentListActivity::displayTaskLoop() {
  while (true) {
    if (updateRequired) {
      updateRequired = false;
      xSemaphoreTake(renderingMutex, portMAX_DELAY);
      render();
      xSemaphoreGive(renderingMutex);
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void ClearRecentListActivity::render() {
  const auto pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();
  renderer.drawCenteredText(UI_12_FONT_ID, 15, "Clear Recent List", true, EpdFontFamily::BOLD);

  if (state == WARNING) {
    renderer.drawCenteredText(UI_10_FONT_ID, pageHeight / 2 - 60, "This will delete all recent books", true);
    renderer.drawCenteredText(UI_10_FONT_ID, pageHeight / 2 - 30, "from your library history.", true);
    renderer.drawCenteredText(UI_10_FONT_ID, pageHeight / 2 + 10, "This action cannot be undone.", true,
                              EpdFontFamily::BOLD);

    const auto labels = mappedInput.mapLabels("« Cancel", "Clear", "", "");
    renderer.drawButtonHints(UI_10_FONT_ID, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
    renderer.displayBuffer();
    return;
  }

  if (state == CLEARING) {
    renderer.drawCenteredText(UI_10_FONT_ID, pageHeight / 2, "Clearing recent list...", true, EpdFontFamily::BOLD);
    renderer.displayBuffer();
    return;
  }

  if (state == SUCCESS) {
    renderer.drawCenteredText(UI_10_FONT_ID, pageHeight / 2 - 20, "Recent List Cleared", true, EpdFontFamily::BOLD);
    renderer.drawCenteredText(UI_10_FONT_ID, pageHeight / 2 + 10, "All recent books have been removed");

    const auto labels = mappedInput.mapLabels("« Back", "", "", "");
    renderer.drawButtonHints(UI_10_FONT_ID, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
    renderer.displayBuffer();
    return;
  }

  if (state == FAILED) {
    renderer.drawCenteredText(UI_10_FONT_ID, pageHeight / 2 - 20, "Failed to clear recent list", true, EpdFontFamily::BOLD);
    renderer.drawCenteredText(UI_10_FONT_ID, pageHeight / 2 + 10, "Check serial output for details");

    const auto labels = mappedInput.mapLabels("« Back", "", "", "");
    renderer.drawButtonHints(UI_10_FONT_ID, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
    renderer.displayBuffer();
    return;
  }
}

void ClearRecentListActivity::clearRecentList() {
  Serial.printf("[%lu] [CLEAR_RECENT] Clearing recent books list...\n", millis());

  try {
    // Use the public clear() method from RecentBooksStore
    if (RECENT_BOOKS.clear()) {
      Serial.printf("[%lu] [CLEAR_RECENT] Recent list cleared successfully\n", millis());
      state = SUCCESS;
    } else {
      Serial.printf("[%lu] [CLEAR_RECENT] Failed to clear recent list\n", millis());
      state = FAILED;
    }
  } catch (...) {
    Serial.printf("[%lu] [CLEAR_RECENT] Exception occurred while clearing\n", millis());
    state = FAILED;
  }
  updateRequired = true;
}
