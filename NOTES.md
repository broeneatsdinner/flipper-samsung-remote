# Notes

## Assumptions

- v0 uses parsed Samsung32 IR messages hardcoded in `samsung_remote.c`.
- Physical remote mode uses an explicit 700 ms Back hold threshold instead of Flipper's default long-press threshold.
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
- Physical Back held for less than 700 ms: `Return`
- Physical Back held for at least 700 ms: return to app Home screen, no IR command
