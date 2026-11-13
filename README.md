# pre-glow

Phase 1 stereo reverb prototype. Decay, Damping and Mix support host automation and session recall.

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

GitHub Actions passed the channel-layout update on Apple Silicon, Intel macOS, and Windows x64 ([build results](https://github.com/SongWooSeok02/pre-glow/actions/runs/43503197818)). The plugin supports mono-to-mono, mono-to-stereo, and stereo-to-stereo layouts. The host selects the channel layout. In Logic Pro, select the Mono → Stereo AU variant on a mono channel strip when available.

### Logic Pro smoke test

The user confirmed the AU loaded on a stereo track and produced audible reverb. In the open Logic Pro project, an accessibility-driven check set Decay to 5.00, Damping to 0.45 and Mix to 0.35. Closing and reopening the plug-in editor preserved those values. The initial values (0.76, 1.00 and 1.00) were restored. Apple's auval passed after the mono output update and reported 1→1, 1→2, and 2→2 channel layouts. Logic Pro menu visibility for the updated Mono → Stereo variant, project save/reload, and automation playback remain pending.

## Next development session

Start Phase 2 from the 0.1.1 foundation. Expand the FDN with distinct reverb characters, diffusion, early reflections, and stereo processing. Preserve the existing parameter IDs, state schema, and supported channel layouts. Validate changes with impulse tests and use Logic Pro to check the mono-to-stereo insert when testing the next AU build.
