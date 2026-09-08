# Plane Radar

<img width="800" height="450" alt="plane-radar" src="https://github.com/user-attachments/assets/716d0992-dab8-47ba-8f1a-2aec7f607419" />

**3D printed case (STL + assembly):** [MakerWorld](https://makerworld.com/en/models/2872376-esp32-plane-radar-live-ads-b-on-a-round-display#profileId-3207083) · **Firmware:** [Releases](https://github.com/MatixYo/ESP32-Plane-Radar/releases)

Firmware for an **ESP32-C3 Super Mini** and a **1.28″ round GC9A01** display (240×240). Shows a circular **ADS-B radar** around your configured location, with **WiFiManager** for first-time setup.

## What it does

1. **Wi‑Fi setup** (if needed) — captive portal on AP **`PlaneRadar-Setup`**
2. **Radar** — live aircraft from [adsb.fi](https://opendata.adsb.fi/) on a sonar-style grid

After Wi‑Fi is saved, the device reconnects automatically; the radar runs in the main loop with periodic ADS-B updates (~5 s).

## Controls (BOOT, GPIO 9, active LOW)

| Action | Effect |
|--------|--------|
| **Short tap** | Cycle range preset (5 → 10 → 15 → 25 km); saved to flash |
| **Hold 3 s** | Clear Wi‑Fi, location, and units; reboot into setup portal |

During setup you can also hold BOOT at power-on to force a credential reset (same as the long press).

## Wi‑Fi setup portal

**First-time setup** (no saved Wi‑Fi):

1. Connect to **`PlaneRadar-Setup`** — the AP is WPA2-protected; its password is shown on the yellow setup screen (8 characters, derived from this device's MAC, so it never changes)
2. Open **`http://plane-radar.local`** (preferred) or **`http://192.168.4.1`** — both are shown on the setup screen; captive portal may open automatically
3. Set home Wi‑Fi, then save

**Reconfigure anytime** (after the device is on your network):

1. Open **`http://plane-radar.local`** or **`http://<device-ip>`** (e.g. from your router or serial log at boot)
2. Change Wi‑Fi, location, units, or runway overlay; save

The same portal runs on the setup AP and on the device’s LAN IP while connected to Wi‑Fi. mDNS hostname is `plane-radar` → **plane-radar.local** (`kPortalHostname` in `config.h`). Some clients resolve `.local` slowly; use the IP if needed.

**Custom fields** (stored in NVS):

| Field | Purpose |
|-------|---------|
| **Latitude / Longitude** | Radar center and ADS-B query position (defaults in `config.h` until set) |
| **Display distances in miles** | Ring scale label in **mi** instead of **km** (e.g. `6mi` vs `10km`) |
| **Show airport runways** | Major-airport runway overlay on the radar (off to hide) |
| **Show aircraft direction lines** | Track/speed vector ahead of each aircraft symbol (off to hide) |
| **Text size** | `1` normal, `2` small, `3` smallest — picks a smaller embedded font for every label |

After a reset, the device reboots and shows the setup screen immediately (no “Connecting” loop on stale credentials).

## Radar display

### Grid

- Dark blue background, subdued green rings and crosshairs
- White **N / S / E / W** at the bezel; range label on the **east** spoke (ring 3 = ¾ of outer radius)
- White center dot

Layout and colors: `include/ui/radar_theme.h`.

### Range presets

| Ring 3 label | Outer radius (aircraft scale) |
|------------|-------------------------------|
| 5 km / 3 mi | ~6.7 km |
| 10 km / 6 mi | ~13.3 km (default) |
| 15 km / 9 mi | ~20 km |
| 25 km / 16 mi | ~33.3 km |

Preset, units, overlays and text size persist across reboot (`planeradar` NVS namespace).

### Text size

Three steps, set in the Wi‑Fi setup portal, applied without a reboot.

LovyanGFX draws a VLW glyph by mapping each source pixel to a `size_x` × `size_y`
rectangle, so a font shown below its native size loses whole rows and columns of
its anti-aliasing. Instead of scaling one font down, the firmware embeds one file
per pixel size (15 / 13 / 11 / 9) and each label picks the file matching its
target height, always drawn at scale 1.0. Sizes per role and step:
`kFontStepFonts` in `include/ui/radar_theme.h`.

Rebuild the font files (needs `pip install freetype-py`):

```bash
python3 scripts/build_ui_fonts.py
```

### Runways

- Major airports from OurAirports (`large_airport`); all open runway strips in range (helipads excluded)
- Teal runway lines with one ICAO label per airport (e.g. `KJFK`); toggle in the Wi‑Fi setup portal
- Update the embedded list: `python3 scripts/build_large_airports.py`

### Aircraft

- **Inside the outer ring** — red heading triangle, magenta speed vector (clipped at the ring; toggle in the portal), callsign / type / altitude tags
- **Outside the ring** (still within ADS-B fetch) — small **red dot on the screen rim** at the correct bearing (direction cue; not distance-accurate past the ring)
- **Tags** — placed toward the **center**: west (left) → tag on the **right** of the symbol; east (right) → tag on the **left**

As range decreases (or aircraft approach), targets move inward; beyond-ring dots become full symbols when they cross the outer ring.

### ADS-B

- Source: `https://opendata.adsb.fi/api/v3/`
- Fetch radius: `ui::radar::fetchRadiusKm()` — scales with the active preset to roughly the screen edge (so rim dots have data)
- Poll interval: `kAdsbFetchIntervalMs` (5 s) in `config.h`
- Ground aircraft hidden by default (`kAdsbShowGroundAircraft`)

## Security

The device sits on a home network and its settings page has no login, so the
aim is to keep the reachable surface as small as the feature set allows.

| Measure | Why |
|---------|-----|
| Setup AP is WPA2-protected | An open AP lets anyone in radio range hand the radar a network of their choice. Password is MAC-derived, stable, and printed on the setup screen. |
| `/update`, `/u`, `/erase`, `/restart` return 404 | WiFiManager registers an **unauthenticated OTA upload** plus credential-wipe and reboot endpoints on every portal, and links the OTA form from its default menu. Nothing here needs them: the partition table has no second OTA slot, and a credential reset is a 3 s BOOT hold on the device. The web-server callback runs before the library registers its routes, and ESP32's `WebServer` dispatches to the first match, so claiming the URIs there shadows them for good. |
| No OTA partition | `partitions/plane_radar.csv` has a single app slot. Firmware changes require physical USB access. |
| Response size cap (48 KB) | The C3 has 320 KB of RAM and the radar holds a 115 KB frame sprite. An oversized or endless response body would otherwise exhaust the heap and reboot the device. `Content-Length` is never trusted for the reservation. |
| Strict portal input parsing | Coordinates, checkboxes and the text-size step are validated in `include/util/`; a malformed field leaves the stored setting untouched. Covered by host tests (stage 2 of the [quality pipeline](docs/QUALITY.md)). |

**Known gap:** the ADS-B fetch uses `client.setInsecure()` — the server's
certificate is not validated. A network attacker can therefore read the
configured coordinates out of the request URL and feed fabricated aircraft to
the display. It cannot reach further: the JSON parser is bounded, every string
copy is length-checked, and the response is capped. Fixing it means pinning
adsb.fi's root CA, which trades this exposure for a device that stops fetching
whenever that CA rotates.

## Quality pipeline

Five stages, cheapest first — compiler warnings, host unit tests, static
analysis, firmware build, and a hardware smoke-test checklist. See
[docs/QUALITY.md](docs/QUALITY.md). Run the host tests with `pio test -e native`.

## Configuration

Edit **`include/config.h`** for hardware and behavior:

| Area | Keys / notes |
|------|----------------|
| Portal | `kPortalApName`, `kPortalIp`, `kPortalHostname` / `kPortalHostUrl` (mDNS; needs `-DWM_MDNS` in `platformio.ini`) |
| Wi‑Fi timing | connect attempts, reconnect grace, portal timeout (`0` = no timeout) |
| BOOT | `kBootPin`, `kBootResetHoldMs`, `kBootTapMinMs` |
| Display SPI | pins, `kDisplayInvert`, `kDisplayRgbOrder`, `kDisplaySpiWriteHz` |
| Default location | `kDefaultRadarLat`, `kDefaultRadarLon` (until portal overrides) |
| ADS-B | `kAdsbFetchIntervalMs`, `kAdsbShowGroundAircraft` |

Range presets: `include/ui/radar_range.h` (`kRangePresets`).

## Project layout

```
include/
  config.h
  util/                    — Arduino-free helpers (host-testable)
  hardware/
    lgfx_config.hpp
    display.h
    display_font.h
  data/
    large_airports.h
  ui/
    radar_theme.h
    radar_range.h
    radar_display.h
    runway_overlay.h
    status_screens.h
  services/
    wifi_setup.h
    radar_location.h
    adsb_client.h
data/
  ui_font_15.vlw           — embedded smooth UI fonts (Noto Sans SemiBold),
  ui_font_13.vlw             one per native pixel size
  ui_font_11.vlw
  ui_font_9.vlw
scripts/
  build_large_airports.py
  build_ui_fonts.py
test/
  test_util/               — host unit tests (pio test -e native)
docs/
  QUALITY.md
src/
  main.cpp
  data/
    large_airports_data.cpp
  hardware/
  ui/
  services/
```

## Wiring (GC9A01 ↔ ESP32-C3 Super Mini)

| Display | ESP32-C3 |
|---------|----------|
| VCC | 3V3 |
| GND | GND |
| RST | GPIO **0** |
| CS | GPIO **1** |
| DC | GPIO **10** |
| SDA (MOSI) | GPIO **3** |
| SCL (SCLK) | GPIO **4** |
| BOOT (user) | GPIO **9** |

## Build

```bash
pio run -t upload
pio device monitor
```

- PlatformIO env: **`supermini`**
- Serial: **115200** baud
- USB CDC on boot enabled in `platformio.ini` for the Super Mini

### Web-flashable release image

Single `.bin` for [esptool-js](https://espressif.github.io/esptool-js/) and similar tools (ESP32-C3, 4 MB, flash at **0x0**):

```bash
chmod +x scripts/merge-firmware.sh   # once
./scripts/merge-firmware.sh
```

Writes `release/plane-radar-merged.bin`. Skip rebuild if firmware is already built:

```bash
./scripts/merge-firmware.sh --no-build
```

Or via PlatformIO only (output: `.pio/build/supermini/firmware-merged.bin`):

```bash
pio run -e supermini
pio run -t merge -e supermini
```

Put the board in download mode (hold **BOOT**, tap **RESET**), then flash with Chrome/Edge over USB.

### CI and releases (GitHub Actions)

| Workflow | When | Output |
|----------|------|--------|
| [Build](.github/workflows/build.yml) | Push / PR to `main` | Artifact `plane-radar-supermini` (merged + split `.bin` files, ~90 days) |
| [Release](.github/workflows/release.yml) | Git tag `v*` (e.g. `v1.0.0`) | GitHub Release asset `plane-radar-v1.0.0.bin` + `.sha256` |

To ship a version users can download:

```bash
git tag v1.0.0
git push origin v1.0.0
```

The release workflow builds firmware in CI and attaches the merged image to the release. Download from **Releases** on GitHub, then flash at **0x0** (ESP32-C3, 4 MB).

## Dependencies

- [LovyanGFX](https://github.com/lovyan03/LovyanGFX)
- [WiFiManager](https://github.com/tzapu/WiFiManager)
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson)
