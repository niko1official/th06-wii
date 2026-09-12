[![Discord][discord-badge]][discord] <- click here to join discord server.

[discord]: https://discord.gg/VyGwAjrh9a
[discord-badge]: https://img.shields.io/discord/1147558514840064030?color=%237289DA&logo=discord&logoColor=%23FFFFFF

This is the readme for the portable fork of EoSD. For the readme of the decomp project, see [here](https://github.com/GensokyoClub/th06/blob/master/README.md).

EoSD-portable is a port of Touhou 6 using SDL2 and OpenGL (with a more general renderer abstraction layer hopefully on the way).
This enables theoretical portability to any system supported by SDL2, with Linux, Windows, and macOS in particular being known to work.
Builds for the BSDs and other Unices are also almost certainly possible, but may require some slight modifications to the build system.

### Platform Requirements

- SDL2, SDL2-image, and SDL2-ttf support
- C++20 standard library support
- A little endian architecture (though big endian support is currently being worked on)
- OpenGL ES 1.1, OpenGL 1.3, or GL 2.1 / GL ES 2.0 / WebGL support

### Dependencies

EoSD-portable has the following dependencies:

- `SDL2`
- `SDL2_image`
- `SDL2_ttf`
- `libasound` (Optional and Linux-only, enables MIDI support. This will almost always be present as part of a desktop distro.)

On Windows and macOS, MIDI support uses the system APIs and needs no extra dependencies.

In addition, building uses [`premake5`](https://premake.github.io/download) and a compiler that supports C++20.

#### Building

In the repository root directory, run `premake5` with the desired build system as an argument (a list can be seen by running `premake5 --help`).
This will output the build files to the `build` directory, and then compilation may be done with the desired build system.

##### Build Options (Use with Premake Invocation)
`--no-asoundlib`: On Linux, doesn't build MIDI support. Removes libasound as a dev and runtime dependency
`--use-c23-embed`: Uses `#embed` for resource inclusion instead of a lua script in the Premake file.

##### Build Example (Debian-based Linux)

Obtain dependencies:

`sudo apt install build-essential libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libasound2-dev`

Generate makefile:

`premake5 gmake`

Compile:

`cd build && make -j16`

##### Build Example (macOS)

Install the Xcode Command Line Tools (provides the compiler), if not already present:

`xcode-select --install`

Obtain dependencies (using [Homebrew](https://brew.sh)):

`brew install premake sdl2 sdl2_image sdl2_ttf`

Generate makefile:

`premake5 gmake`

Compile:

`cd build && make -j16`

### Use

EoSD-portable is designed to be a drop-in replacement for the vanilla EoSD binary.
You will also need to add a font to your game directory with the filename `msgothic.ttc`.
This may be the actual MS Gothic, taken from a Windows machine, or a compatible font such as Kochi Gothic.
EoSD-portable uses the Japanese filenames (e.g. 紅魔郷CM.DAT, 東方紅魔郷.cfg). English and other patches, static or thcrap, do not currently work.
A Japanese locale is not required.

#### Wii / Homebrew Channel

Build with a devkitPro installation containing `wii-dev` and `wii-sdl-libs`:

`make -f Makefile.wii -j4`

Copy `build-wii/th06.dol` to `sd:/apps/th06/boot.dol`. Put `meta.xml`,
`icon.png`, the original `bgm` directory, and the six original Japanese data
archives in that same `sd:/apps/th06` directory:

- `紅魔郷CM.DAT`
- `紅魔郷ED.DAT`
- `紅魔郷IN.DAT`
- `紅魔郷MD.DAT`
- `紅魔郷ST.DAT`
- `紅魔郷TL.DAT`

Also provide either `msgothic.ttc` or a complete, valid
`NotoSansJP-Regular.ttf` there. A tiny placeholder/download-stub file is not a
font and SDL_ttf will reject it. The unprefixed archives (`CM.DAT`, etc.),
`th06e_*.DAT`, and `boot.elf` are not used by this build. A separate `data`
directory is not required for graphics.
The Wii launcher changes to the directory containing `boot.dol`, so relative
asset lookup remains reliable under Homebrew Channel and compatible loaders.

The default sideways-Wiimote controls are:

- D-pad: movement (mapped for a sideways Wiimote)
- 2: shoot/select
- 1: focus/slow movement
- A: bomb/back
- B: skip/fast-forward dialogue
- +: pause/menu
- HOME: Homebrew Channel exit

For Dolphin performance testing, leave framebuffer dumping and verbose SD/IOS
logging disabled. Frame dumping PNG-compresses and writes every 640x480 frame
and can make an otherwise full-speed build appear to run at 10-30 FPS.

The Wii renderer caches static menu/background surfaces after their first GPU
upload. Re-uploading the 1024x512 backing texture every frame was the cause of
the severe real-hardware title-menu slowdown.

# Decomp Credits

We would like to extend our thanks to the following individuals for their
invaluable contributions:

- @EstexNT for porting the [`var_order` pragma](scripts/pragma_var_order.cpp) to
  MSVC7.
