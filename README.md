# Samsung Physical Remote for Flipper Zero

A minimal Flipper Zero app that turns known-working Samsung TV IR codes into a physical-button replacement remote.

The goal is not to build a universal remote. The goal is to make the Flipper Zero behave like a simple physical Samsung TV remote, using known-working IR codes from a saved Flipper Infrared remote.

## v1.0 behavior

- Starts on a `Samsung Remote` home screen.
- `Power` sends the Samsung TV `POWER` infrared command.
- `Simulate Remote` enters a physical-button remote mode.
- The home screen stays in the normal Flipper orientation. Physical remote mode rotates the app view for holding the Flipper sideways with the IR transmitter pointed at the TV.
- In physical mode, the D-pad arrows still send the matching TV navigation directions in the orientation shown on screen.
- In physical mode, Up/Down released before 2000 ms send TV navigation. Holding Up/Down for at least 2000 ms starts volume repeat with `VOL+`/`VOL-` every 500 ms.
- `OK` sends `Select`, Back released before 3000 ms sends `Return`, and Back held for at least 3000 ms returns to the home screen while still held.

## Build and install

This is a Flipper Zero external application. It is built with `ufbt`, the standard tool for building Flipper apps outside the main firmware tree.

### Step 1: Install `ufbt`

You need Python 3 and `ufbt`.

```sh
python3 -m pip install --upgrade ufbt
```

### Step 2: Clone this repository

```sh
git clone https://github.com/broeneatsdinner/flipper-samsung-remote.git
```

### Step 3: Enter the repository directory

```sh
cd flipper-samsung-remote
```

### Step 4: Build the app

```sh
ufbt
```

After the build finishes, `ufbt` creates a `dist/` directory inside this repository.

The built Flipper app package will be inside that directory:

```text
dist/samsung_remote.fap
```

### Step 5: Connect your Flipper Zero

Connect the Flipper Zero to your computer over USB.

Make sure the Flipper has an SD card installed.

### Step 6: Install and launch the app

```sh
ufbt launch
```

This builds the app, copies it to the Flipper, and launches it.

### Step 7: Find the app on the Flipper later

After installation, the app should remain available on the Flipper even when it is not connected over USB.

Look for it under:

```text
Apps
  Infrared
    Samsung Physical Remote
```

When installed by `ufbt launch`, the app is copied to:

```text
/ext/apps/Infrared/samsung_remote.fap
```

### Manual install

If you do not use `ufbt launch`, you can manually copy the built `.fap` file from `dist/` to the Flipper's apps storage using qFlipper or another file manager.

The destination on the Flipper SD card is:

```text
/ext/apps/Infrared/
```

## Remove from your Flipper

This app installs as a single `.fap` file on the Flipper SD card.

To remove it, connect the Flipper over USB, open the SD card with qFlipper or another file manager, and delete:

```text
/ext/apps/Infrared/samsung_remote.fap
```

After deleting the file, disconnect or reboot the Flipper. The app should no longer appear under:

```text
Apps
  Infrared
    Samsung Physical Remote
```

## Test notes

1. Launch `Samsung Physical Remote`.
2. Confirm Up/Down toggles between `Power` and `Simulate Remote`.
3. Confirm OK on `Power` toggles the Samsung TV power state.
4. Enter `Simulate Remote` and confirm the physical remote screen is rotated while the home screen remains normal.
5. Hold the Flipper sideways with the IR transmitter facing the TV.
6. Confirm Up/Down/Left/Right send the matching TV navigation directions in the orientation shown on screen.
7. Confirm holding Up/Down for at least 2000 ms sends `VOL+`/`VOL-` repeatedly every 500 ms until release.
8. Confirm Left/Right do not trigger volume hold.
9. Confirm OK and a Back tap control TV Select and Return.
10. Confirm a Back tap does not exit the app in physical mode.
11. Confirm holding Back for at least 3000 ms returns to the home screen before release and does not send `Return`.
12. Confirm Back from the home screen exits the app.

## Status

v1.0.2 has been built with `ufbt`, launched on a physical Flipper Zero, and tested successfully against the target Samsung TV.
