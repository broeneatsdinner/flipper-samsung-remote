# Notes

## Assumptions

- v0 uses parsed Samsung32 IR messages hardcoded in `samsung_remote.c`.
- Physical remote mode uses an explicit 3000 ms Back hold threshold instead of Flipper's default long-press threshold.
- A successful Back hold returns Home while Back is still held; the later release is ignored.
- `Hold Back = Home` on the physical remote screen means returning to the app's home menu. It does not send the Samsung `Menu` IR command.
- No hold-volume or directional hold behavior is implemented in v0.

## Unresolved questions

- A later version should decide whether commands are loaded from `/ext/infrared/Samsung.ir` or bundled with the app.

## Build verification

- `ufbt` builds successfully with target 7 and API 87.1.

## IR command mapping

- Home `Power`: `POWER`
- Physical `Up`: `Up`
- Physical `Down`: `Down`
- Physical `Left`: `Left`
- Physical `Right`: `Right`
- Physical `OK`: `Select`
- Physical Back released before 3000 ms: `Return`
- Physical Back held for at least 3000 ms: return to app Home screen while held, no IR command
