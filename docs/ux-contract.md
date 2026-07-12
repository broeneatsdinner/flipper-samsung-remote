# UX Contract

## Home screen

The app starts with two selectable options:

- Power
- Simulate Remote

Behavior:

- Up/Down moves between options.
- OK on Power sends Samsung Power.
- OK on Simulate Remote enters physical remote mode.
- Back exits the app.

## Simulate Remote mode

In this mode, the Flipper physical buttons control the Samsung TV. The app home screen remains in the normal Flipper orientation, but Simulate Remote mode rotates the app view for holding the Flipper sideways with the IR transmitter pointed at the TV.

- Up sends Samsung Up.
- Down sends Samsung Down.
- Left sends Samsung Left.
- Right sends Samsung Right.
- Holding Up for at least 2000 ms repeats Samsung VOL+ every 500 ms until release.
- Holding Down for at least 2000 ms repeats Samsung VOL- every 500 ms until release.
- Left and Right do not trigger volume hold.
- OK sends Samsung Select / Enter.
- Short Back sends Samsung Return / Back.
- Long Back for at least 3000 ms returns to the Home screen while Back is still held.

Short Back must not exit the app while in Simulate Remote mode.
Back release after a handled long Back must be ignored.
