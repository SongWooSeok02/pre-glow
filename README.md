# pre-glow

Stereo reverb plugin. Phase 5 implementation is complete. Decay, Damping and Mix support host automation and session recall.

## Build

Install CMake 3.22 or newer and a C++17 toolchain. On macOS use Xcode. On Windows use Visual Studio 2022 with Desktop development with C++. CMake fetches the pinned JUCE 8.0.8 revision.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

For Visual Studio x64 add `-A x64` at configure time. For a universal macOS build add `'-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64'`. Outputs are under `build/pre_glow_artefacts/Release`. Launch the Standalone application directly. AU and VST3 are generated on macOS, and VST3 is generated on Windows.

See [architecture](docs/architecture.md) for the DSP boundary and state contract. Check the pinned JUCE [license](https://github.com/juce-framework/JUCE/blob/8.0.8/LICENSE.md) and applicable VST3 SDK terms before distribution.

## Verification

On Apple Silicon with Xcode 27.2, Apple Clang 21, CMake 3.31.6 and Ninja, Release builds succeeded for AU, VST3 and Standalone. The Standalone process launched. CTest passed the stereo impulse test at 44.1, 48, 88.2 and 96 kHz, using decay settings of 0.1, 3 and 120 seconds. The test checks finite, bounded samples and stereo tail energy over four seconds per case.

Ducking bypass and Reverse-like swell with attack retriggering built locally on macOS in Standalone, AU, and VST3 formats. Both existing DSP tests passed.

XY Morph with centered A/B and Wet controls and a collapsible square pad that moves below the spectrum when Multiband or Shimmer opens and linked Wet and A/B controls built locally on macOS in Standalone, AU, and VST3 formats. Both existing DSP tests passed.

Freeze with live reverb built locally on macOS in Standalone, AU, and VST3 formats. Both existing DSP tests passed.

Band Decay, Feedback Damping, Wet Level, Shimmer Send, and Modulation Depth with adjustable crossovers and bordered sections built locally on macOS in Standalone, AU, and VST3 formats. Both existing DSP tests passed.

Low Cut placement built locally on macOS in Standalone, AU, and VST3 formats. Both existing DSP tests passed.

Reverb A/B Blend with both percentages displayed built locally on macOS in Standalone, AU, and VST3 formats. Both DSP tests passed.

Parallel Shimmer built locally on macOS in Standalone, AU, and VST3 formats. Both DSP tests passed.

Pre-Reverb Shimmer and the Dry/Reverb spectrum display with a fixed height beside the controls built locally on macOS in Standalone, AU, and VST3 formats. Both DSP tests passed, including checks for Pre-Reverb routing and Freeze input isolation.

Shimmer On was built locally on macOS in Standalone, AU, and VST3 formats. Both DSP tests passed.

GitHub Actions passed the channel-layout update on Apple Silicon, Intel macOS, and Windows x64 ([build results](https://github.com/SongWooSeok02/pre-glow/actions/runs/43503197818)). The plugin supports mono-to-mono, mono-to-stereo, and stereo-to-stereo layouts. The host selects the channel layout. In Logic Pro, select the Mono → Stereo AU variant on a mono channel strip when available.

### Logic Pro smoke test

The user confirmed the AU loaded on a stereo track and produced audible reverb. In the open Logic Pro project, an accessibility-driven check set Decay to 5.00, Damping to 0.45 and Mix to 0.35. Closing and reopening the plug-in editor preserved those values. The initial values (0.76, 1.00 and 1.00) were restored. Apple's auval passed after the mono output update and reported 1→1, 1→2, and 2→2 channel layouts. Logic Pro menu visibility for the updated Mono → Stereo variant, project save/reload, and automation playback remain pending.

## Phase 2 complete

The reverb engine includes stereo early reflections, input diffusion, six characters, independent Early and Late Width, and separate Dry and Wet controls. See the [Phase 2 handoff](docs/phase-2-handoff.md) for verification, listening feedback, and the Phase 3 starting point.

Phase 3 starts with **Freeze**. It holds the current tail, fades new input out of the wet path, and leaves the dry signal live. Turning it off restores the current Decay and Damping settings. Older projects without the Freeze parameter load with it off.

Verified on Apple Silicon: Release AU, VST3 and Standalone builds; DSP tests at 44.1, 48, 88.2 and 96 kHz; AU validation with `auval`; and AU/VST3 Freeze hold and release at 48 kHz in a plug-in host. Logic listening, project recall and automation playback remain to be checked.

**Infinite Decay** keeps the wet input live and sets the feedback decay to 600 seconds before damping. The Damping control still shapes the tail, and Freeze takes precedence while engaged. Switching Infinite Decay off brings back the Decay knob setting. Older projects default to off.

Verified on Apple Silicon: Release AU, VST3 and Standalone builds; DSP tests at 44.1, 48, 88.2 and 96 kHz; AU validation with `auval`; and AU/VST3 input, persistence and release at 48 kHz in a plug-in host.

**Bloom** softens early reflections and lets the late tail grow in response to input. Amount blends in the effect (0% leaves the original response); Attack sets how quickly it rises, Release sets how long it lingers, and higher Shape values soften the start further. Freeze gradually takes over the tail level while engaged.

Verified on Apple Silicon: Release AU, VST3 and Standalone builds, Bloom impulse tests at 44.1, 48, 88.2 and 96 kHz, AU validation with `auval`, and AU/VST3 Bloom response at 48 kHz in a plug-in host. Logic listening remains to be checked.

**Bloom and Swell** each have a switch. Turning a switch on expands its settings; turning it off collapses the row and keeps its settings. Swell fades in the wet signal when input crosses the Sensitivity threshold. Time sets the rise duration and Curve shapes the fade. Both switches start off in new projects. Schema 1 and 2 projects with a positive saved Bloom Amount restore with Bloom on; Swell defaults off.

Verified on Apple Silicon: Release AU, VST3 and Standalone builds; DSP at 44.1, 48, 88.2 and 96 kHz; AU validation with `auval`; both effects and off states in AU/VST3 at 48 kHz; and independent expansion and collapse in Standalone. Logic window behavior remains to be checked.

When combining switches, Freeze holds the tail and suspends Infinite Decay, Bloom and Swell without changing their settings. Releasing it resumes the selected decay mode and lets the wet contours respond to input again. Bloom and Swell use the lower of their two gains for each wet path rather than multiplying the gains.

Verified all 16 switch combinations for bounded DSP output and compared the frozen combinations against Freeze alone at four sample rates. The all-on and release paths also passed in AU and VST3 at 48 kHz, and the AU passed Apple validation.

## Phase 3 implementation complete

See the [Phase 3 handoff](docs/phase-3-handoff.md) for the original scope, implementation changes, compatibility, validation gaps, and the Phase 4 starting point.

## Phase 4 complete

The standalone DSP pitch shifter uses two crossfaded delay taps and supports ±24 semitones. The pitch test checks frequency, output bounds, and reset at 44.1, 48, 88.2 and 96 kHz on Apple Silicon. The four-voice Shimmer DSP adds independent pitch, fine tune, level, pan, delay, tone, and feedback send. DSP tests at the same four rates cover voice summing, stereo placement, delay, and send isolation. The editor's Shimmer panel exposes Mix, Feedback, and the seven controls for each of four voices. Raise Mix to hear the default octave-up voice. Feedback adds the summed voice sends to the reverb loop without scaling down its original feedback. The added return is limited by the loop gain and fades out as Freeze takes hold. The audible pitch-shifted layer continues over the held tail.

State schema 4 adds Shimmer parameters. Schemas 1–3 restore with Shimmer Mix and Feedback at zero and retain the existing Mix and Bloom migrations.

Verified Apple Silicon Release AU, VST3 and Standalone builds and DSP tests at 44.1, 48, 88.2 and 96 kHz. The tests check that feedback changes the active tail, fresh input and feedback changes leave the fully frozen tail intact, and output stays bounded on release. Listening and state restoration in a DAW remain to be checked.

Verified the revised Shimmer gain and routing with Apple Silicon Release builds, DSP tests at all four sample rates, and validation of the installed AU. With every voice Send at zero, the Feedback knob leaves the rendered tail identical.

Each Shimmer voice has its own card. On enables the voice and opens its controls. Hide controls folds the card while the voice continues to play. Show controls opens it for editing, including while the voice is off. Scroll the cards when opening multiple sets of controls. Schema 5 saves the checkboxes and enables voices with a positive saved Level when opening schema 4 projects. Apple Silicon Release AU, VST3 and Standalone builds and DSP tests passed.

Phase 4 includes Shimmer Routing with Feedback and Post-Reverb modes. Feedback is the default for existing projects. Post-Reverb keeps the pitch layer in the wet output and fades its return into the reverb loop to zero. Apple Silicon AU, VST3 and Standalone builds and DSP tests passed. Logic listening and project recall checks are pending.

See the [Phase 4 handoff](docs/phase-4-handoff.md) for the Phase 5 starting point.

## Phase 5 implementation complete

See the [Phase 5 handoff](docs/phase-5-handoff.md) for the completed scope, validation results, and the Phase 6 starting point.

Phase 6 starts with Basic and Advanced editor modes. Basic opens by default with Reverb A/B, Decay, Dry, Wet, Shimmer On, and Freeze. The mode button sits below the PRE-GLOW title. Advanced exposes the detailed controls and restores expanded sections when switching back.

The macOS Release Standalone, AU, and VST3 builds and the `reverb_impulse` and `pitch_shifter` tests passed for this change. DAW mode-switch listening and session recall are pending.

### Presets

Open **Presets** below the spectrum in either editor mode. Filter by Factory or User, search by name, and select **Load**. **Save As** captures the current parameter settings. User presets can be renamed or moved to Trash. Factory presets are read-only.

User files use the `.preglowpreset` extension and the versioned processor state payload. On macOS, AU, VST3, and Standalone use `~/Library/Application Support/pre-glow/Presets`.

The macOS Release Standalone, AU, and VST3 builds, both DSP tests, and validation of the installed AU passed. In-host preset UI and AU/VST3 round-trip verification are pending.

The PRE-GLOW naming and Basic layout update passed macOS Release builds for all three formats, both DSP tests, and AU validation under the new display name.

Input and output peak meters display dBFS below the spectrum in both editor modes. macOS Release Standalone, AU, and VST3 builds, both DSP tests, and installed AU validation passed. Meter behavior in Logic Pro has yet to be verified.

The Tail meter shows the wet signal entering the output mix in both editor modes. macOS Release builds, both DSP tests, and installed AU validation passed.

The Stereo meter shows output correlation from −1 to +1. macOS Release builds, both DSP tests, and installed AU validation passed.

The smoothed Stereo display passed macOS Release builds, both DSP tests, and installed AU validation.

The Advanced Shimmer signal path display passed macOS Release builds, both DSP tests, and installed AU validation.

The Multiband spectrum overlay passed macOS Release builds, both DSP tests, and installed AU validation.

## Phase 6 implementation complete

See the [Phase 6 handoff](docs/phase-6-handoff.md) for the implementation checkpoint, verification gaps, and Phase 7 performance baseline.

## Phase 7 verification

Apple Silicon Release Standalone, AU and VST3 builds passed, along with both DSP tests and installed AU validation. AU and VST3 rendered impulse responses at 48 and 96 kHz in Pedalboard 0.9.17. See the [Phase 7 QA record](docs/phase-7-qa.md) for engine timings and pending DAW checks.
