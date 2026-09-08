/**
 * Plane Radar — WiFi setup, then radar UI on the round GC9A01 display.
 */

#include <Arduino.h>
#include <WiFi.h>

#if __has_include(<driver/temp_sensor.h>)
#include <driver/temp_sensor.h>
#define PLANE_RADAR_HAS_TEMP_SENSOR 1
#endif

#include "config.h"
#include "hardware/display.h"
#include "services/adsb_client.h"
#include "services/radar_location.h"
#include "services/wifi_setup.h"
#include "ui/alert.h"
#include "ui/radar_display.h"
#include "ui/radar_range.h"
#include "ui/status_screens.h"
#include "ui/trails.h"

namespace {

bool g_radar_visible = false;
unsigned long g_wifi_down_since = 0;
unsigned long g_last_reconnect_ms = 0;
unsigned long g_last_adsb_fetch_ms = 0;
unsigned long g_last_temp_log_ms = 0;

/** Pin the core clock to its maximum; see config::kCpuFreqFullMhz. */
void applyCpuClock() {
  setCpuFrequencyMhz(config::kCpuFreqFullMhz);
  Serial.printf("CPU clock: %u MHz\n",
                static_cast<unsigned>(getCpuFrequencyMhz()));
}

/** Chip temperature to the serial log, so power tuning can be measured. */
void logChipTemperature() {
  if (millis() - g_last_temp_log_ms < config::kTempLogIntervalMs) {
    return;
  }
  g_last_temp_log_ms = millis();
#ifdef PLANE_RADAR_HAS_TEMP_SENSOR
  float celsius = 0.0f;
  if (temp_sensor_read_celsius(&celsius) == ESP_OK) {
    Serial.printf("Chip temperature: %.1f C\n", celsius);
  }
#endif
}

void initTemperatureSensor() {
#ifdef PLANE_RADAR_HAS_TEMP_SENSOR
  temp_sensor_config_t cfg = TSENS_CONFIG_DEFAULT();
  temp_sensor_set_config(cfg);
  temp_sensor_start();
#endif
}

void showRadarIfConnected() {
  if (WiFi.status() != WL_CONNECTED) {
    g_radar_visible = false;
    return;
  }
  ui::radarDisplayDraw();
  g_radar_visible = true;
}

void onRangeTap() {
  ui::radar::rangeNext();
  char range_label[12];
  ui::radar::formatCurrentRing3Label(range_label, sizeof(range_label));
  Serial.printf("Range: %s (outer ~%.0f km)\n", range_label,
                ui::radar::rangeCurrent().outer_km);

  if (g_radar_visible && WiFi.status() == WL_CONNECTED) {
    ui::radarDisplayDraw();
  }
}

/** Portal saves land in wifiLoop(); apply new label sizes / layers right away. */
void applyDisplaySettingsIfChanged() {
  if (!wifiConsumeDisplaySettingsChanged()) {
    return;
  }
  ui::radarDisplayInvalidateStyle();
  // The radar centre may have moved with the same save, which would leave every
  // stored trail pointing at the wrong place.
  ui::trails::clear();
  ui::alert::reset();
  if (g_radar_visible && WiFi.status() == WL_CONNECTED) {
    ui::radarDisplayDraw();
  }
}

void handleBootButton() {
  bootButtonPollLongPress();
  if (bootButtonConsumeTap()) {
    onRangeTap();
  }
}

/** Keeps the portal and the BOOT button alive during the alert animation. */
void pollDuringAlert() {
  wifiLoop();
  bootButtonPollLongPress();
}

void fetchAndDrawAircraft() {
  const float fetch_km = ui::radar::fetchRadiusKm();
  if (!services::adsb::fetchUpdate(services::location::lat(),
                                   services::location::lon(), fetch_km)) {
    handleBootButton();
    return;
  }
  const unsigned long now = millis();
  ui::trails::update(services::adsb::aircraftList(),
                     services::adsb::aircraftCount(), now);
  ui::alert::scan(services::adsb::aircraftList(),
                  services::adsb::aircraftCount(), now);
  ui::radarDisplayRefreshAircraft();
  ui::radarDisplayPlayAlert(pollDuringAlert);
  handleBootButton();
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("Plane Radar");

  bootButtonInit();
  services::location::init();
  ui::radar::rangeInit();
  applyCpuClock();
  initTemperatureSensor();
  displayInit();
  if (wifiShowsSetupScreenOnBoot()) {
    statusScreenPortal();
  }
  services::adsb::setPollFn(wifiLoop);

  if (wifiSetupConnect()) {
    showRadarIfConnected();
  }
}

void loop() {
  handleBootButton();
  wifiLoop();
  applyDisplaySettingsIfChanged();

  if (WiFi.status() != WL_CONNECTED) {
    if (g_wifi_down_since == 0) {
      g_wifi_down_since = millis();
      Serial.println("WiFi lost - waiting for auto-reconnect");
    }

    const unsigned long down_ms = millis() - g_wifi_down_since;
    // Home networks hiccup for a few seconds all the time. Leave the last
    // radar picture up while that plays out; only an outage that outlasts
    // kWifiConnectingScreenDelayMs is worth taking the screen away for.
    const bool show_ui = down_ms >= config::kWifiConnectingScreenDelayMs;
    if (show_ui && g_radar_visible) {
      g_radar_visible = false;
    }

    if (down_ms >= config::kWifiDownGraceMs &&
        millis() - g_last_reconnect_ms >= config::kWifiReconnectIntervalMs) {
      g_last_reconnect_ms = millis();
      if (wifiReconnect(show_ui)) {
        g_wifi_down_since = 0;
        showRadarIfConnected();
      }
    }
  } else {
    if (g_wifi_down_since != 0) {
      Serial.printf("WiFi back after %lu ms\n", millis() - g_wifi_down_since);
      g_wifi_down_since = 0;
    }
    if (!g_radar_visible) {
      showRadarIfConnected();
    } else if (millis() - g_last_adsb_fetch_ms >= config::kAdsbFetchIntervalMs) {
      g_last_adsb_fetch_ms = millis();
      fetchAndDrawAircraft();
    }
  }

  logChipTemperature();
  delay(10);
}
