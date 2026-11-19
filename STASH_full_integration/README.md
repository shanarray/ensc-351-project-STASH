# STASH — integrated study timer (BeagleY-AI)

This package is based on the previously working STASH timer/LCD/keypad/buttons/LEDs project and the individually hardware-tested music and motion source. The two standalone examples were integrated into the **one** `app/src/main.c` program. The WAV file is already on the BeagleY-AI and is not copied into this project.

## Wiring: final agreed changes

| Function | GPIO / interface |
|---|---|
| Red LED | GPIO14 |
| Yellow LED | GPIO7 |
| Green LED | GPIO25 |
| Rotary A (CLK) | GPIO22: `/dev/gpiochip1`, offset 41 |
| Rotary B (DT) | GPIO17: `/dev/gpiochip2`, offset 8 |
| Rotary push (SW) | GPIO15: `/dev/gpiochip2`, offset 13 |
| Keypad **row 2** (formerly GPIO17) | **GPIO12**; runtime searches for GPIO12 by name |
| Keypad other lines | Unchanged from working base |
| **One shared buzzer** (timer + motion) | GPIO13 |
| Motion sensor | GPIO20 |
| LCD | I²C `/dev/i2c-1`, 0x27 |
| Joystick axis / physical CH1–CH4 buttons | MCP3208 `/dev/spidev0.0` |
| Joystick selection button | `/dev/gpiochip1` offset 33 |
| Speaker | ALSA `plughw:CARD=MAX98357A,DEV=0` |

**Wiring requirement:** Disconnect the old second/motion buzzer **and its driver circuit** from GPIO12. Move the **keypad row wire formerly on GPIO17** onto GPIO12. Keep the original timer buzzer on GPIO13 and the encoder on GPIO22/17/15. Do not wire two outputs together. Because GPIO line numbering depends on board setup, check `gpiofind GPIO12`, `gpiofind GPIO7`, `gpiofind GPIO14`, `gpiofind GPIO25`, and `gpioinfo` on the actual board.

## Behaviour

- Existing study/break presets, custom keypad entry, joystick selection, LCD countdown, ADC buttons and timer LEDs are retained.
- The single GPIO13 **active buzzer** handles the original timer-completion alert and **three short on/off beeps** for motion inactivity. The passive buzzer's 2 kHz generated tone is not used; it is not equivalent to GPIO13's active-buzzer wiring.
- Motion inactivity is detected after 30 seconds (samples every 100 ms); subsequent movement rearms it. Buzzer pattern does not block the timer.
- Music loops from the existing WAV. Rotary A/B adjusts volume in 5% increments; rotary switch pauses/resumes music, debounced at 250 ms. Encoder is **enabled by default**.
- Missing motion or audio hardware prints an error and lets the timer continue. Encoder controls are unavailable if its GPIO request fails.

## Build

Build on the BeagleY-AI or in the correctly configured aarch64 Linux toolchain with compatible `libgpiod` **v2**, ALSA development headers (`libasound2-dev`), and CMake:

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

The compiled application is `build/app/study_session`. Stop all earlier GPIO demo processes before running, so the keypad and encoder can each request their own GPIO lines.

The music code uses `/mnt/remote/home/debian/music/study_music.wav` by default. If the WAV file is somewhere else on the BeagleY-AI, run with `sudo env STASH_WAV_FILE=/actual/path/study_music.wav ./build/app/study_session`.

You can deploy using the same NFS process that previously worked for the STASH project. If building on a host x86 machine, the binary is **not** suitable for running on the ARM board until cross-compiled correctly.

## Verification status

The existing timer regression test passed locally with `cc -std=c11 -Wall -Wextra -Wpedantic -Werror` on November 19, 2026. The package structure and source pin assignments were checked, but the combined application **could not be compiled against real libgpiod and ALSA development headers in this environment**, nor run on the physical BeagleY-AI. It must still be built and hardware-tested by the team. The base code was previously reported hardware tested; the merged GPIO wiring is newly specified and unverified.

## Test order

1. With board powered off, confirm GPIO12 is connected to keypad row 2, no buzzer circuit remains on GPIO12, and LEDs are now GPIO14/7/25.
2. Stop older test processes. Check `gpiofind GPIO12`, `gpiofind GPIO17`, `gpiofind GPIO22`, `gpiofind GPIO15`, `gpiofind GPIO7`, `gpiofind GPIO14`, `gpiofind GPIO25`.
3. Run the integrated program and make sure no GPIO line-request error occurs; check LCD menu, keypad all 16 keys, joystick and ADC buttons.
4. Verify red/yellow/green LED meanings during study/break/pause and timer completion beep through GPIO13.
5. Verify music speaker output, rotary volume, rotary push pause/resume.
6. Leave motion sensor inactive for 30 seconds: hear three short beeps from the **same** GPIO13 buzzer. Verify the timer and audio continue running.
7. Check shutdown/cleanup, then commit to Git after resolving any board-specific GPIO mappings.
