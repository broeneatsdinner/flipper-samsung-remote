# Notes

## Assumptions

- v1.0 uses parsed Samsung32 IR messages hardcoded in `samsung_remote.c`.
- The app home screen remains in the normal Flipper orientation.
- Physical remote mode uses `view_port_set_orientation(..., ViewPortOrientationVertical)` so the help screen is readable when the Flipper is held sideways with the IR transmitter pointed at the TV.
- Physical remote mode keeps the D-pad mapped to the visible arrow directions in the rotated screen orientation.
- Physical remote mode uses an explicit 3000 ms Back hold threshold instead of Flipper's default long-press threshold.
- A successful Back hold returns Home while Back is still held; the later release is ignored.
- Physical remote mode uses explicit 2000 ms volume hold thresholds and 200 ms volume repeat intervals.
- User-triggered IR commands are enqueued in a fixed-size ring buffer and transmitted with a 150 ms gap so rapid repeated taps remain distinct.
- If the transmit queue is full, the newest command is dropped instead of blocking input handling.
- `Hold Back = Home` on the physical remote screen means returning to the app's home menu. It does not send the Samsung `Menu` IR command.
- No physical Left, physical Right, or OK hold behavior is implemented in v1.0.

## Unresolved questions

- A later version should decide whether commands are loaded from `/ext/infrared/Samsung.ir` or bundled with the app.

## Build verification

- `ufbt` builds successfully with target 7 and API 87.1.

## IR command mapping

- Home `Power`: `POWER`
- Physical `Up`: `Up`
- Physical `Down`: `Down`
- Physical Up held for at least 2000 ms: repeat `VOL+` every 200 ms until release
- Physical Down held for at least 2000 ms: repeat `VOL-` every 200 ms until release
- Physical `Left`: `Left`
- Physical `Right`: `Right`
- Physical `OK`: `Select`
- Physical Back released before 3000 ms: `Return`
- Physical Back held for at least 3000 ms: return to app Home screen while held, no IR command

## IR transmit queue

- Queue size: 8 command-name pointers.
- Pacing interval: 150 ms between transmissions.
- Queue processing runs in the existing app event loop alongside Back hold and volume hold timers.
- Home Power, physical navigation, OK, Back tap, volume single-shot, and volume repeat commands use the queue.
