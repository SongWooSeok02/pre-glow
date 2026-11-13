# Phase 2 handoff

Recorded on 2026-11-14 (KST). Phase 3 starts from the Phase 2 implementation at [a22fa0a](https://github.com/SongWooSeok02/pre-glow/commit/a22fa0a).

## Delivered

- Stereo early reflections and input diffusion feeding the feedback delay network (FDN).
- Hall, Room, Plate, Cloud, Cathedral, and Void.
- Independent Early and Late Width controls from 0% to 200%. At 0% the affected wet path is mono.
- Independent Dry and Wet levels. Schema 1 projects migrate saved Mix to complementary Dry and Wet values in schema 2. The legacy Mix parameter remains available to hosts.
- Softer early reflections and a second diffuser in Hall and Room after listening feedback.

## Verification and open work

- Apple Silicon Release builds succeeded for AU, VST3, and Standalone. CTest passed at 44.1, 48, 88.2, and 96 kHz. The final AU passed Apple's auval.
- At 48 kHz, the 0.1.3 AU and VST3 produced distinct responses for all six characters and independent width changes. The subsequent Hall and Room revision was checked in both formats for dry-only, wet-only, muted output, and reduced early peaks.
- Logic Pro loaded and played a test WAV through the diffusion build. The user reported that the latest Hall and Room adjustment helped, though some short delay character remains. Track that as a listening and tuning task during Phase 3.
- Full Logic save/reload and automation playback still need verification. Hosts storing normalized Character automation may remap older values because the list expanded from three to six choices.
- Test assets on the development Mac are under ~/Music/Logic. Reuse the pre-glow Diffusion Test project and pre-glow-test.wav for comparisons. Restart Logic after replacing the AU so it loads the new binary.

## Start Phase 3 with Freeze

Use the existing FDN in [Reverb.h](../DSP/Reverb/Reverb.h). Start with a default-off Freeze control that holds the current tail. Ramp new input into the wet path down and bring feedback toward unity while reducing loop damping. On release, restore the input, damping, and Decay settings smoothly. Keep parameter IDs, state compatibility, mono/stereo layouts, and allocation-free audio processing as described in the [architecture](architecture.md).

Verify that a charged tail holds during Freeze, fresh input does not build up in the held tail, and the tail returns to normal decay after release. Check bounded output and repeated transitions in DSP tests, then validate in AU and VST3 and audition in Logic. Continue with Infinite Decay, Bloom, and Swell after Freeze. Phase 4 covers Shimmer.
