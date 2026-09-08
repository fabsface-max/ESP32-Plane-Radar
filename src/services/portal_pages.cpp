#include "services/portal_pages.h"

#include <Arduino.h>

#include <cstdio>

#if __has_include(<esp_random.h>)
#include <esp_random.h>
#else
#include <esp_system.h>
#endif

#include "services/radar_location.h"
#include "ui/radar_range.h"
#include "util/portal_input.h"

namespace services::portal {

namespace {

constexpr char kSettingsPath[] = "/settings";
constexpr char kSavePath[] = "/settingssave";

/**
 * Navigation shown on every portal page. WiFiManager renders each page as a
 * dead end with at most a Back button, which is how the settings ended up
 * hidden; these four links are the whole map of the interface.
 */
constexpr char kNavHtml[] =
    "<div id='nv'>"
    "<a href='/'>Home</a>"
    "<a href='/wifi'>Wi-Fi</a>"
    "<a class='on' href='/settings'>Settings</a>"
    "<a href='/info'>System</a>"
    "</div>";

/** Styles for our own pages: same palette as WiFiManager, nothing loaded. */
constexpr char kStyleHtml[] =
    "<style>"
    "body{background:#fff;color:#222;font-family:system-ui,-apple-system,"
    "sans-serif;margin:0;padding:16px}"
    ".wrap{max-width:430px;margin:0 auto;text-align:left}"
    "h1{font-size:20px;margin:0 0 14px}"
    "#nv{display:flex;gap:5px;margin:0 0 16px}"
    "#nv a{flex:1;text-align:center;padding:9px 4px;border-radius:8px;"
    "background:#f2f2f2;border:1px solid #dcdcdc;color:#444;"
    "text-decoration:none;font-size:14px}"
    "#nv a.on{background:#1fa3ec;border-color:#1fa3ec;color:#fff}"
    ".sc{margin:22px 0 8px;font-size:12px;font-weight:700;letter-spacing:.08em;"
    "text-transform:uppercase;color:#777;border-bottom:1px solid #e0e0e0;"
    "padding-bottom:5px}"
    ".hn{font-size:12px;line-height:1.45;color:#888;margin:2px 0 12px}"
    "label{display:block;font-size:14px;margin:10px 0 3px}"
    "label.cb{display:flex;align-items:center;gap:8px;margin:11px 0}"
    "input[type=number],select{width:100%;box-sizing:border-box;padding:8px;"
    "font-size:15px;border:1px solid #ccc;border-radius:6px;background:#fff;"
    "color:#222}"
    "input[type=checkbox]{width:18px;height:18px;margin:0;flex:none}"
    "button{width:100%;margin:24px 0 8px;padding:12px;font-size:16px;"
    "border:0;border-radius:6px;background:#1fa3ec;color:#fff;cursor:pointer}"
    ".ok{margin:0 0 14px;padding:10px 12px;border-radius:6px;font-size:14px;"
    "background:#e6f6e6;border:1px solid #bfe3bf;color:#245c24}"
    "</style>";

/**
 * Cross-site request forgery token, one per boot.
 *
 * The portal has no login — anyone on the same network can open it, which is
 * the same trust model WiFiManager ships with. Without a token, though, any web
 * page the user happens to visit could silently POST to the device and change
 * its settings, because a browser attaches no origin restriction to a plain
 * form submit. The token is only readable by fetching the settings page itself,
 * which the browser's same-origin policy denies to a foreign page.
 */
char s_form_token[9] = {};

const char* formToken() {
  if (s_form_token[0] == '\0') {
    snprintf(s_form_token, sizeof(s_form_token), "%08lx",
             static_cast<unsigned long>(esp_random()));
  }
  return s_form_token;
}

void sendCheckbox(WebServer& server, const char* name, const char* label,
                  bool checked) {
  char buf[220];
  snprintf(buf, sizeof(buf),
           "<label class='cb'><input type='checkbox' name='%s' value='T'%s>"
           "<span>%s</span></label>",
           name, checked ? " checked" : "", label);
  server.sendContent(buf);
}

/**
 * A drop-down over a fixed set of numbers. The stored value is validated again
 * on save, so a hand-crafted POST gains nothing by picking another option.
 */
void sendChoiceSelect(WebServer& server, const char* name, const char* label,
                      const uint8_t* choices, size_t count, uint8_t current,
                      const char* const* option_labels) {
  char buf[200];
  snprintf(buf, sizeof(buf), "<label for='%s'>%s</label><select id='%s' name='%s'>",
           name, label, name, name);
  server.sendContent(buf);
  for (size_t i = 0; i < count; ++i) {
    snprintf(buf, sizeof(buf), "<option value='%u'%s>%s</option>",
             static_cast<unsigned>(choices[i]),
             choices[i] == current ? " selected" : "", option_labels[i]);
    server.sendContent(buf);
  }
  server.sendContent("</select>");
}

void handleSettings(WebServer& server) {
  char buf[256];

  // Chunked output: the page never exists in RAM as a whole, so its length is
  // independent of how fragmented the heap is by the time it is requested.
  server.sendHeader("Cache-Control", "no-store");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  server.sendContent(
      "<!DOCTYPE html><html lang='en'><head><meta charset='utf-8'>"
      "<meta name='viewport' content='width=device-width,initial-scale=1'>"
      "<title>Settings - Plane Radar</title>");
  server.sendContent(kStyleHtml);
  server.sendContent("</head><body><div class='wrap'><h1>Plane Radar</h1>");
  server.sendContent(kNavHtml);

  if (server.hasArg("saved")) {
    server.sendContent("<div class='ok'>Settings saved.</div>");
  }

  snprintf(buf, sizeof(buf),
           "<form method='POST' action='%s'>"
           "<input type='hidden' name='tok' value='%s'>",
           kSavePath, formToken());
  server.sendContent(buf);

  server.sendContent(
      "<div class='sc'>Location</div>"
      "<div class='hn'>Where the radar is centred, in decimal degrees. "
      "Negative for south and west.</div>");
  snprintf(buf, sizeof(buf),
           "<label for='lat'>Latitude</label>"
           "<input id='lat' name='lat' type='number' step='0.000001' "
           "min='-90' max='90' value='%.6f'>",
           services::location::lat());
  server.sendContent(buf);
  snprintf(buf, sizeof(buf),
           "<label for='lon'>Longitude</label>"
           "<input id='lon' name='lon' type='number' step='0.000001' "
           "min='-180' max='180' value='%.6f'>",
           services::location::lon());
  server.sendContent(buf);

  server.sendContent("<div class='sc'>Display</div>");
  {
    static const uint8_t kFontChoices[] = {1, 2, 3};
    static const char* const kFontLabels[] = {"1 - normal", "2 - small",
                                              "3 - smallest"};
    sendChoiceSelect(server, "font", "Text size", kFontChoices,
                     sizeof(kFontChoices) / sizeof(kFontChoices[0]),
                     static_cast<uint8_t>(ui::radar::fontStep() + 1),
                     kFontLabels);
  }
  sendCheckbox(server, "miles", "Distances in miles instead of kilometres",
               ui::radar::useMiles());
  sendCheckbox(server, "runways", "Show airport runways",
               ui::radar::showRunways());
  sendCheckbox(server, "icons",
               "Separate symbols for helicopters and heavies",
               ui::radar::showClassIcons());
  sendCheckbox(server, "track", "Show red direction lines",
               ui::radar::showTrackVectors());
  sendCheckbox(server, "trails", "Show grey flight trails",
               ui::radar::showTrails());

  server.sendContent(
      "<div class='sc'>Alerts</div>"
      "<div class='hn'>The radar pulses a coloured ring when an emergency, a "
      "military aircraft or a rare type first appears.</div>");
  {
    static const char* const kAlertLabels[] = {"Off", "3 seconds", "5 seconds",
                                              "7 seconds"};
    sendChoiceSelect(server, "alert", "Flash duration",
                     ui::radar::kAlertSecondsChoices,
                     ui::radar::kAlertSecondsChoiceCount,
                     ui::radar::alertSeconds(), kAlertLabels);
  }

  server.sendContent(
      "<div class='sc'>Network</div>"
      "<div class='hn'>Raise the transmit power if the connection drops in a "
      "weak spot; it costs current, and therefore heat.</div>");
  {
    static const char* const kTxLabels[] = {"8 dBm - low", "13 dBm - medium",
                                           "19 dBm - high"};
    sendChoiceSelect(server, "tx", "Wi-Fi transmit power",
                     ui::radar::kTxPowerChoices, ui::radar::kTxPowerChoiceCount,
                     ui::radar::txPowerDbm(), kTxLabels);
  }

  server.sendContent(
      "<button type='submit'>Save</button></form>"
      "<div class='hn'>Saved settings survive a restart. Hold the BOOT button "
      "for five seconds to clear the Wi-Fi credentials and these settings."
      "</div></div></body></html>");
  server.sendContent("");
}

/** True when the named checkbox arrived ticked. Absent means unticked. */
bool checkboxArg(WebServer& server, const char* name) {
  if (!server.hasArg(name)) {
    return false;
  }
  const String value = server.arg(name);
  return util::portal::checkboxChecked(value.c_str());
}

void handleSave(WebServer& server, void (*on_saved)()) {
  const String token = server.arg("tok");
  if (token != formToken()) {
    server.send(403, "text/plain",
                "Stale settings form. Reload /settings and try again.");
    return;
  }

  const String lat = server.arg("lat");
  const String lon = server.arg("lon");
  if (!services::location::saveFromStrings(lat.c_str(), lon.c_str())) {
    Serial.println("Invalid lat/lon from portal - keeping previous location");
  }

  // Every value goes through the same validating parsers the host test suite
  // covers; anything malformed leaves the stored setting untouched.
  ui::radar::saveFontStepFromPortal(server.arg("font").c_str());
  ui::radar::saveMilesFromPortal(checkboxArg(server, "miles") ? "T" : "");
  ui::radar::saveRunwaysFromPortal(checkboxArg(server, "runways") ? "T" : "");
  ui::radar::saveClassIconsFromPortal(checkboxArg(server, "icons") ? "T" : "");
  ui::radar::saveTrackVectorsFromPortal(checkboxArg(server, "track") ? "T" : "");
  ui::radar::saveTrailsFromPortal(checkboxArg(server, "trails") ? "T" : "");
  ui::radar::saveAlertSecondsFromPortal(server.arg("alert").c_str());
  ui::radar::saveTxPowerFromPortal(server.arg("tx").c_str());

  if (on_saved != nullptr) {
    on_saved();
  }

  char location[64];
  snprintf(location, sizeof(location), "%s?saved=1", kSettingsPath);
  server.sendHeader("Location", location);
  server.send(302, "text/plain", "");
}

}  // namespace

const char* headHtml() {
  /**
   * WiFiManager's own pages get the navigation from this script, because the
   * library offers no hook to add markup inside the page body.
   */
  static constexpr char kHead[] =
      "<style>"
      ".wrap{max-width:430px}"
      "#nv{display:flex;gap:5px;margin:0 0 16px}"
      "#nv a{flex:1;text-align:center;padding:9px 4px;border-radius:8px;"
      "background:#f2f2f2;border:1px solid #dcdcdc;color:#444;"
      "text-decoration:none;font-size:14px}"
      "#nv a.on{background:#1fa3ec;border-color:#1fa3ec;color:#fff}"
      "body.invert #nv a{background:#2b2b2b;border-color:#3a3a3a;color:#ddd}"
      "</style>"
      "<script>"
      "addEventListener('DOMContentLoaded',function(){"
      "var m=[['/','Home'],['/wifi','Wi-Fi'],['/settings','Settings'],"
      "['/info','System']],w=document.querySelector('.wrap');if(!w)return;"
      "var n=document.createElement('div');n.id='nv';"
      "m.forEach(function(e){var a=document.createElement('a');"
      "a.href=e[0];a.textContent=e[1];"
      "if(location.pathname==e[0])a.className='on';n.appendChild(a)});"
      "w.insertBefore(n,w.firstChild)});"
      "</script>";
  return kHead;
}

void registerRoutes(WebServer& server, void (*on_saved)()) {
  server.on(kSettingsPath, HTTP_GET, [&server]() { handleSettings(server); });
  server.on(kSavePath, HTTP_POST,
            [&server, on_saved]() { handleSave(server, on_saved); });
  // WiFiManager still registers its parameter page. It carries no parameters
  // any more, so send anyone who lands there to the real one.
  server.on("/param", HTTP_ANY, [&server]() {
    server.sendHeader("Location", kSettingsPath);
    server.send(302, "text/plain", "");
  });
}

}  // namespace services::portal
