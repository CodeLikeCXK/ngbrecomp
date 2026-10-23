# Ninja Gaiden Black — technical notes

How the recompilation works and how its PC features are built, for whoever works on it. The
player-facing summary is the README; how to build and test is in AGENTS.md.

## How it runs

The game's x86 code (`default.xbe`) is disassembled and lifted to C by
[xboxrecomp](https://github.com/sp00nznet/xboxrecomp)'s pipeline (`tools/pipeline.sh`: about
11.5k functions, 2.2M lines, written to `src/recomp/gen/`, never committed -- it is made from the
player's own disc) and compiled for the host. Nothing is emulated at the CPU level. The rest of the
console is xboxrecomp's runtime:

- **Kernel**: the Xbox kernel's services (threads, files, memory, the hard disk's partitions as
  sparse image files) on the host's.
- **One virtual CPU** (`platform/vcpu.h`): the Xbox has a single CPU and a priority scheduler, and
  the game depends on it (a thread made ready above the running one runs at once). Guest threads
  share one token; waits give it up; a higher-priority thread preempts the holder -- on Linux with a
  signal, on Windows by suspending it and redirecting it into a yield (`vcpu_win_trampoline`).
- **GPU**: xemu's NV2A model runs the game's command stream (fences, vertical blanks, flips); the
  drawing is done by a renderer of the game's Direct3D calls on Vulkan (below).
- **Audio**: the MCPX APU's voice processor (from xemu) mixing into SDL2's output.
- **Input**: SDL2 controllers, keyboard and mouse, as the Xbox's USB pad through an OHCI model.

The arcade Ninja Gaiden (#18) is another program on the disc that the Xbox reboots into
(`XLaunchNewImage`): `NinjaTrilogy.xbe`, or `NinjaTomoe.xbe` (the Japanese build, chosen by the game's
region flag 0xDB9890). Each is lifted the same way (`tools/pipeline_trilogy.sh [tomoe]`, into
`src/recomp/gen_trilogy` / `gen_tomoe`) and built as its own program from `src/trilogy/main.c`, drawn by
the GPU model's own NV2A renderer. `src/launch.c` is the reboot: the runtime hands it the launch data
page, which it saves and passes on (`RECOMP_LAUNCH_PAGE`), then it starts the other program in place
of this one -- exec on Linux and macOS, a new process on Windows -- with the window's current mode.
The arcade is a 68000 emulator written in x86 that keeps the 68000's flags with setcc/daa/das and 16-bit
mul/imul, which is what made the recompiler's overflow flag, decimal adjust and narrow multiplies matter.

`kill -USR1 <pid>` (Linux) prints the guest thread's host backtrace, every lifted function a named
`sub_XXXXXXXX`, and its guest registers -- use it instead of gdb, which swallows the TF single-steps
the APU and AC97 register traps depend on.

## Windows

The same code, cross-compiled with llvm-mingw against MSYS2 CLANG64 libraries
(`packaging/windows/build.sh`, `cmake/toolchains/windows-x64-llvm-mingw.cmake`). It uses the same
host backends as Linux (`XBOXRECOMP_SDL_BACKENDS`: SDL2 audio and input, no XInput, XAudio2 or
Media Foundation) and the virtual CPU (`XBOXRECOMP_WIN_VCPU`: `platform/win32_host.c`, and
`platform/win32_vcpu.h` force-included to route the Win32 waits through it). The guest's 4 GB
window is one placeholder reservation (`VirtualAlloc2`) whose regions are mapped in place. Notes:
clang targeting MinGW accepts `__declspec(thread)` but makes a plain global of it (thread-local
storage must be keyed on `_MSC_VER`); `<windows.h>` is in every file there (its `near` and `far`
macros); Windows sleeps in milliseconds, so sub-millisecond waits use a high-resolution waitable
timer (`vcpu_nap_us`). Under Proton, `XDG_CONFIG_HOME` and `SDL_AUDIODRIVER` do not reach the game.

## Presentation

The game's frame clock is the emulated display's 60 Hz vblank. Each picture the renderer finishes
(at the game's Swap, a couple of milliseconds after a vblank) is presented at once from the
renderer's thread (`present_sdl.c`, `nv2a_present_set_external`); the vblank tick presents only what
was not (a loading pause). Presented at the tick, every frame waited a whole refresh; through the
GPU-command thread, presents came out unevenly. V-Sync's Mailbox is mailbox where the system has it
(no tearing, no queue: a FIFO queue fills against the game's own clock and adds 30-50 ms), FIFO on as
few images as allowed where it has not; its FIFO is FIFO (every frame in turn: steadier where
mailbox, like Nvidia's Fast sync, now and then shows a frame twice, #17); Off is immediate. `NV2A_FPS_LOG=1` logs the frame rate and
`[LAT]` lines: each picture's age when presented and the spacing of presents.

## Settings, menus and features

The settings are in the game's own menus, in its style: the main menu's RECOMP SETTINGS
(where Ranking was) opens the Options screen on them -- Graphics, Display and Startup
Settings and Cheats, each a page of rows like Video Settings', and Controls (the bindings' panel,
below), under a SETTINGS header (`src/pc_menu.c`;
the header and category words are drawn in Liberation Serif in the game's pictures' style,
`src/pause_tab.c`) -- and the pause menu has a Settings tab before Exit (A to enter its page,
left/right on the category strip at its top for Graphics, Display, Startup, Cheats or Controls,
up/down for the rows, left/right or A to change one -- or a click: on a value's left half back,
anywhere else on the row on -- B to leave; L and R go on to the next tab, as on the
game's own pages; `NGB_NO_SETTINGS_TAB=1` leaves the game's six tabs). They are: aspect ratio
(4:3, the game's own 4:3 mode; 16:9, its widescreen; 16:10, widescreen with the 3D drawn for
16:10; it applies at once: the 3D view's projection, the presenter and the window, whose sizes
follow the shape -- only the paused picture behind the pause menu keeps the old shape until
play resumes. The game has one layout for its HUD and menus, drawn for 4:3: in 16:9 and 16:10
they are stretched sideways, as the console did on a widescreen TV), window size (the sizes offered match the
aspect ratio), windowed/fullscreen/borderless (a frameless window over the whole display),
render resolution (1x-10x), anisotropic filtering (Off, the console's own filtering; 2x, 4x,
8x or 16x: trilinear blending between mipmaps and anisotropic filtering at that level, and
mipmaps made for the many textures the game gives none -- DXT ones decoded for it -- which stops
rock, wood and gold-leaf textures sparkling and crawling in the distance as the camera moves), anti-aliasing (MSAA Off, 2x, 4x
or 8x: the display-sized targets drawn multisampled and resolved as each pass ends; with the
new renderer only), shadow resolution (Ryu's shadow map at 2x, 4x, 8x or 10x the original
128x128, for smooth instead of blocky edges), V-Sync (Off, Mailbox, FIFO, or Adaptive:
synced, but a late frame is shown at once instead of waiting a refresh -- Vulkan's FIFO relaxed), an FPS
counter, and skipping the intros (the boot movie, which the game itself will not let you skip;
the New Game prologue and the story movies play, and Start, A or B ends any of them; from the
next start). They are saved to `~/.config/NinjaGaidenBlack/settings.ini` as they change.
`--settings` shows the old settings screen before the game.

All Difficulties (Startup, saved): New Game offers every difficulty, Ninja Dog to Master Ninja,
without clearing the one before (or dying for Ninja Dog). The game's menus ask one function (sub_001DB520, now in
`src/cheats.c`) for their unlock flags -- 7-9 are those three, read from the system data --
and with the setting on it answers yes for them; nothing is written, so the player's own record
is untouched and turning it off puts back what they have earned. Off, Ninja Dog is as the game
has it: offered after dying in chapter 1.

Cheats, for getting past a hard spot (`src/cheats.c`, applied each frame; for the session
only, never saved, so a new start has them off): Infinite Health (Ryu's health refilled each
frame -- the game keeps it per segment at 0x58A534, maxima at 0x58A524, and 0x58A566 is their
sum; a single blow taking all of it at once still kills), Infinite Ninpo (0x58A56C/0x58A56E),
Infinite Essence (0x589570, the game's cap of 99,999) and Golden Scarabs (enough in the bag --
inventory slots at 0x5891D0, item 0x59 -- to make 50 with those already given to Muramasa,
0x589574: every one of his rewards at the next visit). The game guards its state with 24
checksums (sub_00164E50) and returns to the title when one changes without the flag its own
code sets (0x7F1A94 + n); a cheat that writes sets them all. Startup Settings also has All
Costumes (saved): the Costume Selection at New Game offers every costume, as after clearing
the game, through the same flag function as All Difficulties (flags 1-4). The game has eight costumes (chr_ryu.afs, chr_ryua-g.afs) and its Costume Selection
lists five (0 and 4-7, the table at 0x3E9D10), the locked ones as white shapes; here it lists only
those unlocked (`src/costume_menu.c`), and with All Costumes on costumes 1-3 too, after the game's five:
the game has no pictures of them, so each is its default costume's picture as a silhouette with
the name across it (EXTRA 1-3, made at run time from the player's copy, `src/pause_tab.c`).
Debugging aids there: `NGB_CHEATS=111` turns them on at start, `NGB_POKE=addr:bytes:value,...`
writes guest memory every frame (only while the file `NGB_POKE_GATE` names exists, if set) and
`NGB_MEMDUMP=<dir>` dumps the 64 MB of guest memory each time `<dir>/dump.now` appears.

Back from OPTIONS or RECOMP SETTINGS the main menu's cursor returns to the entry chosen (the
game puts it where OPTIONS was before it moved, the last entry: now EXIT TO DESKTOP). With All
Difficulties' Ninja Dog the difficulty selection has five rows, a case the game never shows
there: they are spaced closer (38 instead of 44) so the first clears the header.

On a wide screen (16:9, 16:10, 21:9) the game is in its own widescreen mode the whole time, with
its 3D drawn for the screen's shape (both of its aspect constants, 0x414AF4 and 0x414AF8, are that
shape): every menu, the HUD, chapter cards and clear screens are the game's own 16:9 ones, drawn as it
draws them for a widescreen TV, kept 16:9 (`src/d3d/vk/backend.cpp`): at 21:9 narrowed to the middle
16:9, at 16:10 shortened to it, a 2D screen or menu whole there with bars beyond (at the sides at
21:9, above and below at 16:10). The bars are filled with the screen's own colours: each side's edge,
16 bands of it, sampled over the screen's first 2 s and then kept still, so a menu's animation does
not move them (`EdgeSample`; `NGB_BAR_FILL=0` makes them black). A menu drawn over the paused game
(the save statue's) has the paused game around it instead. Which frames are 2D screens: a menu
running in the frame, or (the frame before was one) no 3D drawn in this frame so far. In play the 3D fills the screen and the HUD keeps its 16:9 shape but goes
to the screen's corners and sides: each frame its pieces are grouped into elements (pieces within a few
pixels of each other -- a gauge with its caps and the words under it, the health bar with the karma, a
prompt with its icon, a line of subtitles), and each element keeps to the nearer side (or the middle,
when it is about the middle), to the top when it starts near the top (the health bar, the karma
window opened below it with it), the bottom when it ends near the bottom, else the middle; only what
is drawn after the frame's 3D counts (the 3D's own screen-space mist and spray are left as drawn), and
screen effects in play -- anything covering most of the screen or reading a render target: the
underwater tint, flashes, the bloom -- fill the whole screen as drawn (`HudGroup`, `HudPlace`;
`NGB_HUD_GROUP_LOG=1` logs them, `NGB_HUD_CENTER=1` keeps the whole HUD in the middle 16:9). A screen
of panels drawn over the game (a chapter clear: an element about the middle of the screen, over a third
of its width and nearly half its height, across its middle -- an area's name is not one -- for 4 frames
running; over after 8 without) is kept whole in
the middle, as a menu: what is inside the panel's box goes with it, the HUD around it stays where it is. At 4:3 the game is in its 4:3 mode and everything is its 4:3 picture.
(Earlier versions kept the game in its 4:3 mode in play and narrowed its HUD to its 4:3 shape,
switching modes for the menus; the screens it could not tell apart in time -- item pickups, chapter
clears, leaving the save menu -- came out half one way and half the other. `NGB_HUD_FIXED=1` brings
that back, for comparison.) Movies play in the widescreen mode too, where the game follows its own Movie Display
Ratio (Options, Video Settings): 16:9 across the screen, or 4:3 with bars at the sides. (Switching to
its 4:3 mode for a 4:3 movie narrowed it twice, #28.) The mouse is mapped to the menus the same way.
`NGB_HUD_RATIO_LOG=1` logs the 2D draws not drawn texel for pixel; `NGB_MODE_LOG=1` the mode each
frame.
Aspect ratio also offers 21:9 (64:27;
window sizes 1720x720 to 5120x2160): the 3D widened as for 16:10, the game deciding what to
draw for a narrower view, so objects at the far edges may appear late.

The boot "Warnings" screen is always skipped (`NGB_KEEP_WARNINGS=1` shows it).

For glitches that only show in play: F7 records the next 300 frames to `~/ngb-trace-<n>/` -- a
small picture of each frame and a line per draw (its state: targets, textures, combiner stages,
blending, fog, vertex colour, clears) in `draws.txt` (`NGB_TRACE_FRAMES=<n>` frames;
`NGB_TRACE_PRE=1` only the screen-space draws and the first draw after each target change;
`NGB_TRACE_NOPIC=1` no pictures, `=copy` pictures only around a copy into a 512x512 target, the
cut cross-fade's). F8 appends Ryu's position to `ngb_positions.txt`
in the save directory.

The pause menu's Exit tab has a page of its own: Return to Game (the game's Exit: the menu
closes), Return to Main Menu and Exit to Desktop, the last two after an "Are you sure?"
Yes/No box in the game's menus' style (a near-black panel between gold rules, the game's own
letters, the chosen answer white in the violet haze of its menu items). The main menu's last entry is EXIT TO DESKTOP, behind the same box (OPTIONS moved
up into the never-shown COMING ATTRACTIONS' place). The game has no way back to its title
from play but the console's reset, so Return to Main Menu starts the program again in the
same process (Steam and the like see one game running throughout), skips the intros and
presses Start at the title; the window closes and opens again on the way.

F11 or Alt+Enter toggles fullscreen and
F10 the FPS counter (frames per second, average and worst frame time over the last half
second). Compiled shaders are kept in `~/.cache/NinjaGaidenBlack/shaders` (the new renderer's in its
`d3d` folder: how each shader is made, its SPIR-V and the pipelines), and all of them are made
again in the background at the next start, before the game needs them; a seed shipped with the
AppImage (`packaging/d3d-seed.bin`, recorded over the first chapters; the game's vertex programs in
it are only places in the player's own `default.xbe`) does the same on the very first start. So
an effect does not stutter the frame it first appears. The AppImage also carries the pipelines
recorded while testing (`packaging/shader-seed`), built in the background at startup;
the game's vertex programs are not in it, only where each is in `default.xbe`, and they are
read from the player's own copy (`tools/strip_shader_seed.py` makes the seed from a
recorded shader cache).

(The main menu's Video Settings page has no PC row: the settings are under RECOMP SETTINGS.)

Keyboard and mouse: whichever of them or a controller was used last is the one the game listens
to (touch the other and it switches; xboxrecomp's input). The keys by default (`src/keyboard.c`;
the game's default actions in brackets):

| Keys | Pad | | Keys | Pad |
|---|---|---|---|---|
| W A S D | left stick (move) | | mouse, numpad 8 4 5 6 | right stick |
| left mouse, J | X (attack) | | right mouse, K | Y (special attack) |
| Space | A (jump) | | E | B (projectile) |
| F | Y+B (ninpo) | | C | X+A (wind run) |
| Shift | L (guard) | | Q, middle mouse | R (center camera) |
| Tab | Black (map) | | R | White (karma window) |
| Esc, Enter | START | | Backspace | BACK |
| arrows | d-pad | | Z / V | stick clicks |

Controls (the Settings tab's Controls category and RECOMP SETTINGS' CONTROLS; the game's own
Options > Controller Settings > Button Settings is left as the game has it, for the controller)
changes them: each action in play has two keys or mouse
buttons and a controller button; A (Enter, a click) waits for the new one -- Esc cancels; for the
controller, its next button, or six seconds -- and X (Delete) clears it. A key taken from another
action leaves it; a controller button taken from another action swaps with it. They are kept in
`controls.ini` beside `settings.ini` (`src/bindings.c`); in play the keyboard and mouse press the
pad as bound and the controller's buttons are moved to their actions (`xbox_pad_remap`, weak in
xboxrecomp's input). The sticks are not moved; the menus keep their own controls, the controller
as it is, so no binding can lock them up. Play's key prompts follow the bindings.
`NGB_FIXED_KEYS=1` keeps the keys above, `NGB_NO_PAD_REMAP=1` the controller as it is,
`NGB_CONTROLS_IN_OPTIONS=1` puts the panel over the game's Button Settings too.

In the menus Esc is B (back), Q and E are L and R (the pause menu's tabs) and the mouse points;
in play the mouse is captured and its motion is the right stick. The pointer is shown only in the
menus while the keyboard and mouse are in use. While the keyboard is in use the
game's button prompts are keys and mouse buttons instead -- Xelu's dark keyboard and mouse prompts
(CC0, `assets/prompts/keys`, the same artist's as the controller buttons; a key the set has no picture
of, its blank key with the key's name; `src/pause_tab.c`), in play the ones bound to each action, in
the menus the mouse's buttons (left to choose, right to go back), K, Space, Shift and Q/E;
`NGB_PAD_PROMPTS=1` keeps the pad's.

In the game's menus (main menu, Options, the pause menu and its tabs, message boxes) the mouse
points: the game's own cursor follows it onto the item under it (a press of the pad at a time,
so the game moves it as it would), a click there chooses it, a right click is B, the wheel
up/down, the side buttons the triggers. While a controller is the device in use it does
nothing in the menus; moving it makes the keyboard and mouse the device in use again. (`NGB_MOUSE_LOG=1` shows what it finds.)

The button prompts ("Select", "Back") can be clicked: the button they show. Esc is B (back) in
the game's menus.

The main menu has RECOMP SETTINGS in Ranking's place (Xbox Live's scoreboards, gone; choosing it
sent the game to the dashboard's account setup, which closed the program) and Options > General Settings no Flag
Select (the country the scoreboards wanted) -- `src/main_menu.c`; `NGB_KEEP_RANKING=1` keeps
both.

Total Play Time: saves made by builds before the clock fix hold the 999:59 cap (the PC's uptime
was counted); such a total is started again from 0 when loaded (`src/playtime.c`).

The environment still overrides the file, for testing: `NGB_WIDESCREEN=0|1`,
`NGB_FULLSCREEN=0|1`, `NGB_VSYNC=0|1`, `NV2A_SCALE=n`, `NV2A_SHOW_FPS=1`,
`NGB_NO_SETTINGS=1` (skip the screen), `NGB_AUDIO=0` (mute). `NV2A_FPS_LOG=1` prints the
FPS counter to stderr, and `NV2A_FPS_LOG=/path/to/file` appends it to that file (on a Steam Deck:
`NV2A_FPS_LOG=/home/deck/ngb-fps.txt %command%` as the launch options). `NV2A_EARLY_PRESENT=1` shows a finished frame before the title flips to it
(a refresh less latency); off by default, because on a busy frame it made the rate fall to 40-50.
`NV2A_REFRESH_HZ=n` runs the emulated display at n Hz (testing only: the game steps its logic
once a frame, so at 120 it plays at double speed).

## The renderer

The game is drawn by a renderer that implements its Direct3D calls on Vulkan (`src/d3d`,
`docs/renderer.md`; its shader translation, vertex and texture code come from TGP482's
[SCDA-XBERecomp](https://github.com/TGP482/SCDA-XBERecomp) /
[Xbox-XBERecomp](https://github.com/TGP482/Xbox-XBERecomp), MIT). xemu's NV2A
emulation still runs the GPU's command stream (the game's fences and vertical blanks come from
it) but draws nothing; the renderer draws on the same Vulkan device and the same window, so the
settings and the FPS counter work as before. Render resolution, texture filtering
and shadow resolution apply to it. It needs Vulkan 1.3 (dynamic rendering, push descriptors);
without them the game falls back to xemu's drawing.

`NGB_RENDERER=xemu` draws with xemu's NV2A renderer instead, as before (on Steam:
`NGB_RENDERER=xemu %command%`). `NGB_RENDERER=mirror` keeps xemu drawing and showing the game
and renders with the new one unseen, for comparing (`NGB_D3D_DUMP=dir` writes its frames).

## Bring-up tools

- `kill -USR1 <pid>` prints the guest thread's host backtrace (every lifted function
  is a named `sub_XXXXXXXX`) and its guest registers. It needs no ptrace, so use it
  instead of gdb: gdb swallows the TF single-steps that the register traps depend on.
- `RECOMP_WATCHDOG_SECS=N` dumps guest state and exits after N seconds.
- `tools/feedback.sh [secs]` runs an instrumented build, records every
  indirect-branch target it reaches, reseeds function detection and regenerates.
  Run it after the game reaches new code.
- `XBOX_LOG_LEVEL=4` and `RECOMP_KERNEL_LOG_BUDGET=100000` give a full kernel and
  file trace.

The defaults in `bringup_defaults()` (`src/main.c`) stand in for hardware that isn't
modelled yet. Each one should disappear when the real thing exists.
