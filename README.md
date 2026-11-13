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

Windows x64 and Intel macOS jobs are defined in CI and await execution. Format validation, DAW loading, and listening tests remain pending. The current plugin accepts stereo input and output.
