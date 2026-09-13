[![Discord][discord-badge]][discord] <- click here to join the GensokyoClub discord server.

[discord]: https://discord.gg/VyGwAjrh9a
[discord-badge]: https://img.shields.io/discord/1147558514840064030?color=%237289DA&logo=discord&logoColor=%23FFFFFF

# CREDITS
This is a project very heavily based on the portable decompilation of EoSD, made by GensokyoClub. You can find their work [here](https://github.com/GensokyoClub/th06).
Please make sure to give them support in their project.

# Installation

If you have installed the main files from the Release, you will need a few things before moving on. 
First of all, you will need to add a font to your app directory with the file name `msgothic.ttc`.
This may be the actual MS Gothic, taken from a Windows machine, or a compatible font such as Kochi Gothic.
This also uses the Japanese EoSD filenames (e.g. 紅魔郷CM.DAT, 東方紅魔郷.cfg), you will need to provide your own files from your own EoSD copy.
English and other patches, do not work.
Lastly, you will need to add the `bgm` folder from your EoSD copy too.

# Building
### Requirements
- `MSYS2 (for windows)`
- `devkitPPC`
- `libogc and libfat`
- `GNU Make and Wii build rules`
- `Wii SDL2, Wii SDL2_image, Wii SDL2_ttf`
- `FreeType, HarfBuzz, libpng, libjpeg-turbo, zlib, bzip2, Brotli`

Midi has been removed from this repo, so you do not need to get the packages for that unless you want to edit the repository to add support for MIDI.

##### Building

In the repository root directory, run `make -f Makefile.wii -j4`, then `make -f Makefile.wii clean` to make sure there aren't any leftovers that may have been there by accident.
Finally, you can run `make -f Makefile.wii V=1` to have visible logs. Note that there will be a lot of warnings, these are normal and shouldn't cause any problems.
This process can most likely be reduced if you use `premake5`, but atleast for my own case I have used MSYS2. If you have more knowledge on this, please feel free to make a fork or pull request to this README.

##### Build Example (Windows System)

Obtain dependencies:

`pacman -Syu`
`pacman -S --needed wii-dev wii-sdl2-libs`

Generate makefile:

`make -f Makefile.wii -j4`

Compile:

`make -f Makefile.wii V=1`

