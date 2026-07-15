# Changelog

All notable changes to PS2 CRT Test Suite are documented here.

## [1.3] - 2026-07-15

- Added Scroll Grid with six selectable speeds and both directions.
- Added moving Drop Shadow, adjustable Flicker, and camera-friendly Frame Timer
  patterns.
- Added pause/resume control for all dynamic patterns.
- Expanded the pattern menu to two columns for 17 total patterns.
- Kept all four safe SD modes and their rollback behaviour unchanged.

## [1.2.2] - 2026-07-14

- Updated the cloud build to `actions/checkout@v7` and
  `actions/upload-artifact@v7`, both using Node.js 24.
- Renamed the workflow and build artifact for the current project branding.
- Added this changelog and structured GitHub issue forms for bug and hardware
  compatibility reports.
- Kept the calibration patterns and video-mode implementation unchanged.

## [1.2.1] - 2026-07-14

- Renamed the project to PS2 CRT Test Suite.
- Updated the repository description, search topics, documentation, and
  in-application title.
- Retained the `PS2Grid.elf` filename for OPL and launcher compatibility.

## [1.2] - 2026-07-14

- Added a quick pattern-selection menu.
- Expanded the suite to 13 patterns with convergence, pixel-accurate sharpness,
  colour-bleed, checkerboard, and full-field tests.
- Verified the new menu and patterns on a physical PlayStation 2 connected to a
  JVC AV-2130SE CRT television.

## [1.1] - 2026-07-14

- Added PAL 576i/288p and NTSC 480i/240p modes with automatic region detection.
- Added safe ten-second video-mode preview with manual and automatic rollback.
- Added `Start + Select` exit support.

## [1.0] - 2026-07-14

- Initial public release with five calibration patterns.

[1.3]: https://github.com/alekszverr-jpg/ps2-crt-test-suite/compare/v1.2.2...v1.3
[1.2.2]: https://github.com/alekszverr-jpg/ps2-crt-test-suite/compare/v1.2.1...v1.2.2
[1.2.1]: https://github.com/alekszverr-jpg/ps2-crt-test-suite/compare/v1.2...v1.2.1
[1.2]: https://github.com/alekszverr-jpg/ps2-crt-test-suite/compare/v1.1...v1.2
[1.1]: https://github.com/alekszverr-jpg/ps2-crt-test-suite/compare/v1.0...v1.1
[1.0]: https://github.com/alekszverr-jpg/ps2-crt-test-suite/releases/tag/v1.0
