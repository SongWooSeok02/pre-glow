# Architecture

JUCE owns host wrappers, bus configuration, automation and the editor. The processor reads cached atomic parameter values once per block and passes typed targets to the DSP. The DSP owns the delay memory and runs independently of JUCE. The editor attaches controls to the same parameter state as host automation.

The stereo engine exposes prepare, reset, setTarget and in-place sample processing. Prepare sizes the delay memory. Reset clears the tail and snaps smoothing to the current targets. SetTarget clamps settings and recalculates feedback gains when decay changes. Four short all-pass delays diffuse the input before it enters the feedback network. The four FDN delay lines use an orthogonal Hadamard feedback matrix and low-pass feedback damping. A stereo input history feeds early reflections into the wet output independently of the feedback loop. Prepare allocates the history and reset clears it. The requested decay controls nominal RT60 at low frequencies. The processor reports a conservative 120-second tail and zero algorithmic latency.

The six characters use different FDN and diffuser delays and early reflection levels. The processor prepares all three engines and crossfades their outputs so changing Character during playback preserves a continuous tail.

Cloud and Void add a second input diffuser. Cathedral uses longer FDN delays. Early Width and Late Width scale the side signal of their respective wet paths before the dry/wet mix. At 0% the affected path is mono, and at 100% it uses the original stereo image.

## Host contract

| Item | Value |
|---|---|
| Bundle ID | audio.preglow.plugin |
| Manufacturer code | Pglo |
| Plugin code | PrGl |
| State root | PreGlowState |
| Schema version | 1 |

| Parameter ID | Range | Default |
|---|---|---|
| decay | 0.1–120 seconds | 3 seconds |
| damping | 0–1 | 0.45 |
| mix | 0–1 | 0.35 |
| character | Hall, Room, Plate, Cloud, Cathedral, Void | Hall |
| early_width | 0–200% | 100% |
| late_width | 0–200% | 100% |

Existing parameter IDs use AU version hint 1. Character uses hint 2 and the width controls use hint 3. Missing parameters use their defaults when loading schema 1. Character indices in saved state are preserved. Hosts that store automation as normalized values may remap existing Character automation as the choice range expands. The host state chunk wraps APVTS XML in JUCE's binary envelope and carries schemaVersion. The loader accepts schema 1 and the matching root. Future schema changes should use explicit migration before state replacement. Presets can share the versioned payload and add metadata outside the processor state.

## Phases

1. Foundation and a minimal stereo reverb.
2. Expand the DSP with separate reverb characters, diffusion, early reflections and stereo processing.
3. Add freeze, infinite decay, bloom and swell around the shared reverb engine.
4. Add pitch shifting and the multivoice shimmer engine.
5. Prepare and validate routing configurations outside the callback. Add frequency-dependent feedback inside the shared FDN, bounded modulation and XY mappings. Resolve host automation as the base value before applying modulation.
6. Extend the editor with Basic and Advanced pages, preset management and atomic metering snapshots. Keep platform installation separate from the DSP and UI.
7. Add DSP, integration, stability, performance, format and DAW validation.
8. Package and document release builds.
