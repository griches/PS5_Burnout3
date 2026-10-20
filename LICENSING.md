# Licences

The repository as a whole, and any title built from it, is offered under the GNU
General Public License, version 3 ([`LICENSE`](LICENSE)): a built title links the
PS5 payload SDK's platform layer, which is GPL-3.0. Its files come from projects
under several licences, each compatible with that, and may also be used under
their own licence alone.

| Path | Licence |
| --- | --- |
| `ps5/src/`, `ps5/tools/`, `ps5/cmake/` (written for this port, or from the PS5 Vulkan Template by Mihawk) | MIT ([`licenses/MIT.txt`](licenses/MIT.txt)) |
| `ps5/tools/png-to-title-dds.py` (from PS5 RetroArch, by Mihawk) | GPL-3.0-or-later ([`licenses/GPL-3.0.txt`](licenses/GPL-3.0.txt)) |
| `ps5/sdl/` (SDL2 drivers for the console) and `ps5/patches/sdl.patch` | Zlib, as SDL ([`licenses/Zlib.txt`](licenses/Zlib.txt)) |
| `ps5/patches/takedawn.patch` | MIT, as Takedawn |
| `ps5/patches/xboxrecomp.patch` | MIT, as xboxrecomp; the hunk in `src/apu/apu_core.c` LGPL-2.1-or-later, as that file |

## What is built from elsewhere

| Part | Project | Licence |
| --- | --- | --- |
| The game's port | [Takedawn](https://github.com/xenophobentx/takedawn) | MIT |
| The recompiler and Xbox runtime | [xboxrecomp](https://github.com/xenophobentx/xboxrecomp), and the third-party code it carries (Dear ImGui, SMAA, dr_libs, stb_vorbis, fonts, LGPL-2.1 sound code from xemu) | MIT and the licences in its `LICENSES/` and `third_party/` |
| Windowing, sound and pad | SDL 2.32.10 | Zlib |
| Shader compiler | glslang 16.6.0 | BSD-3-Clause and others (its `LICENSE.txt`) |
| Vulkan driver | RADV (Mesa) through PS5_Vulkan | MIT |
| Console support | PS5 payload SDK and its platform layer | GPL-3.0 |

A built title carries these texts in its `licenses/` folder.

## What is not covered

- *Burnout 3: Takedown*, its data and its trademarks belong to Electronic Arts
  and Criterion Games.
- The translated game code in a built title is made from the game's own
  executable; it is not part of this source. Anyone can make it again from
  their own disc with the scripts here, as the build steps in the README say.
- The icon is Takedawn's own artwork (MIT). The home-screen background was made
  for this port.
