# Plane Radar — extended firmware

<img width="800" height="450" alt="plane-radar" src="https://github.com/user-attachments/assets/716d0992-dab8-47ba-8f1a-2aec7f607419" />

A live aircraft radar on a 1.28″ round display. Point it at your home
coordinates and it draws the planes actually flying overhead, updated every few
seconds, on a sonar-style grid.

> **This is a fork** of [MatixYo/ESP32-Plane-Radar](https://github.com/MatixYo/ESP32-Plane-Radar).
> The hardware, the case and the original radar are theirs. What this fork adds
> is listed below — it was designed and written with
> [Claude Code](https://claude.com/claude-code) (Anthropic) and verified on real
> hardware before each change was kept.

---

## What this fork adds

| | Change | Why it matters |
|---|---|---|
| 🔤 | **Sharp text at three sizes** | The original scaled one font down, which drops pixel rows and makes small labels ragged. Now four fonts are built into the firmware, one per size, so every label is drawn at its native size. Pick normal / small / smallest in the settings page. |
| ✈️ | **Airline names instead of codes** | `DLH4AB` now reads `Lufthansa`. ~5 800 airlines are built in. Registrations and hex ids are left alone. |
| 🚁 | **Symbols per aircraft class** | Helicopters get a rotor disc, wide-bodies a larger triangle. Only two outlines survive rotation at this size, so weight is carried by size instead of by shape. Switchable. |
| 〰️ | **Flight trails** | Thin grey tail through each aircraft's last positions — roughly 20 seconds of history. Switchable. |
| 🚨 | **Alert flash** | The radar pulses a coloured ring when an emergency squawk, a military aircraft or a rare type (A380, 747, An-124…) first appears. 3, 5 or 7 seconds, or off. |
| 🔴 | **Direction lines can be switched off** | The line ahead of each aircraft is now optional. |
| 🔒 | **Three security holes closed** | An **unauthenticated firmware-upload page** was reachable from the whole local network, along with remote credential wipe and reboot. The setup Wi-Fi was **open**. A malicious server could **crash the device on demand** with an oversized reply. See [Security](#security). |
| 🌡️ | **Power saving and radio power** | Lower CPU clock (on by default) and a choice of Wi-Fi transmit power, for the trade-off between heat, range and stability. |
| 📶 | **Rides out short Wi-Fi drops** | A brief hiccup no longer replaces the radar with the search screen. |
| ✅ | **A five-stage quality pipeline** | Compiler warnings, host unit tests, static analysis, firmware build and a hardware checklist — all in CI. It has already caught real defects. See [docs/QUALITY.md](docs/QUALITY.md). |

Everything switchable lives in the device's own settings page — no reflashing,
no recompiling, and the changes take effect immediately.

---

## Getting started

**You need:** an ESP32-C3 Super Mini, a 1.28″ round GC9A01 display, a USB-C
cable, and a Wi-Fi network. Wiring is in [Wiring](#wiring) below; the 3D case is
on [MakerWorld](https://makerworld.com/en/models/2872376-esp32-plane-radar-live-ads-b-on-a-round-display#profileId-3207083).

### 1. Put the firmware on the board

Download `firmware-merged.bin` from the newest successful
[Build](../../actions/workflows/build.yml) (open the run, then **Artifacts**), or
from [Releases](../../releases) if one is published.

Open [esptool-js](https://espressif.github.io/esptool-js/) in **Chrome or Edge**
(Firefox and Safari cannot talk to USB devices). Put the board into download
mode — hold **BOOT**, tap **RESET**, release **BOOT** — then **Connect**, choose
`firmware-merged.bin`, set the address to **`0x0`**, and **Program**. Tap
**RESET** when it finishes.

### 2. Tell it your Wi-Fi and where you live

The screen turns yellow and shows a network name, a **password** and a web
address.

1. Join the Wi-Fi network **`PlaneRadar-Setup`** using the password on screen.
   It is eight characters, derived from your board, and never changes.
2. Open **`http://plane-radar.local`** (or `http://192.168.4.1`).
3. Enter your home Wi-Fi, then your latitude and longitude, and save.

The radar appears within a few seconds.

> Flashing wipes the stored settings, so you will do this again after every
> firmware update. That is a property of the single-file flash image, not a bug.

### 3. Change anything later

Once it is on your network, open **`http://plane-radar.local`** from any device
in the house. The same page carries every setting.

**One tap on BOOT** cycles the range (5 → 10 → 15 → 25 km).
**Holding BOOT for 3 seconds** erases Wi-Fi, location and all settings and
returns to the setup screen.

---

## Settings reference

Everything below is on the setup page and is remembered across reboots.

| Setting | What it does |
|---------|--------------|
| **Latitude / Longitude** | Where the radar is centred, and the position it asks the ADS-B service about |
| **Display distances in miles** | Ring labels in `mi` instead of `km` |
| **Show airport runways** | Runway overlay for major airports |
| **Show aircraft direction lines** | The line ahead of each aircraft showing heading and speed |
| **Show flight trails** | Thin grey tail through recent positions |
| **Separate symbols for helicopters and heavies** | Per-class silhouettes instead of one triangle |
| **Alert flash seconds** | `0` off, or `3` / `5` / `7` — how long the radar pulses for a noteworthy aircraft |
| **Text size** | `1` normal, `2` small, `3` smallest |
| **Wi-Fi transmit power dBm** | `8` low (default), `13` medium, `19` high — see [Heat and stability](#heat-and-stability) |
| **Power saving** | Lower CPU clock. On by default; takes effect at the next restart |

---

## How the radar reads

### The picture

Dark blue background, green rings and crosshairs, white **N / S / E / W** at the
edge. The label on the east spoke gives the distance of the third ring.
Aircraft further away than the outer ring show as a **dot on the rim**, in the
right direction but not at the right distance.

### Aircraft symbols

At 240 pixels only two outlines survive rotation, so weight class is carried by
size and shape only separates rotorcraft from fixed wing:

| Class | Symbol | Chosen when |
|-------|--------|-------------|
| Light | small triangle | emitter category `A1` |
| Jet / airliner | the original triangle | everything else, including unknown |
| Heavy | large triangle | category `A5`, or a wide-body type code (`A388`, `B744`, …) |
| Helicopter | rotor disc with blades | category `A7`, or a rotorcraft type code (`EC35`, `R44`, …) |

The feed is inconsistent — `category`, `squawk`, `emergency` and `dbFlags` are
all optional and most aircraft send none of them — so every rule falls back to
the jet triangle rather than guessing. Type tables are in
`src/util/aircraft_class.cpp` and match **exactly, never by prefix**: `B47` is a
Stratojet, `B47G` a Bell 47.

### Labels

Three lines beside each symbol: operator, type, altitude. The first line shows
the **airline name** when the callsign is an airline flight (`DLH4AB` →
`Lufthansa`) and the callsign as received otherwise, so a registration like
`D-EABC` is never reattributed to whichever airline shares its first three
letters. The flight number is not shown — the name replaces it.

### Alerts

When something noteworthy first appears, a coloured ring pulses outward from the
centre for the configured number of seconds. Each aircraft fires **once per
visit**: it has to leave and stay away ten minutes before it can interrupt
again.

| Trigger | Colour | Detected from |
|---------|--------|---------------|
| Emergency | red | squawk 7500 (hijack), 7600 (radio failure), 7700 (general), or a declared `emergency` |
| Military | amber | the feed's `dbFlags` military bit |
| Notable type | cyan | the list below |

**Notable types** (`kNotableTypes` in `src/util/aircraft_class.cpp`):

`A124` An-124 · `A225` An-225 · `A337` BelugaXL · `A388` **A380** ·
`A3ST` Beluga · `A400` A400M · `AN22` An-22 · `B52` B-52 · `B703` Boeing 707 ·
`B741`–`B748`, `B74D`, `B74R` **Boeing 747 family** · `B74S` Dreamlifter ·
`C5M` C-5 Galaxy · `CONC` Concorde · `IL76`, `IL86`, `IL96` Ilyushin

The list is deliberately short — a false alarm is more annoying than a miss.
Adding a type is one line in that file.

### Where the data comes from

[adsb.fi](https://opendata.adsb.fi/), polled every few seconds. The radius grows
with the selected range so rim dots have something to show. Aircraft on the
ground are hidden by default (`kAdsbShowGroundAircraft` in `include/config.h`).

---

## Heat and stability

The board runs warm — around 70 °C on the chip is normal for a radio that never
sleeps inside a closed printed case. It is well inside the part's rating, but
two settings let you trade:

**Power saving** (on by default) runs the core at 80 MHz instead of 160 MHz.
That halves the core's dynamic power and changes nothing you can see: the SPI
bus keeps its own clock, and the radar redraws only every few seconds. It
applies at the next restart.

**Wi-Fi transmit power** is capped at 8.5 dBm by default — inherited from the
original, and a sensible cap for the Super Mini's small regulator. If the
connection drops in a weak spot, raise it to 13 or 19 dBm. That costs current
and therefore heat, so change one thing at a time.

The chip temperature is written to the serial log every minute
(115200 baud), so you can measure the effect rather than guess it.

**Short Wi-Fi drops** no longer take the screen away: the radar stays up for the
first 25 seconds of an outage while the ESP32's own auto-reconnect does its
work. Only a longer outage brings up the connecting animation.

---

## Security

The device sits on a home network and its settings page has no login, so the
aim is to keep the reachable surface as small as the feature set allows.

| Measure | Why |
|---------|-----|
| `/update`, `/u`, `/erase`, `/restart` return 404 | WiFiManager registers an **unauthenticated firmware-upload page** plus credential-wipe and reboot endpoints on every portal, and links the upload form from its menu. Its own authentication hook is a no-op in 2.0.17. Nothing here needs those routes: there is no second firmware slot to write into, and a credential reset is a 3 s BOOT hold. The web-server callback runs before the library registers its routes and ESP32's `WebServer` uses the first match, so claiming the URIs there shadows them for good. |
| Setup AP is WPA2-protected | An open AP lets anyone in radio range hand the radar a network of their choice. The password is derived from the board's MAC, so it is stable and printable on the setup screen. |
| No over-the-air updates | A single app partition. Firmware changes need physical USB access. |
| Response size cap (48 KB) | The chip has 320 KB of RAM and the radar holds a 115 KB frame buffer. An oversized or endless reply would otherwise exhaust the heap and reboot the device on demand. The server's `Content-Length` is never trusted for the reservation. |
| Strict input parsing | Coordinates, checkboxes and every number field are validated in `include/util/`; a malformed field leaves the stored setting untouched. Covered by host tests. |

**Known gap:** the ADS-B request does not verify the server's certificate
(`client.setInsecure()`). Someone able to manipulate your network traffic can
read your configured coordinates out of the request and feed fabricated aircraft
to the display. It cannot reach further — the parser is bounded, every string
copy is length-checked, and the reply is capped. Fixing it means pinning
adsb.fi's root certificate, which trades this exposure for a device that stops
fetching whenever that certificate is rotated. Documented rather than decided
silently.

**Also open by design:** anyone already on your network can change the settings.
WiFiManager offers no working authentication, so the only real alternatives are
a time-limited settings page or none at all.

---

## Quality pipeline

Five stages, cheapest first — see [docs/QUALITY.md](docs/QUALITY.md) for the
detail and the hardware checklist.

1. **Compiler warnings** — `-Wall -Wextra`; CI fails on any warning in this
   project's own code.
2. **Host unit tests** — `pio test -e native`, 19 cases over the input parsers,
   the callsign parser, the aircraft classifier and the alert rules. They run on
   the build machine, so hostile and malformed inputs are exercised without a
   board.
3. **Static analysis** — cppcheck over `src/` and `include/`, failing on any
   finding.
4. **Firmware build** — plus the web-flashable image, published as an artifact.
5. **Hardware smoke test** — a written checklist for what no machine can see.

It has already earned its keep: stage 1 found dead code carrying the wrong
safety margin, stage 3 found a portability bug in the trail store, and two
defects in the test harness itself surfaced — including assertions that silently
compared nothing.

---

## Wiring

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

---

## Building it yourself

```bash
pio run -e supermini          # build
pio run -t upload             # build and flash over USB
pio device monitor            # serial log, 115200 baud
pio test -e native            # host unit tests
```

Single-file image for a web flasher (ESP32-C3, 4 MB, flash at `0x0`):

```bash
pio run -e supermini && pio run -t merge -e supermini
# -> .pio/build/supermini/firmware-merged.bin
```

### Regenerating the built-in data

| Command | Produces | Source |
|---------|----------|--------|
| `python3 scripts/build_ui_fonts.py` | the four `.vlw` fonts | Noto Sans SemiBold (OFL) |
| `python3 scripts/build_airlines.py` | the airline table | OpenFlights, **ODbL** |
| `python3 scripts/build_large_airports.py` | the runway dataset | OurAirports |

Fonts need `pip install freetype-py`. CI rebuilds the fonts and fails if they
differ from what is committed. The airline table is not diffed, because it
tracks a live upstream dataset.

### Where things live

```
include/
  config.h                 — pins, timing, defaults
  util/                    — Arduino-free logic, covered by host tests
  hardware/                — display and font plumbing
  data/                    — generated tables (airports, airlines)
  ui/                      — radar drawing, settings, alerts, trails
  services/                — Wi-Fi portal, ADS-B client, location
src/                       — the matching implementations
data/                      — embedded font files
scripts/                   — dataset and font generators
test/test_util/            — host unit tests
docs/QUALITY.md            — the five-stage pipeline and hardware checklist
```

### CI

| Workflow | When | Output |
|----------|------|--------|
| [Build](.github/workflows/build.yml) | push, PR, manual | `plane-radar-supermini` artifact with the flashable images |
| [Quality](.github/workflows/quality.yml) | push, PR, manual | unit tests, static analysis, font regeneration check |
| [Release](.github/workflows/release.yml) | git tag `v*` | release asset `plane-radar-v1.0.0.bin` + checksum |

---

## Credits and licence

- Original project: **[MatixYo/ESP32-Plane-Radar](https://github.com/MatixYo/ESP32-Plane-Radar)** — MIT licence, retained in [LICENSE](LICENSE). The hardware design, the case and the radar this fork builds on are theirs.
- Aircraft data: [adsb.fi](https://opendata.adsb.fi/) · Airports: [OurAirports](https://ourairports.com/) · Airlines: [OpenFlights](https://openflights.org/data.html) (ODbL) · Font: Noto Sans (OFL)
- Libraries: [LovyanGFX](https://github.com/lovyan03/LovyanGFX), [WiFiManager](https://github.com/tzapu/WiFiManager), [ArduinoJson](https://github.com/bblanchon/ArduinoJson)
- The additions in this fork were designed and implemented with [Claude Code](https://claude.com/claude-code), then flashed and checked on hardware before being kept.
