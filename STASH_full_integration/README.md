# STASH — combined BeagleY-AI build

This branch combines the supplied *working* MCP3208 buttons + timer/LED/buzzer
modules with the I2C LCD, CH0-only joystick, 4x4 matrix keypad, menu, and
session functions. It intentionally does not compile older placeholder GPIO
modules, unfinished timer `main()` examples, or standalone keypad test programs.

## Hardware signals used

| Component | Signal / configuration |
|---|---|
| MCP3208 ADC | `/dev/spidev0.0`, SPI mode 0, 250kHz, 12-bit |
| Joystick up/down | ADC CH0, high >3200 UP; low <800 DOWN |
| Study button | ADC CH1, LOW when pressed |
| Break button | ADC CH2, LOW when pressed |
| Pause/resume | ADC CH3, LOW when pressed |
| Reset / hold 2s power | ADC CH4, LOW when pressed |
| LEDs red / yellow / green | Named GPIO14 / GPIO15 / GPIO18 via libgpiod 2.x |
| Buzzer | Named GPIO13 via libgpiod 2.x |
| LCD | `/dev/i2c-1`, I2C address 0x27 |
| Joystick press SW | `/dev/gpiochip1` offset 33, active low |
| Keypad rows | chip1:38, chip2:8, chip0:7, chip0:10 |
| Keypad columns | chip2:15, chip2:17, chip2:7, chip1:36 |

**Before energizing**: double-check shared ground, power voltage, physical
chip orientation and GPIO conflicts on the actual board. MCP3208 SPI logic
must be compatible with 3.3V BeagleY-AI I/O. Do not attach a 5V output to
BeagleY-AI SPI/GPIO directly. Pins/offsets are carried over from user files,
not independently verified against physical wiring.

## Build on BeagleY-AI

The target needs `cmake`, a C compiler, **libgpiod 2.x** runtime and
`libgpiod-dev` (development headers and linker library).

```sh
cmake -S . -B build
cmake --build build -j2
./build/app/study_session
```

The `study_session` executable is built **on the BeagleY-AI**, not included
in the ZIP. It should be run while no other STASH, `adc_probe`, or individual
GPIO/keypad test programs are holding the same devices.

### Controls and UI

- Joystick **UP/DOWN** navigates seven menu options (one menu step per movement,
  re-center between steps); its push **SW** selects.
- Presets: Pomodoro 25/5, Study 60, 90, 120, 30, 180, or Custom Session.
- Custom: enter 1–999 minutes on the 4x4 keypad, `#` confirms, `*` deletes
  entry or backs out from an empty entry. `*` also backs out at confirmation.
- Joystick **SW** confirms and starts; LCD counts down, with labels.
- Pomodoro performs four study periods, three short breaks and one 15-minute
  long break; other presets perform one study+break pair. This build then
  returns to the menu.
- Physical Study (CH1) and Break (CH2) start standalone timers (currently
  **1 minute**, as set in `app/include/input_handler.h`).
- Physical Pause (CH3) pauses/resumes; Reset (CH4) short release resets;
  holding CH4 for 2 seconds toggles OFF/ON. OFF blanks the LCD.
- Green LED = study, red = break, yellow = paused. Buzzer sounds for 250ms
  when timer reaches zero naturally.
- During a selected session, the ADC buttons are still scanned. Reset/power
  exits the running session. Study/Break buttons can take over its timer.
  **Manual takeover of the same phase is not distinguished** from the
  existing phase; test this case before using it as a production feature.
- `Ctrl+C` exits from the main menu or running phase; custom keypad entry
  currently uses blocking reads and should be completed or cancelled first.

## Test sequence

1. With both branches' original wiring reconciled and board off, check all
   component connections. Confirm no reused GPIO line between devices.
2. Stop previous programs; build as above and resolve any real board errors.
3. On start: LCD shows a menu. Test joystick UP/DOWN and SW selection.
4. Select a short custom session using keypad digits + `#` and confirm with SW.
5. Watch LCD countdown and green LED. Pause/resume with CH3; yellow LED.
6. Allow study phase to expire naturally; verify buzzer and red LED on break.
7. Press CH4 to reset and return to menu. Check 2-second power hold and wake.
8. From menu press CH1, CH2 to start manual timers; verify LED, LCD, buzzer.
9. Verify Ctrl+C cleanup, then repeat a full Pomodoro cycle with shortened
   durations only after the quick functionality tests pass.

## Verification and limitations

- CMake successfully configured and **all source files built and linked** on
  the development host against a **temporary libgpiod API stub**. This checks
  integration, symbol linkage, compiler warnings, and build layout; the stub
  is **not packaged** and provides **no hardware control**.
- Timer state regression test passed on the host (start/pause/resume,
  OFF/ON/reset and expiry). Run it on the board using `ctest --test-dir build`.
- No real BeagleY-AI cross-compiler or physical GPIO/SPI/I2C hardware was
  available during this packaging. Accordingly the build is **not** claimed
  to have passed on the actual BeagleY-AI.
- Before accepting hardware integration, verify GPIO line ownership and
  shared SPI channel assignments. `GPIO13` buzzer is not the same as keypad's
  chip-specific offset 13; compare actual chip+offset mappings on the board.
- The old `study_helpers.c` and rotary encoder and placeholder GPIO HAL are
  intentionally **not compiled**: they duplicate inputs, outputs and timer
  ownership or contain unfinished drivers. They can be integrated later if
  the project's requirements call for them.
