# Contributing

Bug reports, test results, saves and code are all welcome. This page says what helps most.

Never attach or commit game files: disc images, `.xbe`, `.afs` or other files from the disc,
the generated code in `src/recomp/gen/`, or shaders captured from the game.

## Reporting a bug

Check the [known issues](README.md#known-issues) first, then open an issue with the **Bug report**
template. Include:

- the version (or the commit, if you built it), your GPU and driver, and your settings (render
  resolution, anti-aliasing, aspect ratio);
- where in the game it happens (chapter and place, or the menu) and what you did just before;
- for a **graphics glitch**: press **F7** while it is on screen and attach the `ngb-trace-<n>` folder
  it writes to your home folder (`C:\Users\<you>` on Windows), zipped. It shows exactly what was drawn;
- for a **crash, freeze or a problem starting**: the log. On Windows, macOS, and Linux started without a
  terminal, that's `log.txt` beside the saves ([where things are kept](README.md#where-things-are-kept));
  on Linux started from a terminal, it's the terminal's output;
- if it only happens in one place: a save from just before it (the zipped `TitleData` folder from the
  saves folder).

To tell our bug from the game's own behaviour: launch with `NGB_RENDERER=xemu` (xemu's
hardware-accurate renderer, slower), and compare with footage of the Xbox if you can find it. If the
xemu renderer shows the same thing, say so in the report.

## Suggesting a feature

Open an issue and describe what you want and why. Changes that make the game look or play
differently from the Xbox are added as options, off by default.

## Testing builds

Test builds are posted in issues. When you report back, include your GPU, driver, operating system and
settings, and what you tried: "fixed", "not fixed" and "different" are all useful. A screenshot of
the same spot before and after helps most.

## Code

Pull requests are welcome. A zip with a patch, attached to an issue, works too.

- Build from source as described in the [README](README.md#building-from-source). [AGENTS.md](AGENTS.md)
  has the project layout, the debugging tools, and the rules for test runs: scratch copies of saves and
  settings, no sound, a headless window.
- Keep a pull request to one fix or feature, and say in it how you tested it (GPU, settings, where in the
  game).
- **Fixes** that make the port match the Xbox can be on by default. Check them against the xemu
  renderer or real hardware.
- **Enhancements** that change the original look or behaviour go behind a setting, off by default. An
  environment variable is fine for a start; it can become a menu option once tested.
- For a change to a lifted game function: take it out of the generated code with
  `tools/take_function.py` and list it in `src/manual_functions.json`. Don't edit `src/recomp/gen/`;
  it isn't committed and is regenerated from each person's disc.
- Commit messages start with the area (`Renderer:`, `Controls:`, `Linux:`, ...) and name the issue,
  for example `Renderer: impact sparks keep their 16:9 thickness at 21:9 (#65)`.

The project is licensed GPL-3.0-or-later, and contributions are accepted under the same license.
Contributors are credited in the [README](README.md#contributors) and in the release notes.
