# Touchscreen and shot journal

The 480 × 480 T-RGB display communicates with the controller over ESP-NOW (or
USB serial for development). The display makes HTTP calls to the journal's
`/api/v1` JSON API; the controller never waits for WiFi or HTTP to brew.

## Configure and build

1. Start the server using [its README](../server/README.md).
2. Configure the existing display/controller WiFi secrets for the same LAN/AP.
3. Copy the journal example and edit its server address:

   ```sh
   cp src/ui/src/journal_config.example.h src/ui/src/journal_config.h
   ```

   `journal_config.h` is gitignored. Set `ESPRESSO_API_URL` to e.g.
   `"http://192.168.1.100:8000"`, **without** `/api/v1`. A stable LAN IP is
   recommended. Without a configured HTTP URL, journal uploads are disabled;
   temperature/brew/steam controls still work. This is trusted-LAN HTTP, not TLS
   or authenticated access.

4. Build and flash **both** the controller and display. For example:

   ```sh
   pio run -d src/firmware -e devkit
   pio run -d src/ui -e T-RGB
   # Add "-t upload" with the correct connected board/port when ready.
   ```

   Choose the controller environment matching your board (`release`, `devkit`,
   or `t7s3`). No hardware is automatically flashed by the tests.

The new journal messages require matching controller/display firmware. Existing
state/command packet layouts are unchanged, but an old controller cannot send
journal records.

## User flow

- Add a **bag** on the website, then tap the coffee/grind strip on Main.
- Tap the selected coffee to open the active-bag picker. Use Refresh to load new
  bags; lists are paginated, with roaster and roast date to distinguish purchases.
- Select a bag and enter a non-negative decimal grind setting using the numeric
  keyboard. A different bag clears the draft grind—settings are not silently
  copied between bags. Blank grind means unknown. Save or Cancel.
- The server retains the setup between boots. While idle, the display refreshes
  it every five seconds, including changes made on the website. Wait for
  **Setup ready**: a successful HTTP save alone does not mean the controller has
  acknowledged that revision. With a disconnected controller, the strip remains
  in the syncing state.
- Brew normally. Coffee/setup revision, recipe stage durations, and setpoint are
  captured **by the controller at start**, not when the upload eventually runs.
  The configured stage durations remain frozen for that sequence.
- Only sequences reaching the main BREW stage create shots. Stopping in
  preinfusion/bloom does not create a record. Every main-stage sequence is logged,
  including short ones; there is no duration-based flush heuristic.
- At the end, a result screen shows **actual main brew time** and a blank
  **actual yield (g)** field. Enter a positive decimal measured yield and Save,
  or Skip to leave it unknown. Target yield is never prefilled as a measurement.
  The numeric keyboard's OK/checkmark returns to the result buttons.
- The shot uploads immediately in the background. Save yield updates that same
  shot through the API. A failed save retains the entered yield while the result
  stays open; retry Save yield. **Retry log** retries the shot without setting a
  yield. Skip dismisses the result without clearing any saved measurement.
- The result has a BREW button; another brew dismisses the previous result and
  does not wait for uploads. Unsaved input is discarded when its result closes.
- Dose, target yield, ratings, notes, bag creation/editing/archiving remain on
  the website in this first version. Coffee/grind metadata never changes the
  heater setpoint or machine brew timer.

## Timing

`duration_s` and the machine's finished-shot time now mean **actual time in the
main BREW stage, including HOLD**, excluding preinfusion and bloom. The controller
measures stage entry/exit rather than subtracting configured durations. Actual
short/extended shots can differ from the recipe target. Stage configuration is
still recorded separately as recipe context. No trustworthy wall clock is
required: `started_at` is omitted and the server supplies receipt time.

This intentionally changes the previous total-sequence interpretation. There
were no existing shot records to preserve; no database schema migration is needed.

## Reliability and implementation

- `journal_ui.cpp` is a handwritten LVGL extension: it adds the Main strip and
  transient editor/picker/result overlays at runtime. It does **not** edit
  generated EEZ files in `src/ui/`. These extensions do not appear in the EEZ
  editor preview; rebuilding the EEZ project does not overwrite them.
- `journal_client.cpp` owns a low-priority HTTP worker on core 0, with finite
  connect/read timeouts and bounded volatile work/result queues. Network work
  never runs in the controller loop, transport receive callbacks, or LVGL task.
  Only the main display loop touches widgets. Controller WiFi startup also joins
  in the background; OTA waits for an IP rather than blocking startup.
- API/server failure does not stop brewing. Losing the WiFi AP can also disturb
  ESP-NOW channel alignment and disconnect the wireless touchscreen: an already
  running sequence still finishes under controller control, but this is not a
  guarantee of wireless control with the AP unavailable.
- The controller repeats its current snapshot/result and setup acknowledgement
  over the existing transport. Only the latest sequence is retained in RAM;
  there is **no flash queue or replay history**. Records can be lost when the
  display/server is unavailable, another brew starts, or a device reboots.
- Retries reuse a stable shot UUID (random controller boot identity + sequence)
  and the exact original creation payload, then PATCH the measured yield.
  A timed-out POST may already have committed; retries cannot create duplicates
  or conflict just because yield was entered afterward.
- The display rejects duplicate/out-of-order shot phases and ignores HTTP
  responses belonging to an older result. A dropped idle-state packet delays
  the yield prompt until the next state, rather than losing the prompt.
- Starting before any setup revision reaches the controller still brews, but that
  record cannot be uploaded. The result explicitly reports missing setup at
  start; a later revision is never attached retroactively.
- Setup revisions are monotonically increasing. After deliberately resetting
  the server database, restart the controller and display as well.

## Checks without hardware

```sh
sh src/shared/tests/run.sh
python3 -m unittest discover -s src/ui/tools -p 'test_*.py' -v
cd src/server
uv run --frozen pytest -q
```

Native tests compile the actual controller/timer sources with lightweight Arduino
and PID stubs. They cover preinfusion/bloom cancellation, actual main entry,
manual STOP, HOLD, failsafe, millis rollover, safety shutdown, immutable snapshots,
and repeated/out-of-order packets. Server tests cover the upload/yield/retry
contract. These do not replace a touch/layout and network-loss test on hardware.

The real LVGL editor/picker/result logic can also run headlessly on your computer
(with CMake, a C++ compiler, and the dependencies installed by the T-RGB build):

```sh
cmake -S src/ui/tests -B /tmp/journal-ui-tests
cmake --build /tmp/journal-ui-tests -j 4
ctest --test-dir /tmp/journal-ui-tests --output-on-failure
# Optional: write editor/picker/keyboard/result PPM screenshots to /tmp:
/tmp/journal-ui-tests/journal_ui_test /tmp
```

These tests use production widgets/UI logic with mocked HTTP-job results and
transport. They verify blank actual yield, retained input on failure, identical
retry payloads, setup acknowledgement, dropped idle packets, and rejection of
stale responses. They do not exercise the ESP32 HTTP stack or physical touch.

For a real display with a simulated controller:

```sh
pio run -d src/ui -e T-RGB-serial -t upload
cd src/ui
uv run tools/fake_controller.py /dev/cu.usbmodemXXXX --rate 4
```

The fake emits state, setup acknowledgements, and journal snapshots. It cannot
verify physical pump timing, ESP-NOW reliability, or electrical safety.
