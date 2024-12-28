# Lunatic Vibes F

Lunatic Vibes F (LVF) is a BMS client - a rhythm game that plays community-made charts in BMS format.

The project is still in development stage. Please do not expect a bug-free experience. Feel free to open issues.

It is a clone of Lunatic Rave 2 (beta3 100201).

It is also a fork of [Lunatic Vibes](https://github.com/yaasdf/lunaticvibes).
Please note that LVF upgrades user profiles such that they can't be used in original Lunatic Vibes anymore.

## Features

- Asynchronous input handling
- Full Unicode support
- Built-in difficulty table support
- Mixed skin resolution support (SD, HD, FHD, UHD)
- ARENA Mode over LAN / VLAN

For LR2 feature compatibility list, check out [the wiki](https://github.com/chown2/lunaticvibesf/wiki/LR2-Features-Compatibility).

## Requirements

- **Do NOT use this application to load unauthorized copyrighted contents (e.g. charts, skins).**
- Supported OS: Windows 7+

## Quick Start

- Install [Microsoft Visual C++ Redistributable 2015+ x64](https://aka.ms/vs/17/release/vc_redist.x64.exe)
- Download the latest release from [here](https://github.com/chown2/lunaticvibesf/releases)
- Copy LR2files folder from LR2 (must include default theme; a fresh copy right from LR2 release is recommended)

## Build

### Windows with Visual Studio

Open the project's directory in Visual Studio, it should pick up CMake and install dependencies with vcpkg
automatically.

### Linux

#### vcpkg

```sh
# export VCPKG_ROOT=/path/to/vcpkg
cmake --preset linux-vcpkg -B ./build
cmake --build ./build --config=Debug
ls build/bin/Debug
```

For additional compilation warnings when compiling with GCC or clang,
change the preset above to `linux-vcpkg-gcc` or `linux-vcpkg-clang` respectively.

#### Using system dependencies

For NixOS, a flake with required dependencies is included.

For other distros, building with system dependencies should be possible, but the exact list is not documented.
Feel free to contribute.

```sh
# NixOS: nix develop
# Other: apt install ...
cmake --preset linux -B ./build
cmake --build ./build --config=Debug
ls build/bin/Debug
```

For additional compilation warnings when compiling with GCC or clang,
change the preset above to `linux-gcc` or `linux-clang` respectively.

## License

- MIT License
