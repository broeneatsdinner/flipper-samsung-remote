# Notes

## Assumptions

- v0 uses parsed Samsung32 IR messages hardcoded in `samsung_remote.c`.
- `Hold Back = Menu` on the physical remote screen means returning to the app's home menu, per the v0 UX contract. It does not send the Samsung `Menu` IR command.
- No hold-volume or directional hold behavior is implemented in v0.

## Unresolved questions

- The final package build depends on a local Flipper Zero firmware SDK or `ufbt` install.
- A later version should decide whether commands are loaded from `/ext/infrared/Samsung.ir` or bundled with the app.

## IR command mapping

- Home `Power`: `POWER`
- Physical `Up`: `Up`
- Physical `Down`: `Down`
- Physical `Left`: `Left`
- Physical `Right`: `Right`
- Physical `OK`: `Select`
- Physical short `Back`: `Return`
