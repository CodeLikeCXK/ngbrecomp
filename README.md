![Ninja Gaiden Black Recompiled](docs/images/banner.jpg)

# Ninja Gaiden Black Recompiled

**A PC port of Ninja Gaiden Black by static recompilation.**

A native Linux and Windows version of *Ninja Gaiden Black* (Xbox, 2005), made by statically
recompiling the game's own code to run directly on a PC — not an emulator. It draws with its own
Vulkan renderer, which takes over the game's Direct3D calls instead of emulating the Xbox GPU. It
plays at a steady 60 fps on modest hardware (including the Steam Deck) and adds PC options: higher
resolutions, widescreen and ultrawide, keyboard and mouse, rebindable controls and more.

**You need your own copy of the game.** This project contains none of the game's data (graphics,
sound, movies, text): on first start it asks for your disc image and installs the game from it. It is made for the **retail USA
release** (NTSC-U, title ID `5443000D`) and checks for it.

## Features

- Native Linux (AppImage) and Windows (x64) builds; Steam Deck friendly
- Render resolution up to 10x, anisotropic filtering, MSAA, sharper shadows
- 16:9, 16:10 and 21:9 as well as 4:3: the 3D fills the screen, the HUD goes to the screen's
  corners, menus keep the game's own 16:9 look
- Keyboard and mouse with key prompts in the game, rebindable controls for keyboard, mouse and
  controller, mouse support in the menus
- Skippable intros and movies, unlock-all options for difficulties and costumes, optional cheats
- Smooth frame pacing and low input latency; shaders prepared in the background (no first-time
  stutter)

## Getting started

### Linux and Steam Deck

1. Download `NinjaGaidenBlack-x86_64.AppImage` from the releases page and make it executable
   (`chmod +x`, or Properties → Allow executing).
2. Run it. On the first start a window asks for your copy of the game: a disc image (`.iso`, a full
   Redump dump, or the `.7z`/`.zip` it came in) or a folder of extracted files with `default.xbe`.
   The game is installed once (about 6 GB).
3. On a Steam Deck, add the AppImage to Steam (Add a Non-Steam Game) to play it in Game Mode, and
   choose 16:10 in the settings.

   If it closes as soon as you press Play in Game Mode, that's a bug in the current stable Steam
   client with non-Steam games (fixed in the Steam Beta). Until the next stable release, either
   switch Steam to the Beta, or set the game's Launch Options in Steam to
   `APPIMAGE_EXTRACT_AND_RUN=1 %command%` (#11).

Needs a 64-bit Linux with glibc 2.35 or newer and a GPU with Vulkan 1.3 drivers.

### Windows

1. Download `NinjaGaidenBlack-windows-x64.zip` and extract it anywhere.
2. Run `NinjaGaidenBlack.exe` and choose your disc image as above.

Needs Windows 10 or 11 (64-bit) and a GPU with Vulkan 1.3 drivers.

### macOS (experimental)

Build the Apple Silicon app using the commands below, extract the resulting zip, and open
`Ninja Gaiden Black.app`. Choose your own disc image or extracted game folder on first start.
Current dependencies require macOS 26. Intel Macs and the xemu renderer are unsupported;
packages are signed ad hoc, without Developer ID signing or notarization.

### Where things are kept

| | Linux | Windows | macOS |
|---|---|---|---|
| Installed game, saves | `~/.local/share/NinjaGaidenBlack` | `%LOCALAPPDATA%\NinjaGaidenBlack` | `~/Library/Application Support/NinjaGaidenBlack/saves` |
| Settings (`settings.ini`, `controls.ini`) | `~/.config/NinjaGaidenBlack` | `%APPDATA%\NinjaGaidenBlack` | `~/Library/Application Support/NinjaGaidenBlack/config` |
| Shader cache | `~/.cache/NinjaGaidenBlack` | `%LOCALAPPDATA%\NinjaGaidenBlack\shaders` | `~/Library/Application Support/NinjaGaidenBlack/cache/NinjaGaidenBlack/shaders` |
| Log | the terminal, or `log.txt` beside the saves when started without one | `log.txt` beside the saves (or the console it was started from) | `log.txt` beside the saves |

**Portable mode:** with a file named `portable.txt` next to `NinjaGaidenBlack.exe` (or the AppImage or Mac app),
all of these go into a `userdata` folder next to it instead. The first-run setup offers this as a
checkbox. The folder must be writable, so not inside Program Files. Delete `portable.txt` to go back.

## Settings

Everything is in the game's own menus: **RECOMP SETTINGS** on the main menu, and the **Settings** tab
of the pause menu. Changes apply at once and are saved.

- **Graphics**: render resolution (1x–10x), texture filtering (the console's, or 2x–16x
  anisotropic), anti-aliasing (MSAA), shadow resolution, Smoother Glow (a softer, fainter glow on
  bright lights; off is the console's look)
- **Display**: aspect ratio (4:3, 16:9, 16:10, 21:9), window size, windowed / fullscreen /
  borderless, V-Sync (Off, Mailbox, FIFO, Adaptive), frame-rate counter
- **Startup**: skip the warnings and intro movies, all difficulties from the start, all costumes
- **Gameplay** (each off unless chosen): the Quick Menu, an NG2-style d-pad menu for weapons, Ninpo
  and items, and whether the game pauses while it is open
- **Cheats** (for the session only): infinite health, ninpo, essence; golden scarabs
- **Controls**: rebind every action (two keys or mouse buttons and a controller button each); camera
  inversion; start in third person

The game's own Options (sound, movie aspect, controller vibration and so on) are where they always
were.

## Controls

A controller works as on the Xbox. The keyboard and mouse can be used at any time — the game follows
whichever was touched last, and its button prompts change to match.

| Action | Keys | | Action | Keys |
|---|---|---|---|---|
| Move | W A S D | | Camera | mouse (numpad 8 4 5 6) |
| Attack | left mouse, J | | Special attack | right mouse, K |
| Jump | Space | | Projectile | E |
| Ninpo | F | | Wind run | C |
| Guard / roll | Shift | | Center camera | Q, middle mouse |
| Map | Tab | | Karma window | R |
| Pause | Esc, Enter | | First-person view | Z |

After loading, the right stick (or the mouse) looks around in first person, as on the Xbox; click the
right stick (V) to turn the third-person camera instead. With **Start in Third Person** (RECOMP
SETTINGS → Controls) the stick turns the third-person camera after every load. **Invert Camera X / Y**
flips the stick and mouse for the third-person camera, **Invert First-Person X** first-person left and
right; first-person up and down is the game's own Options → Controller Settings → 1st Person Camera
Control.

With the **Quick Menu** on (RECOMP SETTINGS → Gameplay), Up and Down on the d-pad (or the Up / Down
arrows, 1 / 2) open it on the melee weapons or the items; Up and Down then change category, Left and
Right choose, A / Enter equips or uses, B / Escape closes. Its keys are in Controls.

In the menus the mouse points and clicks, Esc goes back and Q / E switch the pause menu's tabs.
Movies can be skipped with Start, A or B (or the keys for them); in-game cutscenes with Start.

Other keys: **F10** frame-rate counter, **F11** or **Alt+Enter** fullscreen.

## Known issues

- The **Ninja Gaiden arcade cabinet** in Han's bar does not work: the game closes when you play it
  (#18). The NES games are a separate program on the disc (`NinjaTrilogy.xbe`) that the Xbox switches
  to, and only Ninja Gaiden Black itself is recompiled. Save before trying it.
- At **21:9**, objects at the far left and right of the screen can appear a moment late (the game
  decides what to draw for a narrower view).
- At **10x render resolution with 8x anti-aliasing**, where the frame rate drops below 60, characters'
  shadows can flicker (#53). 4x anti-aliasing at 10x, or 8x at 8x, does not do it.
- The Windows build has been tested under Proton and in a Windows 11 virtual machine; reports from
  real Windows PCs are especially welcome.

When reporting a graphics glitch, pressing **F7** while it is on screen records the next few seconds of
drawing to `~/ngb-trace-<n>` on Linux, or `C:\Users\<you>\ngb-trace-<n>` on Windows — attach that
folder (zipped), it shows exactly what went wrong.

## Building from source

The build needs the game's code, so it starts from your own disc image.

Requirements: clang, CMake, Ninja, Python 3, git, SDL2, GLib, glslang and SPIRV-Tools (Qt 6
Widgets for the setup window, optional). The recompiler and Xbox runtime,
[xboxrecomp](https://github.com/sp00nznet/xboxrecomp) with this port's changes, is included in
`xboxrecomp/`.

```sh
xboxrecomp/third_party/fetch.sh                # once: Vulkan headers, volk, VMA and the like
tools/pipeline.sh /path/to/your/ngb.iso        # extract, disassemble and lift to C (a few minutes)
cmake -S . -B build/linux -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
ninja -C build/linux
build/linux/NinjaGaidenBlack
```

Packages: `packaging/appimage/build.sh` (in a Docker container, for an AppImage that runs on older
systems), `packaging/windows/build.sh` (cross-compiled from Linux with llvm-mingw), and
`packaging/macos/build.sh` (native Apple Silicon). Packages contain no game data.

On macOS, install the build dependencies below, then run the fetch and pipeline steps above
and package the app:

```sh
brew install cmake ninja sdl2-compat sdl3 libepoxy glib glslang molten-vk vulkan-loader qt
packaging/macos/build.sh
```

The zip is `build/macos-release/NinjaGaidenBlack-macos-arm64.zip`. Homebrew is needed only to
build; runtime libraries, including MoltenVK, are bundled. `NGB_MAC_PREFIX` overrides the build
dependency prefix (default: `brew --prefix`).

How it works, and the details of every feature: [docs/TECHNICAL.md](docs/TECHNICAL.md) and
[docs/renderer.md](docs/renderer.md). How to contribute: [CONTRIBUTING.md](CONTRIBUTING.md); the layout, tools
and test rules, also for coding agents: [AGENTS.md](AGENTS.md).

## Credits

- [xboxrecomp](https://github.com/sp00nznet/xboxrecomp) by sp00nznet: the recompiler and Xbox
  runtime this is built on (MIT)
- [xemu](https://xemu.app): the NV2A GPU model and the APU's voice processor (the files used here are
  LGPL-2.1-or-later, its shader generators GPL-2.0 or 3.0; xemu as a whole is GPL-2.0)
- TGP482's [SCDA-XBERecomp](https://github.com/TGP482/SCDA-XBERecomp) and
  [Xbox-XBERecomp](https://github.com/TGP482/Xbox-XBERecomp): the renderer's shader, vertex and
  texture code (MIT)
- Fonts: EB Garamond and Liberation Serif (SIL Open Font License)
- Keyboard and mouse prompts: Xelu's Free Controller & Key Prompts (CC0)
- Banner lettering: Chakra Petch by Cadson Demak (SIL Open Font License)

### Contributors

Code:

- [akirahayabusa](https://github.com/akirahayabusa): the freeze in scenes with the sun (#40), folders
  with non-ASCII names on Windows (#41), the Difficulty Selection highlight (#42)
- [ManemanFTW](https://github.com/ManemanFTW) (Manuel Flores): the Windows start-up hang (#9) and the
  audio crackling on Windows (#34)
- [HammerD72](https://github.com/HammerD72): the audio timing fix (#32); the two spots where the game
  picks the camera view at a load, for Start in Third Person (#23); impact sparks at 21:9 (#65); Smoother
  Glow (#64)
- [retro-meister](https://github.com/retro-meister): the macOS port (#49), and the community macOS builds

Testing, saves and traces:

- [Radarzo](https://github.com/Radarzo), [hj-0](https://github.com/hj-0),
  [hyperspeedgx](https://github.com/hyperspeedgx), [serguntchik](https://github.com/serguntchik),
  [PaulAndrewClift](https://github.com/PaulAndrewClift) and
  [phantuannghia95-rgb](https://github.com/phantuannghia95-rgb): seven test builds, screenshots and logs
  on Nvidia cards that tracked down the banded water (#16)
- [HammerD72](https://github.com/HammerD72): testing on Nvidia across many issues (shadows #53, water
  #16, menus and controls)
- [DAVID106-debug](https://github.com/DAVID106-debug): the missing particles and the Xbox comparison
  (#51), the high-resolution shadow flicker (#53)
- [hyperspeedgx](https://github.com/hyperspeedgx): saves across chapters 1 to 9, traces for the torch
  embers (#51) and the Chapter 5 roof (#29)
- [hj-0](https://github.com/hj-0): saves for chapters 11 to 13 and a guide to moving saves from xemu
  (#61)
- [yoyooo941-sys](https://github.com/yoyooo941-sys): the save that made the volcano strip reproducible
  (#55)
- [firepro1211](https://github.com/firepro1211) and
  [huokunlongCHAIN](https://github.com/huokunlongCHAIN): the Chapter 5 roof, including a comparison on
  the Xbox One (#29)
- [matitone71](https://github.com/matitone71): the Steam Deck Game Mode log that found the launch
  problem (#11)

To report a bug, test a build or send a change, see [CONTRIBUTING.md](CONTRIBUTING.md).

*Ninja Gaiden* is a trademark of Koei Tecmo. This project is not affiliated with or endorsed by Koei
Tecmo or Team Ninja.

## License

This project's code is free software: you can redistribute it and/or modify it under the terms of
the GNU General Public License as published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version (`GPL-3.0-or-later`; the text is in [LICENSE](LICENSE)).
Up to 0.9.24 it was GPL-2.0-or-later. It is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY.
The parts it is built from keep their own licenses: see [THIRD_PARTY.md](THIRD_PARTY.md).
