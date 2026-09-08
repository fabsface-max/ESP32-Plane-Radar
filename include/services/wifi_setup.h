#pragma once

/**
 * WPA2 password for the setup access point, derived from this device's MAC so
 * it is stable across reboots and can be printed on the setup screen. An open
 * AP would let anyone in radio range hand the radar a network of their choice.
 */
const char* wifiSetupApPassword();

/** True when the next boot should show the setup screen first (after credential reset). */
bool wifiShowsSetupScreenOnBoot();
void wifiResetCredentialsAndReboot();
/** Boot flow: connect with UI, open portal only if saved creds fail. */
bool wifiSetupConnect();
/**
 * Reconnect using saved creds; never opens the captive portal.
 *
 * @param show_ui replace the radar with the connecting animation. Short drops
 *        are better ridden out with the last picture still on screen.
 */
bool wifiReconnect(bool show_ui);
/** Keeps the LAN config portal alive; call every loop() iteration. */
void wifiLoop();
/** Latched: a portal save changed a display option, so the radar needs a redraw. */
bool wifiConsumeDisplaySettingsChanged();
bool wifiBootButtonPressed();
/** GPIO + interrupt setup; call once early in setup(). */
void bootButtonInit();
/** Latched short tap (survives blocking HTTP/display work). */
bool bootButtonConsumeTap();
/** Call each loop iteration; triggers WiFi reset on long hold. */
void bootButtonPollLongPress();
