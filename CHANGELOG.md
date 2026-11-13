# Changelog

## Unreleased

- Added an automatable Freeze switch that holds the reverb tail while keeping the dry signal live.

- Softened early reflections and increased input diffusion in Hall and Room.
- Replaced the editor Mix knob with independent Dry and Wet controls.

## 0.1.3

- Added Cloud, Cathedral and Void to the Character menu.
- Added independent stereo width controls for early reflections and the late tail.

## 0.1.2

- Added selectable Hall, Room and Plate reverb characters.

- Added input diffusion for a denser start to the reverb tail.
- Added stereo early reflections before the FDN tail.

## 0.1.1

- Show the build version in the host plug-in name and editor.
- Added a mono output option for hosts that filter mono inserts by channel layout.

## 0.1.0

- Added JUCE/CMake targets for AU, VST3 and Standalone.
- Added stereo FDN, three automatable controls and versioned host state.
- Added a DSP impulse test and macOS/Windows build matrix.
- Added mono-to-stereo processing alongside stereo-to-stereo. Mono dry and bypass signals are centered.
