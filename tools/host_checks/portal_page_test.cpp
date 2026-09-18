/**
 * Host check for the portal settings page.
 *
 * The bug this guards against was silent: WiFiManager built the settings page
 * as one Arduino String, String::concat gives no error when the heap cannot
 * grow, and the page simply stopped after six controls — no Save button, no
 * warning anywhere. Rendering the page off the board and asserting that every
 * control and the closing tag are present turns that failure into a red build.
 *
 * Build and run (also run by .github/workflows/quality.yml):
 *   g++ -std=gnu++17 -Wall -Wextra -Iinclude -Itools/host_checks/stubs \
 *       tools/host_checks/portal_page_test.cpp src/services/portal_pages.cpp -o /tmp/portal_page_test
 *   /tmp/portal_page_test
 */

#include <Arduino.h>
#include <WebServer.h>

#include <cstdio>
#include <string>
#include <vector>

#include "services/portal_pages.h"

HostSerial Serial;

// Stand-ins for the firmware's stored settings, with values chosen so a
// mixed-up field shows up: not every checkbox is on, and no two drop-downs
// select the same position.
namespace services::location {
double lat() { return 52.520000; }
double lon() { return 13.405000; }
bool saveFromStrings(const char*, const char*) { return true; }
}  // namespace services::location

namespace ui::radar {
bool useMiles() { return false; }
bool showRunways() { return true; }
bool showTrackVectors() { return false; }
bool showTrails() { return true; }
bool showClassIcons() { return true; }
uint8_t alertSeconds() { return 5; }
uint8_t fontStep() { return 1; }
uint8_t txPowerDbm() { return 13; }
void saveMilesFromPortal(const char*) {}
void saveRunwaysFromPortal(const char*) {}
void saveTrackVectorsFromPortal(const char*) {}
void saveTrailsFromPortal(const char*) {}
void saveClassIconsFromPortal(const char*) {}
void saveAlertSecondsFromPortal(const char*) {}
void saveFontStepFromPortal(const char*) {}
void saveTxPowerFromPortal(const char*) {}
}  // namespace ui::radar

namespace {

int g_failures = 0;

void expectContains(const std::string& page, const char* needle,
                    const char* what) {
  if (page.find(needle) == std::string::npos) {
    std::printf("FAIL  %s\n      missing: %s\n", what, needle);
    ++g_failures;
  }
}

}  // namespace

int main() {
  WebServer server;
  services::portal::registerRoutes(server, nullptr);

  if (server.routes.count("/settings") == 0 ||
      server.routes.count("/settingssave") == 0 ||
      server.routes.count("/param") == 0) {
    std::printf("FAIL  portal routes were not registered\n");
    return 1;
  }

  server.routes["/settings"]();
  const std::string& page = server.body;

  // Every control the settings page owes the user, in the order they appear.
  const std::vector<std::pair<const char*, const char*>> controls = {
      {"name='lat'", "latitude field"},
      {"name='lon'", "longitude field"},
      {"name='font'", "text size"},
      {"name='miles'", "miles checkbox"},
      {"name='runways'", "runways checkbox"},
      {"name='icons'", "class icons checkbox"},
      {"name='track'", "direction lines checkbox"},
      {"name='trails'", "trails checkbox"},
      {"name='alert'", "alert duration"},
      {"name='tx'", "transmit power"},
      {"name='tok'", "CSRF token"},
  };
  for (const auto& control : controls) {
    expectContains(page, control.first, control.second);
  }

  // The page must reach its end: this is the truncation the check exists for.
  expectContains(page, "<button type='submit'>Save</button>", "save button");
  expectContains(page, "</form>", "closed form");
  expectContains(page, "</html>", "closed document");

  // Stored values must reach the form, or the page would quietly reset
  // settings the moment somebody pressed Save.
  expectContains(page, "value='52.520000'", "latitude prefilled");
  expectContains(page, "value='13.405000'", "longitude prefilled");
  expectContains(page, "value='2' selected", "text size step 2 selected");
  expectContains(page, "value='5' selected", "5 second alert selected");
  expectContains(page, "value='13' selected", "13 dBm selected");
  expectContains(page, "name='runways' value='T' checked", "runways ticked");
  expectContains(page, "name='track' value='T'>", "direction lines unticked");

  // Navigation on every page was the other half of the complaint.
  expectContains(page, "href='/wifi'", "Wi-Fi link");
  expectContains(page, "href='/info'", "System link");

  if (g_failures != 0) {
    std::printf("\n%d check(s) failed; page was %zu bytes\n", g_failures,
                page.size());
    return 1;
  }
  std::printf("portal settings page: %zu bytes, %zu controls, all present\n",
              page.size(), controls.size());
  return 0;
}
