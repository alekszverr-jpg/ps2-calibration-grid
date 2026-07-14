# PS2 Calibration Grid

A standalone calibration-pattern generator for PlayStation 2. It produces a
single `PS2Grid.elf` and does not need external images or data files.

[Russian documentation / Документация на русском](README_RU.md)

## Download and run

Download `PS2Grid.elf` from the latest GitHub Release, copy it to a USB drive,
and launch it with wLaunchELF/uLaunchELF. It can also be registered as an OPL
application.

The default mode is PAL 50 Hz at 640x512i. Press **Triangle** to switch between
PAL 50 Hz and NTSC 60 Hz (640x448i).

## Patterns

- 32-pixel geometry grid, centre axes, and circular references
- 100%, 95%, 90%, 85%, and 80% overscan frames
- grayscale steps and PLUGE black-level fields
- 75% colour bars
- 16x16 checkerboard

## Controls

- **Left / Right**: previous or next pattern
- **Cross**: next pattern
- **Triangle**: switch PAL/NTSC
- **Select**: show or hide the on-screen help

## Build

The included GitHub Actions workflow builds and validates the ELF with the
official `ps2dev/ps2dev` container. A local ps2dev environment with PS2SDK and
gsKit can also build it by running `make`.

## Project status

The ELF is compiled and structurally validated by the current PS2 toolchain.
It has also been successfully launched and tested on the project owner's
physical PlayStation 2. Compatibility with every console revision, cable type,
and display is not guaranteed; please include those details when opening an
issue.

## Safety

Do not leave a static pattern displayed for a long time on plasma or OLED
panels. Record the original values before changing a television's service-menu
settings.

Released under the [MIT License](LICENSE).
