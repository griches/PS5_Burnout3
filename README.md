# Burnout 3 for PlayStation 5

A PlayStation 5 port of [Takedawn](https://github.com/xenophobentx/takedawn), the
static recompilation of the original Xbox *Burnout 3: Takedown*. It runs the
2004 game natively on the console at up to 4x resolution, with widescreen,
60 fps crash cameras and the rest of Takedawn's options.

You need your own copy of the game: the **North America (USA) Xbox** disc.
Other regions and the PlayStation 2 version do not work.

> Work in progress. How the release is packaged and installed is still being
> decided; the steps below are for testing.

## Controls

The game uses the DualSense as the Xbox pad, with PlayStation button names:

| Action | Button |
|---|---|
| Accelerate | R2 |
| Brake, reverse | L2 |
| Boost | Cross |
| Menu select / back | Cross / Circle |
| Pause | Options |
| **Takedawn settings menu** | **Touchpad + Options** (press both together) |

The Create button belongs to the console (screenshots and clips), so it does
nothing in the game. The settings menu is also in the game's own menus:
**TAKEDAWN SETTINGS** under Driver Details > Settings, and in the pause menu.

Inside the settings menu: L1 / R1 change page, the d-pad moves and changes a
value, Cross selects, Triangle resets a value, Circle goes back.

The PS2 driving layout (Input > Driving controls) moves accelerate to Cross,
brake to Square, boost to R1 and look back to L1.

## Settings that need a restart

Some settings (render scale among them) take effect the next time the game
starts. Quit to the home screen and start the game again.

## Credits

- [Takedawn](https://github.com/xenophobentx/takedawn) and the
  [xboxrecomp](https://github.com/xenophobentx/xboxrecomp) toolkit, which do the
  real work. This port adds the PlayStation 5 layer.
- The PS5 layer builds on PS5_Vulkan (RADV) and the PS5 payload SDK.

*Burnout 3: Takedown* belongs to Electronic Arts and Criterion Games. This is an
unofficial, non-commercial fan project, not affiliated with them, Microsoft or
Sony. No game data is included.
