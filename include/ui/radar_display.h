#pragma once

namespace ui {

/** Draw the static sonar/radar grid (black disc, green overlay, labels). */
void radarDisplayDraw();

/** Redraw aircraft only (blits cached grid; no full-screen clear). */
void radarDisplayRefreshAircraft();

/** Drop cached label fonts after a portal setting changed. Redraw follows. */
void radarDisplayInvalidateStyle();

}  // namespace ui
