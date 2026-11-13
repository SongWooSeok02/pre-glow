# Changelog

## Unreleased

- Added a Shimmer On switch that fades both Shimmer routes out without changing their settings. State schema 6 stores the switch; older sessions restore with Shimmer enabled.

- Kept Shimmer voice value fields inside their cards with per-column spacing and bottom padding.

- Added a Shimmer Routing selector for Feedback and Post-Reverb.

- Grouped Shimmer knobs into four cards with independent controls for enabling the voice and folding its knobs.

- Added independent Shimmer voice checkboxes that mute or expand each voice and retain its settings.

- Raised the Shimmer wet level and made its feedback additive so the base reverb loop keeps its decay.

- Added a four-voice Shimmer panel with independent voice controls, global Mix and Feedback, and Freeze-aware reverb feedback.

- Started Shimmer development with a standalone pitch shifter and DSP verification (no plug-in controls yet).

- Coordinated Freeze, Infinite Decay, Bloom and Swell so their switches can be combined without stacking wet gain fades.

- Added Swell with Time, Sensitivity and Curve controls.
- Added Bloom and Swell switches that expand or collapse their settings.

- Added Bloom Amount, Attack, Release and Shape to bring in the late tail gradually.

- Added Infinite Decay for extended tails that continue to accept input.

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
