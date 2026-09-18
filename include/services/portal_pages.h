#pragma once

#include <WebServer.h>

/**
 * The firmware's own pages inside the Wi-Fi portal.
 *
 * WiFiManager builds each page by concatenating one Arduino String and sends it
 * in a single response. Arduino's String::concat fails silently when the heap
 * cannot grow the buffer, so once the settings form outgrew the largest free
 * block the page simply stopped mid-way — no error, no Save button. The 240x240
 * frame sprite alone holds 115 KB, so that block is not coming back.
 *
 * The settings page here is written straight to the socket in small chunks
 * instead, so the memory it needs does not depend on how long the page is.
 */
namespace services::portal {

/**
 * Markup injected into the <head> of WiFiManager's own pages: the shared
 * styles plus the script that prepends the navigation bar. Our pages emit the
 * same navigation directly and do not need the script.
 */
const char* headHtml();

/**
 * Register /settings, /settingssave and the /param redirect on the portal's
 * web server. Call from WiFiManager's web-server callback, which runs before
 * the library registers its own routes: the ESP32 web server dispatches to the
 * first handler that matches, so registering /param here shadows the library's
 * parameter page for good.
 *
 * @param on_saved called after a successful save, so the caller can apply the
 *        settings that take effect without a restart.
 */
void registerRoutes(WebServer& server, void (*on_saved)());

}  // namespace services::portal
