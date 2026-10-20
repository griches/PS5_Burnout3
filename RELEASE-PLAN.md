# Burnout 3 for the PS5: release plan

Written 2026-10-20. A plan only: no repository, tag, release, upload or catalog
change has been made. Title `PPSA99316`, "Burnout 3", built from Takedawn 0.2.0
(`vendor/takedawn` 5a00fc1, `vendor/xboxrecomp` 5ae8ad9 = tag `takedawn-0.2.0`).

## Recommendation

Ship it **built**, the way upstream Takedawn ships its own downloads and the way
we shipped Eternal Darkness: a public source repository, `griches/PS5_Burnout3`,
and a release zip of the ready `PPSA99316` folder (the eboot with the translated
game code, no game data) with an empty `dump/` folder in it. The player copies
the folder to `/data/homebrew/`, drops their own USA disc image (`.iso`) into
`dump/`, and starts the game. So yes: "drop the ROM next to it". On the first
start the title runs **Takedawn's own data installer, which is already inside the
program** (`--install`, the XISO reader and the 716-file SHA-256 manifest). It
checks the image, unpacks it into the title folder once, and says what it is
doing with console notifications. Then the image can be deleted. An extracted
disc folder named `Burnout 3 Takedown` works too and is checked where it lies.
No separate installer program and no builder app. With a built zip the title can
then go on ProsperoStore, which only lists built titles.

### Short answers to the three questions

- **A repository?** Yes: `griches/PS5_Burnout3`, source only (the `ps5/` layer,
  the patches, the two upstream submodules). Never game files or generated code.
- **An installer?** Not a separate one. The program already has one; it needs a
  small PS5 front end (about a day of work, below) so it runs by itself on the
  first start and talks through notifications, since there is no setup window
  on the console.
- **Drop a ROM next to it?** Yes, into `dump/`. The game cannot read the image
  directly every time (the runtime only reads plain files), so the first start
  unpacks it once: about 2.5 GB, checked file by file.

## Why built, not a builder app

- **Upstream does exactly this.** Takedawn's downloads are "only the program"
  with "code translated from the game's executable" inside; the player's disc
  gives the data (`vendor/takedawn/README.md`, Legal notice;
  `docs/installer.md`, "Since the 2026-10-11 decision the releases ship the
  prebuilt program with the compiled game code"). Its own credits name
  SpeedBreaker's model: "a prebuilt program, data from the player's own disc".
  A PS5 build of the same code takes no more risk than their four downloads.
- **Same choice as Eternal Darkness.** You chose built for ED ("the user just
  adds their game"), and it is live on the store. Mario Galaxy went the builder
  route because it is a Nintendo game; Burnout 3 is EA's, and the upstream
  project already ships built.
- **A builder app is a lot of work for little gain here.** The recompiler is
  Python with capstone, the output is 66 MB of C, and the PS5 compile and link
  need the payload SDK, LLVM, PS5_Vulkan's RADV archive and its SELF tool. That
  is the Mario Galaxy job again (Mac and Windows apps, notarising, a Windows
  test PC) for a game whose upstream doesn't bother.
- **The store needs a built title.** The catalog accepts one format: a zip of
  the app folder with `eboot.bin` and `sce_sys/`. A builder release can't be
  listed (Mario Galaxy isn't). Other recompilation ports with built eboots are
  listed today (Rush 2 Recompiled, Wave Race 64 Recompiled, FFTARecomp, Eternal
  Darkness), and `console-ports` is described as "decompilations and
  recompilations. They usually need your own game data."

## The options compared

### A. Where the game data comes from

| Option | How the player does it | Good | Bad | Work |
| --- | --- | --- | --- | --- |
| **A1. Extracted folder only** (works today) | FTP the 716-file `Burnout 3 Takedown` folder into the title folder | Nothing to write; it is how your console runs it now ("checkout" mode) | 716 files over FTP is slow and easy to get half done; nothing is checked, so a PAL folder or a broken upload just crashes; no message at all when it's missing (the title just closes) | None |
| **A2. Image in `dump/`, installed on the first start** (recommended) | FTP one `.iso` into `dump/`, start the game | One file to copy; every file checked against the manifest; wrong disc, damaged image or no disc gets a clear message; a stopped install carries on next time; the folder route still works and is checked in place | Needs the image's space plus 2.5 GB until the image is deleted; about a day of work | A small PS5 front end (work items 2 to 4) |
| A3. Run straight from the image | FTP the `.iso`, play from it every time | No unpacking | The runtime opens files by path (`kernel_path.c`); reading from an image means a virtual file layer in the toolkit's kernel (open, read, seek, info, directory listing) that upstream doesn't have. A redump image stays 7.8 GB on the console instead of 2.5 GB | Large, and a patch we'd carry for ever |
| A4. A computer-side installer | Run a Mac or PC app that unpacks the disc, then FTP the result | Familiar from Takedawn's desktop setup window | Another app to build and sign for nothing: the program already has the installer; the player still uploads 716 files | Medium |

What the installer accepts, from `docs/installer.md`: an `.iso` in XISO layout,
a full redump image (video partition first), or an extracted folder (the game
folder itself or one level down). It refuses any other `default.xbe`: only the
USA retail disc, sha256 `9f497acd...`, title id `0x4541005B`. It copies each
file through `<name>.part` while hashing it, keeps `install.journal` so a run
that was stopped resumes, and writes `.complete` last. A folder that is already
the destination is checked in place, with nothing copied. It wants 3 GB free
(less whatever is already there). Upstream measured 3 to 10 seconds on its
computers and the Steam Deck; the PS5 should be close (to be measured).

How progress can be shown on the PS5: the installer reports through callbacks
(`xr_install_opts.progress`, `log`, `cancelled` in
`vendor/xboxrecomp/src/install/disc_install.h`); `install.c` prints them as
`PROGRESS disc 0.420 <file>` lines, and Android passes them to its own page
(the JNI functions at the end of `vendor/takedawn/src/game/install.c`). The PS5
has no setup window and the game's renderer isn't running yet, so the simple way
is a **console notification** (`sceKernelSendNotificationRequest`, which a title
can call: PS5_SwitchWaker already uses it for its disc errors, in
`vendor/switchwaker/native/src/pc/runtime/pc_disc.cpp`) when it starts, maybe
every 25 % if it turns out slow, and when it's done or failed. Optionally the
shell's splash can stay up during the install instead of a black screen. A real
progress bar on screen would need a drawing path before the game starts: not
worth it for a step this short and done once.

### B. What a release ships

| Option | Good | Bad |
| --- | --- | --- |
| **B1. Built title zip** (recommended) | Copy, drop disc, play. Store listing possible. Same as upstream and ED | The eboot (and its `code.bin` copy of the code) holds code translated from EA's executable |
| B2. Builder apps (Mario Galaxy) | No translated code in the download | Mac and Windows apps to build, sign and test; minutes of compiling on the player's machine; no store listing |
| B3. Source only, build it yourself | Nothing of the game anywhere | Needs the Lima VM, the PS5 stack and Python: almost nobody could play it |

### C. Where the player's files live, and store updates

Everything is in the title folder today: the game data (`Burnout 3 Takedown/`),
saves, settings and logs (`userdata/takedawn/`), the shader cache (`cache/`).
That is what all your listed ports do, and copying a new release over the
folder keeps it all.

**ProsperoStore behaves differently.** Its README says: "An update replaces the
app's whole folder; the previous folder, with anything you had put inside it, is
kept in `/data/prosperostore/previous/<TITLE ID>` until that app's next update."
Uninstall deletes the folder. For Burnout 3 that means a store update leaves the
game with no data and no saves until the player moves them back by FTP, and the
next update throws the old copy away. This isn't a listing blocker, and ED and
the others have the same problem with their saves, but here it is 2.5 GB plus
the saves.

| Option | Good | Bad |
| --- | --- | --- |
| **C1. Keep it all in the title folder** (recommended for the first release) | Works now, same as every other port; easy to see and back up over FTP | Store updates move it aside; the README must say what to move back (the first start re-checks a moved-back folder by itself, so moving `Burnout 3 Takedown` and `userdata` back is enough) |
| C2. `/download0` | The catalog itself recommends it ("survive replacing the app"); a title can see it without help | Untested on our titles: does ShadowMountPlus give homebrew a `/download0`, does `downloadDataSize` (256 in our `param.json`, MiB) limit it, and can players reach it by FTP to delete the image or back up saves? |
| C3. `/data/<somewhere>` through the Lapy daemon (`ps5platform/elevation.h`) | Survives everything | Needs a Lapy daemon running on the player's console; not something to depend on |

### D. Disk space: the partition images

The toolkit makes six Xbox hard-disk images in the save folder on every start
(`vendor/xboxrecomp/src/kernel/kernel_path.c`, the POSIX branch of
`xbox_path_init`): `Partition0.img` (the partition table), `Partition1.img`
2,362,982,400 bytes, `Partition2.img` 512,000,000, `Partition3` to `5`
786,432,000 each: 5.2 GB in all. They are sized with `ftruncate`, which leaves a
hole on file systems that support sparse files, so on a Mac or Linux they cost
almost nothing. The title asks a partition its length from the file's size
(`IOCTL_DISK_GET_PARTITION_INFO` in `kernel_bridge.c`), so the sizes matter.

What the game uses: the console log shows Burnout 3 opening only `Partition0`
(the table) and `Partition5` (Z:, the utility cache it formats) as devices.
`Partition1` paths (`UDATA`, `TDATA`, `CACHE`) go to folders, so
`Partition1.img` is never written. FTP shows the full sizes either way; whether
the PS5's `/data` keeps the holes is **not known yet**. If it doesn't, every
player loses 5.2 GB to empty files, and copying the folder back to a computer
always costs 5.2 GB. Check it before release (work item 5).

## Work, in order

Sizes are rough: S = an hour or two, M = half a day to a day.

0. **Gate: play it first.** The port boots and draws on the console today
   (2026-10-20). Release when a race, Crash mode, a save and the settings menu
   have been played on the console, and you're happy with it.

1. **Make the generated code the release way (S).** Regenerate with
   `vendor/takedawn/tools/release/gen.sh` instead of plain `regen.sh`, so the
   tree has `regen.stamp` and the program names its regen key and patches in
   `--version` and the log (useful for bug reports). Try `-DBO3_RELEASE=ON` in
   `ps5/tools/build.sh`'s configure; it renames the program target to
   `takedawn`, so `link-title.sh` and the default ninja target may need
   following. Skip the flag if it fights the PS5 link; the stamp is the useful
   part.

2. **First-start data step (M).**
   - `ps5/patches/takedawn.patch`: add one small exported function to
     `src/game/install.c`, for example
     `int bo3_install_from(const char *source, const char *dest, int check_only, void (*progress)(void *, double, const char *), void *ctx, char *message, size_t n)`,
     which fills `xr_install_opts` from the file's own `k_manifest`,
     `GAME_DIR` and `FREE_NEEDED` and returns the code and Takedawn's sentence.
     (The manifest is `static` there today, so the PS5 layer can't call
     `xr_install` itself; `bo3_install_main` works but only prints.) Worth
     offering upstream later: it is the same shape as their Android entry.
   - New `ps5/src/firstrun.c` (MIT), called from `__wrap_main` in
     `ps5/src/startup.c` before `__real_main`:
     1. If `/app0/.complete` is there, start the game.
     2. Else look for a source, in this order: the folder
        `/app0/Burnout 3 Takedown` (checked in place, nothing copied); an
        `.iso` or `.xiso` in `/app0/dump/` (any name, any case); a folder in
        `dump/` that holds `default.xbe` (or one level down); an `.iso` in
        `/app0/` itself (the habit from SwitchWaker, Dusklight and Sunshine).
        Identify each with `check_only` and take the first that is the right
        disc, so a PAL image beside the USA one doesn't stop it.
     3. Install into `/app0` (files in `/app0/Burnout 3 Takedown/`, marker
        `/app0/.complete`), with notifications from the progress callback.
     4. Success: notify "game data ready, you can delete the disc image in
        `dump/`" and start the game. Never delete the player's image ourselves.
     5. Failure or nothing found: notify with Takedawn's own sentence (it names
        the region or version it found) or "put your USA disc image in
        `/data/homebrew/PPSA99316/dump/`", then close the title.
   - Start the game with `--data /app0` added to the arguments in
     `__wrap_main`. That makes Takedawn insist on the `.complete` marker
     (case 2 in `install.h`), so an unchecked folder is never run. `args.txt`
     still works for test runs.
   - Add `firstrun.c` to the `ps5_platform` library in
     `ps5/cmake/ps5-game.cmake`.

3. **Notifications and error boxes (S).**
   - `ps5/src/platform.c`: a `notify(const char *format, ...)` helper (the
     request struct and call as in SwitchWaker's `pc_disc.cpp`), plus a klog
     line with the same text.
   - `ps5/tools/link-title.sh`: `--wrap=SDL_ShowSimpleMessageBox` to a PS5
     function that notifies. Takedawn's `bo3_fatal_box` (`src/game/fatal.c`)
     uses it for "the game's default.xbe cannot be read" and "the memory the
     game needs cannot be reserved"; on the PS5 those boxes go nowhere today.
   - Check on a television that the text is readable: SwitchWaker's README
     still says it hasn't been read on a TV.

4. **The splash during the install (S, test only).** `platform_init` hides the
   shell's splash in the start-up constructor. Try hiding it only after the
   install, so the player sees the splash and a notification rather than black.
   Keep it only if the shell doesn't object to a splash held that long.

5. **Partition images (S to measure, M if they need changing).** Log
   `st_blocks * 512` beside `st_size` for each `Partition*.img` once at start (a
   klog line in the POSIX branch of `xbox_path_init`, `ps5/patches/xboxrecomp.patch`),
   or compare the free space in Settings > Storage before and after deleting
   them. If the console keeps the holes: nothing to do, note it in the README.
   If not: create `Partition1` to `4` only when the title opens that device, or
   keep them empty and answer the length from the table (`s_part_sectors`)
   instead of the file size. Either way the change is in the toolkit's POSIX
   branch and could go upstream.

6. **Test on the console (M).** One run each, with a fresh title folder
   (deploy to a scratch copy, never over your own saves):
   - XISO in `dump/` (the common case); a full redump image; the extracted
     folder in place; an extracted folder inside `dump/`.
   - A wrong disc (any non-USA image) and no disc: the notification says why.
   - Close the title halfway through the install, start again: it carries on.
   - Copy a new build over an installed folder: data, saves and settings stay,
     and the start is quick (marker only).
   - Move `Burnout 3 Takedown` and `userdata` out and back (the store-update
     case): the first start re-checks and plays.
   - How long the install takes, so the README can say.

7. **Licences in the title (S).** `link-title.sh` copies seven texts today.
   Match upstream's set (`copy_licenses` in
   `vendor/takedawn/tools/release/common.sh`) and add the PS5 parts:
   - from the toolkit: `third_party/imgui/LICENSE.txt` (Dear ImGui, the settings
     menu), `third_party/smaa/LICENSE.txt`, `third_party/dejavu/LICENSE.txt`,
     `third_party/roboto_condensed/OFL.txt`, `third_party/dr_libs/LICENSE.txt`,
     `third_party/stb_vorbis/LICENSE.txt` (the LGPL-2.1 and ADPCM-XQ texts are
     already there inside `xboxrecomp-LICENSES/`);
   - the PS5 stack: Mesa's licence for RADV (MIT), the PS5_PayloadSDK and its
     platform layer (GPL-3.0, same text as PS5_Vulkan's), zlib (Mesa's copy in
     the RADV archive), LLVM libc++ (Apache-2.0 with LLVM exception, if the SDK's
     libc++ is linked for glslang and ImGui), and the clean-room `libc.prx`
     (GPL-3.0, from ps5-native-app-boilerplate via PS5_Vulkan);
   - `licenses/SOURCE.txt`, written at link time: this repository's commit, the
     two upstream commits, and the PS5 stack's revisions (PS5_Vulkan, the RADV
     archive's `PROVENANCE.txt` revision, PS5_PayloadSDK `611893f`, SDL
     2.32.10, glslang 16.6.0), with the sentence upstream uses: the game's own
     code is made from the player's executable and is not part of the source.
     Upstream relies on this for the LGPL-2.1 sound code, and our
     `xboxrecomp.patch` changes one of those LGPL files (`src/apu/apu_core.c`),
     so that hunk must be public, which it will be.

8. **`ps5/tools/package-release.sh` (S),** on the model of
   `PS5_EternalDarkness/ps5/tools/package-release.sh`:
   - zip `dist/PPSA99316/` as `dist/Burnout3-PS5-<version>.zip`, with an empty
     `dump/` holding `PUT YOUR DISC HERE.txt` ("your own image of the North
     America Xbox disc of Burnout 3: Takedown, `.iso`, any name; or the
     extracted folder, named `Burnout 3 Takedown`, beside `eboot.bin`");
   - refuse to zip if anything from the disc or the player is inside: `*.xbe`,
     `*.iso`, `*.xiso`, `*.img`, `*.part`, a `Burnout 3 Takedown` folder,
     `.complete`, `install.journal`, `userdata/`, `cache/`, `settings.toml`,
     `env.txt`, `args.txt`, `devloop-shot`;
   - refuse a dirty tree (the port, and the submodules beyond the saved
     patches, which `setup-source.sh` already compares), and a `contentVersion`
     not higher than the last tag's;
   - print the zip's size and sha256 for the catalog record. (The current build
     zips to about 39 MB.)

9. **Repository files (S).**
   - `LICENSE` (GPL-3.0 text) and `LICENSING.md` on the PS5_Dusklight pattern:
     the `ps5/` layer is MIT (with `png-to-title-dds.py` GPL-3.0 and `ps5/sdl`
     Zlib), the patches follow their upstreams (Takedawn and xboxrecomp MIT,
     the `apu_core.c` hunk LGPL-2.1-or-later, `sdl.patch` Zlib), a built title
     is GPL-3.0 as a whole because it links the payload SDK's platform layer,
     and "what is not covered": the game, the translated code (made from your
     own `default.xbe`), trademarks, and the art (the crimson T icon is
     Takedawn's own MIT artwork; the background is ours, made with ChatGPT).
     Say plainly that the GPL's offer of source covers everything except the
     translated game code, which anyone can make again from their own disc with
     the scripts here: the same position as ED and as upstream's LGPL note.
   - `.gitignore`: add `env.txt`, `args.txt`, `*.iso`, `*.xiso`, `*.xbe`,
     `*.img` as a second fence.
   - Before the first push, check the history once more for game files and
     private data (done today for the four commits: the biggest blobs are the
     two `pic0.dds` versions, the art source and the placeholder icon picture;
     no IPs, hostnames or home paths).

10. **README (S),** in the shape of PS5_EternalDarkness's: title, badge row
    (stars, release, `licence-GPL--3.0 as built` linking `LICENSING.md`), the
    opening line crediting Takedawn (no big credit box), a "USA disc only"
    callout, then What you need, Set it up (the steps below), Check the folder,
    If something's wrong, Updating (by hand and with ProsperoStore), Where your
    files are, Controls and the settings menu (keep the current table: Touchpad
    + Options), What's better than on the Xbox (Takedawn's highlights that hold
    on the PS5), Known issues, Reporting a bug (Settings > Advanced > Create bug
    report writes a zip under `userdata/takedawn/`; fetch it by FTP), How it
    works, Build from source (your own `default.xbe`, `gen.sh` on the Mac,
    `ps5/tools/glyphs.sh`, `ps5/tools/vm.sh build`, `console.sh`), Credits,
    Licence, Legal notice. For the legal wording, adapt upstream's: unofficial,
    free, not affiliated with Electronic Arts, Criterion Games, Microsoft or
    Sony; "no game data is included: the download holds the program, which
    contains code translated from the game's executable, and no textures,
    models, sound, music, films or disc files". Don't write "no game code".
    Screenshots: frames of the port running on your console, captured by you.

11. **Title art and tile music (S).** The icon is Takedawn's crimson T (MIT,
    original artwork, fine to use); the background is your ChatGPT crash scene,
    no logos. Do **not** make a `snd0.at9` from the game: Burnout 3's music is
    EA Trax, licensed songs by real bands. Leave the tile silent (and drop
    `loudnessSnd0` from `param.json`, or leave it, it's harmless), or use
    original music.

12. **Version (S).** First release `contentVersion` `01.000.000` (it is
    already). Raise it on every later release. Tag scheme: see the questions.

13. **Publish (you).** Create `griches/PS5_Burnout3` private, push with the
    badges and topics already in (`ps5`, `ps5-homebrew`, `homebrew`, `burnout`,
    `burnout-3`, `takedawn`, `static-recompilation`, `xbox`, `radv`, `vulkan`),
    you read the README, then make it public, tag, and attach the zip to a
    GitHub release whose text is the store's release notes (changes first,
    short headings, flat lists). Never replace that zip once it's listed.

14. **ProsperoStore (you).** A PR from `griches/ps5-homebrew-catalog` to
    `darkxex/ps5-homebrew-catalog` (the catalog moved there) adding
    `apps/PPSA99316.json`: category `console-ports`, `icon_url` pinned to the
    tag's `ps5/sce_sys/icon0.png` (the catalog then finds `param.json` beside
    it), `sha256` from step 8, the PR template with a reviewer note like ED's:
    the eboot holds code translated from the game's executable, all game data
    comes from the player's own disc, and upstream Takedawn distributes its
    program the same way. The catalog's daily discovery job may propose the
    listing itself once the repository is public with a release.

## What could block a ProsperoStore listing

Nothing found that blocks it:

- **Scope:** a native title with `eboot.bin` and `sce_sys/`; recompilations are
  what `console-ports` is for, and several built ones are listed.
- **Title ID:** `PPSA99316` is free (the live index of 2026-10-20 19:07Z has
  110 apps and no Burnout or Takedawn). No other Burnout 3 port is listed, so the
  "distinct app" rule doesn't come up.
- **Format and size:** a zip of the folder, about 39 MB (limit 2 GiB).
- **Version:** `contentVersion` in `NN.NNN.NNN` form, committed at the tag.
- **Provenance:** the policy refuses "repackaged or rebranded copies of someone
  else's app" without consent. A PS5 port with its own layer isn't that (your
  Dusklight, SwitchWaker, Ghostship and Sunshine ports of other people's
  projects are listed), but telling xenophobentx first is polite and removes any
  doubt (question below).
- **"I have the right to distribute this build under the stated license":** the
  same box you ticked for ED, with the same reviewer note.
- **Not a blocker, a player problem:** store updates move the title folder,
  with the game data and saves, to `/data/prosperostore/previous/PPSA99316`
  (part C).

## README install steps, as players would read them

> **USA disc only.** You need your own copy of *Burnout 3: Takedown* for the
> original Xbox, the North America (NTSC-U) release. PAL and Japanese discs, the
> PlayStation 2 and GameCube versions and the Xbox 360 download don't work.

### What you need

- A PS5 that runs homebrew (etaHEN or similar), with **ShadowMountPlus** and an
  **FTP server** (port 2121).
- An image of your disc: an `.iso` (an XISO, about 2.5 GB, or a full redump
  image, about 7.8 GB). The extracted folder of disc files works too.
- About 3 GB free on the console, plus the size of your image until you delete
  it.

### 1. The game

1. Download `Burnout3-PS5-....zip` from the
   [latest release](https://github.com/griches/PS5_Burnout3/releases/latest)
   and unzip it. You get a folder called `PPSA99316`.
2. Copy that folder to the console over FTP, as `/data/homebrew/PPSA99316/`.

### 2. Your disc

3. Copy your disc image into `/data/homebrew/PPSA99316/dump/`. Any file name
   ending in `.iso` works.

   Have the extracted files instead? Copy the folder into
   `/data/homebrew/PPSA99316/` and name it exactly `Burnout 3 Takedown`, with
   `default.xbe` directly inside it.

### 3. Play

4. Wait a few seconds for **Burnout 3** to appear on the home screen, and start
   it.
5. The first start checks every file of your disc and unpacks it. A
   notification says when it starts and when it's done (about N seconds). Then
   the game starts. This happens once.
6. You can now delete the image in `dump/` to get the space back. The game
   doesn't need it again.

### Check the folder

```
/data/homebrew/PPSA99316/
├── eboot.bin, code.bin, sce_sys/, sce_module/, licenses/   (from the zip)
├── dump/                     your disc image (delete it after the first start)
├── Burnout 3 Takedown/       the game's files, unpacked from your disc
├── userdata/takedawn/        saves, settings.toml, logs, bug reports
└── cache/                    the shader cache
```

### If something's wrong

- **It isn't on the home screen:** restart ShadowMountPlus (or the console).
- **"Can't start the game or app" (CE-107750-0):** your FTP program uploaded the
  files without the right permissions. Set `PPSA99316` and everything in it to
  `777`.
- **A notification says the disc isn't supported:** it names what it found.
  Only the USA Xbox disc works.
- **A notification says no disc was found:** the image isn't in `dump/`, or the
  folder isn't named `Burnout 3 Takedown`.
- **The game closed during the first start:** start it again. It carries on
  where it stopped.

### Updating

- **By hand:** copy the new release's `PPSA99316` folder over the old one. Your
  game data, saves and settings stay.
- **With ProsperoStore:** the store moves the old folder, with your game data
  and saves inside it, to `/data/prosperostore/previous/PPSA99316/`. Move
  `Burnout 3 Takedown` and `userdata` from there back into
  `/data/homebrew/PPSA99316/` with your FTP program, then start the game: it
  checks the files again and goes on.
- **Uninstalling deletes your saves.** Copy `userdata/` somewhere first if you
  want to keep them.

## Open questions for Gary

1. **Built, like ED and upstream?** That is the recommendation. Say if you'd
   rather do builder apps for this one too.
2. **Where the disc goes:** `dump/` like ED (recommended, and the zip carries an
   empty `dump/`), or beside `eboot.bin` like SwitchWaker and Sunshine? The
   first start can look in both either way; it's about what the README says.
3. **Store updates and the player's data:** keep everything in the title folder
   for the first release and explain the move-back in the README (C1), or test
   `/download0` first (C2), which the catalog recommends?
4. **Delete the disc image after a good install,** or only tell the player they
   can? The plan says tell, never delete their files.
5. **Repository name:** `griches/PS5_Burnout3` (matches this folder and the
   title name; recommended) or `PS5_Takedawn` (matches the upstream project,
   the way PS5_Dusklight and PS5_SwitchWaker are named)?
6. **Version numbers:** follow upstream (`v0.2.0`, like Sunshine followed its
   upstream and ED its recomp), or start the port at `v1.0.0`?
   `contentVersion` starts at `01.000.000` either way.
7. **Tell upstream first?** A short issue on `xenophobentx/takedawn` saying a
   PS5 port exists and is about to be listed. Optionally offer the small
   install hook (work item 2) and the partition-image fix (item 5) as PRs.
8. **The icon:** keep Takedawn's crimson T, or make a new one in the style of
   the ChatGPT background (no EA logos either way)?
9. **Tile music:** silent (recommended: the soundtrack is licensed songs), or
   something original?
10. **Store listing:** submit the catalog PR yourself after the release, or let
    the catalog's discovery job propose it?
