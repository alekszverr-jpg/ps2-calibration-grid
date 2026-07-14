# PS2 Calibration Grid

A standalone calibration-pattern generator for PlayStation 2. It produces a
single `PS2Grid.elf` and does not need external images or data files.

[Russian documentation / Документация на русском](README_RU.md)

## Download and run

Download `PS2Grid.elf` from the latest GitHub Release, copy it to a USB drive,
and launch it with wLaunchELF/uLaunchELF. It can also be registered as an OPL
application.

The initial region is detected from the console ROM. PAL consoles start in
576i/50 and NTSC consoles start in 480i/60.

Available output modes are PAL 576i, PAL 288p, NTSC 480i, and NTSC 240p. Press
**Triangle** to preview the next mode, then **Cross** to keep it or **Circle**
to go back. If no choice is made, the previous mode is restored automatically
after ten seconds.

## Patterns

- 32-pixel geometry grid, centre axes, and circular references
- 100%, 95%, 90%, 85%, and 80% overscan frames
- grayscale steps and PLUGE black-level fields
- 75% colour bars
- output-pixel-accurate 16x16 checkerboard
- convergence crosshatch and cross markers
- one- and two-pixel vertical and horizontal sharpness fields
- colour-bleed and chroma alignment bars
- full-screen white, gray, red, green, and blue fields

## Controls

- **Left / Right**: previous or next pattern
- **Cross**: next pattern
- **Square**: open or close the pattern menu
- **Up / Down**, then **Cross**: select and open a pattern from the menu
- **Circle**: close the pattern menu without changing the pattern
- **Triangle**: preview the next video mode
- **Cross / Circle** during mode preview: keep or restore the mode
- **Select**: show or hide the on-screen help
- **Start + Select**: exit to the PS2 Browser

## Build

The included GitHub Actions workflow builds and validates the ELF with the
official `ps2dev/ps2dev` container. A local ps2dev environment with PS2SDK and
gsKit can also build it by running `make`.

## Project status

Version 1.2 has been successfully tested by the project owner on a physical
PlayStation 2 connected to a JVC AV-2130SE CRT television. All 13 patterns, the
quick pattern menu, PAL 576i, PAL 288p, NTSC 480i, NTSC 240p, safe automatic
and manual mode rollback, and the exit command were verified. Compatibility
with every console revision, cable type, and display is not guaranteed; please
include those details when opening an issue.

## Safety

Do not leave a static pattern displayed for a long time on plasma or OLED
panels. Do not leave the full-white field on a CRT for an extended period.
Record the original values before changing a television's service-menu settings.

Released under the [MIT License](LICENSE).
