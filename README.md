# Flipper Samsung Remote

A minimal Flipper Zero Samsung TV infrared remote app.

The goal is not to build a universal remote. The goal is to make the Flipper Zero behave like a simple physical Samsung TV remote, using known-working IR codes from a saved Flipper Infrared remote.

## v0 behavior

- Starts on a `Samsung Remote` home screen.
- `Power` sends the Samsung TV `POWER` infrared command.
- `Simulate Remote` enters a physical-button remote mode.
- In physical mode, arrows send TV navigation, `OK` sends `Select`, Back released before 3000 ms sends `Return`, and Back held for at least 3000 ms returns to the home screen while still held.

## Build

Install the Flipper Zero external app toolchain, then build from this repository:

```sh
ufbt
```

The app is defined by `application.fam` and builds as an external FAP named `samsung_remote`.

## Install

With a Flipper Zero connected over USB:

```sh
ufbt launch
```

Alternatively, copy the built `.fap` from the `dist/` output directory to the Flipper's apps storage using qFlipper or another file manager.

## Test notes

1. Launch `Samsung Remote`.
2. Confirm Up/Down toggles between `Power` and `Simulate Remote`.
3. Confirm OK on `Power` toggles the Samsung TV power state.
4. Enter `Simulate Remote` and confirm arrows, OK, and a Back tap control TV navigation.
5. Confirm a Back tap does not exit the app in physical mode.
6. Confirm holding Back for at least 3000 ms returns to the home screen before release and does not send `Return`.
7. Confirm Back from the home screen exits the app.

## Status

v0 has been built with `ufbt`, launched on a physical Flipper Zero, and tested successfully against the target Samsung TV.
