# Burnout 3 for PlayStation 5

[![GitHub stars](https://img.shields.io/github/stars/griches/PS5_Burnout3?style=flat)](https://github.com/griches/PS5_Burnout3/stargazers)
[![Latest release](https://img.shields.io/github/v/release/griches/PS5_Burnout3)](https://github.com/griches/PS5_Burnout3/releases/latest)
[![Licence](https://img.shields.io/badge/licence-GPL--3.0%20as%20built-blue)](LICENSING.md)

A PlayStation 5 port of [Takedawn](https://github.com/xenophobentx/takedawn), the
static recompilation of the original Xbox *Burnout 3: Takedown* by xenophobentx.
The 2004 game runs natively on the console at 60 fps: 4x resolution, widescreen
with an undistorted HUD, and Takedawn's settings menu.

> [!IMPORTANT]
> You need your own disc image of the **North America (USA) Xbox** release of
> *Burnout 3: Takedown*. Other regions (PAL, Japan) and the PlayStation 2
> version do not work. No game data is included.

## What you need

- A PS5 that runs homebrew, with an FTP server and ShadowMountPlus (or another
  way to register a title folder).
- Your own image of the USA Xbox disc of *Burnout 3: Takedown* (`.iso` or
  `.xiso`, any name), or the extracted disc folder.
- About 3 GB free for the game data, plus the image until the first start has
  set it up.

## Set it up

1. Download `Burnout3-PS5-<version>.zip` from the
   [releases](https://github.com/griches/PS5_Burnout3/releases) and unzip it.
   You get a `PPSA99316` folder.
2. Copy the whole `PPSA99316` folder to `/data/homebrew/` on the console.
3. Copy your disc image into `/data/homebrew/PPSA99316/dump/`.
4. Start **Burnout 3** from the home screen. The first start checks your disc
   and sets up the game data. This happens once and takes a few minutes: a
   screen shows the progress, the time left and a clock, and then the game
   starts. Leave it running until it does.
5. You can now delete the image in `dump/`.

If the disc is the wrong region or the image is damaged, a notification says
so and the game closes. An extracted disc folder works too: put the folder that
holds `default.xbe` in `dump/`, or name it `Burnout 3 Takedown` and put it
beside `eboot.bin`. It's checked file by file, and a missing or damaged file is
named.

## Controls

The DualSense stands in for the Xbox pad, and the game's own menus show
PlayStation buttons.

| Action | Button |
|---|---|
| Accelerate | R2 |
| Brake, reverse | L2 |
| Boost | Cross |
| Menu select / back | Cross / Circle |
| Pause | Options |
| **Settings menu** | **Touchpad + Options** (press both together) |

The Create button belongs to the console (screenshots and clips), so it does
nothing in the game. The settings menu is also in the game's own menus:
**TAKEDAWN SETTINGS** under Driver Details > Settings, and in the pause menu.

Inside the settings menu: L1 / R1 change page, the d-pad moves and changes a
value, Cross selects, Triangle resets a value, Circle goes back.

The PS2 driving layout (Input > Driving controls) moves accelerate to Cross,
brake to Square, boost to R1 and look back to L1.

## Settings

The PS5 starts with: 16:9 with the widescreen HUD fix, 4x render scale
(2560x1920 inside), FXAA with light sharpening, half the original motion blur,
texture sharpness following the render scale, vehicles at their farthest detail
distance, scenery far, and the Maximum power mode (steadiest frame times). Crash
cameras and split screen run at the game's own 30 fps: "60 fps crash cameras"
and "60 fps split screen" in the settings menu run them at 60, but the game
slows down wherever the console can't hold it. Change any of them in the settings
menu. A few, render scale among them, take effect the next time the game
starts: close it from the home screen and start it again.

The Xbox Live entry is gone from the main menu: the service closed long ago.

## Where your files are

All in the title folder, `/data/homebrew/PPSA99316/`:

- `Burnout 3 Takedown/`: the game data from your disc.
- `userdata/takedawn/`: saves, settings (`settings.toml`) and bug reports.

Back up `userdata/` to keep your saves.

## Updating

Copy the new `PPSA99316` folder over the old one. Your game data, saves and
settings stay.

## Known issues

- Some frame-rate dips below 60 can happen in busy races.
- The game makes Xbox hard-disk image files (`userdata/takedawn/Partition*.img`)
  as the PC versions do.

## Build from source

This repository holds no game code and no game data. A build translates your
own `default.xbe`, so you need your own disc for that too.

1. `git clone --recursive https://github.com/griches/PS5_Burnout3`
2. Put your extracted disc folder at `gamedata/Burnout 3 Takedown/` and link
   it into `vendor/takedawn/` (`ln -s "../../gamedata/Burnout 3 Takedown" "vendor/takedawn/Burnout 3 Takedown"`).
3. Translate the game on the Mac: `ps5/tools/setup-source.sh`, then in
   `vendor/takedawn`, `python3 -m venv .venv && .venv/bin/pip install capstone resvg-py`
   and `XBOXRECOMP_DIR=../xboxrecomp ./regen.sh --disasm`.
4. Render the button prompts: `ps5/tools/glyphs.sh`.
5. Build in the PS5 Linux VM: `ps5/tools/vm.sh build` (PS5_Vulkan's RADV
   archive, the PS5 payload SDK and LLVM 22, as the other ports in this
   family). The title folder is `dist/PPSA99316/`.

The PS5 changes to Takedawn and its toolkit are `ps5/patches/takedawn.patch`
and `ps5/patches/xboxrecomp.patch`; the console layer is in `ps5/`.

## How it works on the PS5

Takedawn turns the game's x86 code into C, and its runtime answers the Xbox's
kernel, graphics chip, sound chip and USB pads. On the PS5:

- The graphics go through Vulkan on RADV, linked into the title.
- The Xbox's RAM and its mirrors live in the console's direct memory.
- The sound chip's registers are trapped by page faults and decoded from a copy
  of the title's code, because PS5 code is execute-only.
- The guest's registers sit in one thread-local block, so the translated code
  doesn't pay the console's emulated thread-local cost on every access.

## Credits

- **xenophobentx** for [Takedawn](https://github.com/xenophobentx/takedawn) and
  the [xboxrecomp](https://github.com/xenophobentx/xboxrecomp) toolkit, which
  do the real work.
- PS5_Vulkan (RADV for the PS5) and the PS5 payload SDK.
- The settings menu icon is Takedawn's own artwork; the home-screen background
  was made for this port.

## Licence

See [LICENSING.md](LICENSING.md). The PS5 layer is MIT; the patches follow
their upstreams; a built title is GPL-3.0 as a whole.

## Legal notice

This is an unofficial, non-commercial fan project. It is not affiliated with,
endorsed by or sponsored by Electronic Arts, Criterion Games, Microsoft or
Sony. *Burnout 3: Takedown* and all related trademarks belong to their owners.

No game data is included: the download holds the program, which contains code
translated from the game's executable, and no textures, models, sound, music,
films or disc files. The game's data comes from your own disc. Please do not
ask for game files or disc images, and do not share them.
