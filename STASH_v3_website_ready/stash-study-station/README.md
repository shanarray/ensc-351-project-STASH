# STASH Study Station website

Adapted from the provided ENSC 351 Assignment 3 Beat-Box website structure. Read-only dashboard: it does not send commands to the timer or use additional GPIO pins.

## Run

On the BeagleY-AI, install Node.js if needed, then from this directory:

```sh
node server.js
```

Open `http://BEAGLEY_IP:8088` on a device on the same LAN. Find the board IP with `hostname -I`.

No `npm install` is necessary for this version. The website stays open even if the C timer is not yet exporting data; it will show "Waiting for timer data".

## Connect to the C program

The integrated C application must write `data/status.json` approximately once per second in this format:

```json
{
  "state": "STUDY",
  "remaining_seconds": 1420,
  "light": 320,
  "light_unit": "lux",
  "noise": 42,
  "noise_unit": "sensor units"
}
```

Use actual sensor values. Only use `dB` if the sound sensor is calibrated to report decibels. `light` and `noise` may be `null` if unavailable. Write to `status.json.tmp` and rename to `status.json` to avoid partial reads. The C program must use the absolute data directory path for this deployed server, not rely on its working directory.

After a completed session, append a row to `data/history.csv`:

```csv
date,mode,duration_seconds
2026-11-09 10:30:00,STUDY,1500
```

The Node server polls these files; no UDP code is needed for this initial read-only website. Your existing timer and ADC/joystick modules do not need to change to run the website independently. To display live data, the integrated application still needs the file-writing functionality.

## Test without hardware

For an isolated UI test, manually create `data/status.json` with the JSON example above. Refresh the page and check the timer and sensor readings. Edit the file and confirm the display changes within about one second. Delete the file to check the disconnected state.

## Files

- `server.js`: HTTP server and read-only status/history APIs
- `public/index.html`: dashboard layout
- `public/stylesheets/style.css`: appearance
- `public/javascripts/study_station_ui.js`: updates display
- `data/history.csv`: initially empty session log

## Notes

This is a starting point for the group project, not a complete integration of the C timer and sensor programs. Keep the server on a trusted local network; it is not hardened for public internet use.
