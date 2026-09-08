# Quality pipeline

Five stages, cheapest first. Stages 1–4 run in CI on every pull request; stage 5
is a human checklist, because nothing in CI can see the panel.

The ordering is deliberate: a class of defect caught in stage 2 costs seconds,
the same defect caught in stage 5 costs a flash cycle and a pair of eyes.

## Stage 1 — Compiler warnings are errors (project code)

`platformio.ini` builds with `-Wall -Wextra`. The **Build** workflow greps the
build log and fails on any `warning:` or `error:` whose path is under `src/` or
`include/`. Third-party libraries warn plenty and are not ours to fix, so they
are deliberately not matched.

Locally: `pio run -e supermini` and read the output.

## Stage 2 — Host unit tests

`test/test_util/` covers the parsers that sit behind the Wi-Fi setup portal —
`util::portal::checkboxChecked`, `util::portal::stepIndex`,
`util::coords::parseCoordinate`, `util::coords::validLatLon`. They run on the
build machine, so every malformed and hostile field value can be exercised
without a board, a network, or a flash cycle.

```bash
pio test -e native
```

Anything reachable from the portal or from the network belongs here. Keep such
logic free of Arduino headers (`include/util/`, `src/util/`) so it stays
testable; the rule of thumb is that a parser or a piece of geometry that needs
`WiFi.h` to compile is in the wrong file.

### The settings page renders whole

The settings page is the one piece of portal code that cannot be made free of
Arduino headers, so `tools/host_checks/` supplies just enough of `WebServer` and
`String` to render it on the build machine and assert that every control, the
Save button and the closing tag are there.

```bash
g++ -std=gnu++17 -Wall -Wextra -Werror -I include -I tools/host_checks/stubs \
    tools/host_checks/portal_page_test.cpp src/services/portal_pages.cpp \
    -o /tmp/portal_page_test && /tmp/portal_page_test
```

This exists because of a defect that no compiler and no static analyser could
see: WiFiManager assembled the page as one Arduino `String`, and `String` gives
no error when the heap cannot grow it. The page stopped after six controls, with
no Save button and nothing in the log. The page is now streamed in small chunks;
the check fails the build if it ever stops short again.

## Stage 3 — Static analysis

`cppcheck` over `src/` and `include/` with `--enable=warning,performance,portability`,
failing the build on any finding. Third-party headers are not checked out in
that job, so unresolved includes are suppressed; everything cppcheck can still
prove about this project's own code has to come back clean.

A finding that is genuinely a false positive gets an inline
`// cppcheck-suppress <id>` with a comment saying why — never a blanket
suppression in the workflow.

## Stage 4 — Firmware build and artifact

`pio run -e supermini`, then `pio run -t merge` for the web-flashable image.
Fails on link errors and on overflowing a partition, and publishes
`plane-radar-supermini` (including `firmware-merged.bin`) for download.

Watch the reported RAM figure. It excludes the 115 KB frame sprite, which is
allocated at runtime — headroom is tighter than the percentage suggests.

## Stage 5 — Hardware smoke test

CI cannot see the display, reach a Wi-Fi network, or press BOOT. Before merging
anything that touches the UI, the portal, or the network path, flash the build
and walk this list:

- [ ] Boot with no stored credentials → yellow setup screen shows the AP name,
      the **AP password**, and the URL, all fully on screen and readable.
- [ ] Join the setup AP with that password → portal opens; saving Wi-Fi and a
      location connects and the radar appears.
- [ ] Portal reachable again at `http://plane-radar.local` once on the LAN, and
      its menu offers no **Update** (OTA) entry.
- [ ] The navigation bar (Home / Wi-Fi / Settings / System) appears on all four
      pages, the current page is highlighted, and every link works. If a phone
      blocks scripts the bar is absent — the pages still work through the Home
      buttons.
- [ ] The Settings page shows the four group headings and every option; saving
      from there applies immediately without touching the Wi-Fi credentials.
- [ ] `http://<device-ip>/update`, `/u`, `/erase` and `/restart` each return
      404 — these are the routes WiFiManager would otherwise expose unauthenticated.
- [ ] Change **Text size** to 1, 2 and 3 → labels resize immediately, no reboot,
      nothing clipped at the round edge, aircraft tags do not collide.
- [ ] Untick **Show aircraft direction lines** → lines disappear, triangles stay.
      Re-tick → they come back.
- [ ] Range presets still cycle on a short BOOT tap, and the range label matches.
- [ ] Hold BOOT 3 s → credentials, location, units, text size and overlay flags
      all reset, device reboots into setup.
- [ ] Untick **Show flight trails** → tails disappear, symbols stay. Re-tick and
      wait two fetch intervals → tails grow again and follow the aircraft.
- [ ] Untick **Separate symbols…** → every aircraft is the original triangle.
      Re-tick → helicopters show as rotor discs, wide-bodies as large triangles.
- [ ] Set **Alert flash seconds** to 3, then wait for a military or wide-body
      contact → the ring pulses for about three seconds and the radar returns
      to normal. The same aircraft must not fire again on the next sweep.
- [ ] Set it to 0 → no flash at all.
- [ ] An airline flight shows its operator's name on the top tag line; a
      registration or hex id still shows verbatim.
- [ ] Serial log reports the CPU clock at boot (160 MHz) and a chip
      temperature once a minute.
- [ ] The **Settings** page renders every group down to the **Save** button,
      and Save reports success. The **System** page offers no Update button and
      no list of pages the firmware refuses to serve.
- [ ] Raise **Wi-Fi transmit power** to 19 → the log confirms it and the device
      stays connected. Drop it back to 8 if the board browns out or reboots.
- [ ] Pull the plug on the router for ~10 s → the radar picture stays up and
      recovers by itself. Only an outage past ~25 s brings up the connecting
      animation.
- [ ] Leave it running ~15 minutes → aircraft keep updating, no reboot loop
      (watch the serial log at 115200 baud for stack traces).

Record the outcome in the pull request. A stage-5 failure means the change is
not ready, however green CI is.
