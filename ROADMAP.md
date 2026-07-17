# PS2 CRT Test Suite Roadmap

This roadmap describes the intended direction of the project. It is not a
fixed release schedule: priorities may change after hardware testing and user
feedback.

The current stable release is **v1.3**. Development remains focused on safe SD
video modes, useful measurements, predictable controls, and a single
self-contained ELF.

## Completed milestones

- [x] **v1.0** — initial geometry grid and standalone ELF.
- [x] **v1.1** — PAL/NTSC SD modes, safe mode preview and rollback, and exit to
  the PS2 Browser.
- [x] **v1.2** — expanded static test suite and pattern menu.
- [x] **v1.2.1–v1.2.2** — project rename, documentation, compatibility-report
  forms, and maintenance work.
- [x] **v1.3** — Scroll Grid, Drop Shadow, Flicker, Frame Timer, dynamic
  controls, pause, and the two-column menu.

Version 1.3 was successfully tested on a physical PlayStation 2 connected to a
JVC AV-2130SE CRT television in PAL 576i, PAL 288p, NTSC 480i, and NTSC 240p.

## v1.4 — Interactive calibration

The next main release will turn several fixed patterns into adjustable tools.
The goal is to improve practical calibration value without making the program
harder to operate.

### Adjustable patterns

- [ ] Checkerboard cell sizes: 1, 2, 4, 8, 16, and 32 output pixels.
- [ ] Selectable full-field brightness levels.
- [ ] Adjustable convergence-grid spacing.
- [ ] 75% and 100% colour bars.
- [ ] Additional PLUGE and IRE reference levels.
- [ ] Horizontal and vertical Scroll Grid directions.
- [ ] Drop Shadow speed and background controls.
- [ ] Per-pattern reset to safe defaults.
- [ ] On-screen display of the active parameter values.

Proposed common controls are **Up/Down** for the primary parameter,
**L1/R1** for a secondary parameter, **Start** for pause, and **Circle** for
reset. Controls will remain context-sensitive and will be shown on screen.

### Navigation and help

- [ ] Organize patterns into Geometry, Color, Sharpness, Motion, and Utility
  categories.
- [ ] Add a short purpose and usage description for every pattern.
- [ ] Show a first-use safety warning before the Flicker test.
- [ ] Keep every existing pattern reachable with a controller only.

### Technical foundation

- [ ] Replace the large pattern switch with a descriptor table containing the
  pattern name, category, draw function, update function, and controls.
- [ ] Remove hard-coded dynamic-pattern indices.
- [ ] Store the application version in one central location.
- [ ] Enable and resolve useful compiler warnings, including `-Wall` and
  `-Wextra` where supported by the PS2 toolchain.
- [ ] Pin the CI toolchain/container to a known version for reproducible builds.

### v1.4 acceptance criteria

- [ ] Existing v1.3 patterns and controls pass regression testing.
- [ ] PAL 576i, PAL 288p, NTSC 480i, and NTSC 240p all work.
- [ ] Mode confirmation, manual rollback, and ten-second automatic rollback
  remain reliable.
- [ ] Dynamic patterns pause, resume, reset, and change parameters correctly.
- [ ] The ELF remains self-contained and requires no external assets.
- [ ] The release candidate is tested on a physical PlayStation 2 and CRT.

## v1.5 — Timing and audio/video synchronization

This milestone will focus on measurements that benefit from camera capture and
audio output.

- [ ] Tie the Frame Timer directly to VSYNC and show the detected 50/60 Hz
  cadence.
- [ ] Detect and display missed or delayed updates where practical.
- [ ] Add a camera-friendly timer mode with large binary frame indicators.
- [ ] Add left/right channel, mono, phase, and reference-tone audio tests.
- [ ] Add a synchronized flash and sound test for estimating A/V latency.

Timing claims will be documented carefully and validated on real hardware
before release.

## Future candidates

- [ ] Embedded Cyrillic bitmap font and optional Russian interface.
- [ ] About screen with the version, license, and repository address.
- [ ] Safe persistence of user preferences, if it does not complicate recovery.
- [ ] README screenshots and short demonstrations of each pattern.
- [ ] Community-maintained compatibility results covering console revisions,
  cables, scalers, and displays.

## Experimental video modes

480p, 576p, and other non-SD modes are deliberately outside the default safe
mode list. They may be explored later in a separate experimental build or
module with:

- a prominent compatibility warning;
- the same timed automatic rollback used by the safe modes;
- testing on hardware that is known to accept the selected signal.

These modes will not be promoted to the main build until they can be tested
properly. The currently documented JVC AV-2130SE test setup is an SD CRT and
cannot validate 480p or 576p output.

## Project principles

- Preserve a safe, usable picture after mode changes.
- Validate release candidates on physical hardware, not only in emulators.
- Prefer measurement value and clarity over a larger pattern count.
- Keep the application usable offline as one ELF with no external assets.
- Do not modify a television's service-menu settings automatically.
- Do not claim instrument-grade color calibration without measurement hardware.

Suggestions and hardware test reports are welcome through GitHub Issues.
