#pragma once

namespace ui {

/** Draw the static sonar/radar grid (black disc, green overlay, labels). */
void radarDisplayDraw();

/** Redraw aircraft only (blits cached grid; no full-screen clear). */
void radarDisplayRefreshAircraft();

/** Drop cached label fonts after a portal setting changed. Redraw follows. */
void radarDisplayInvalidateStyle();

/**
 * Play the armed alert flash, if any, and clear it. Blocks for the duration set
 * in the portal; `poll_fn` is called every frame so the config portal and the
 * network stay responsive meanwhile.
 */
void radarDisplayPlayAlert(void (*poll_fn)());

}  // namespace ui
