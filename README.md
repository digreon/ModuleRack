# ModuleRack

Generative EDM instrument, modular-synth style, played entirely from an Akai
MPK Mini (8 pads, 8 knobs). Part of the HORIZON project, developed alongside
(not replacing) the existing iPad music-box work in `digreon/Horizon-8`.

## Concept

Each of the MPK Mini's 8 pads corresponds to one fixed "module" — a sound
source or an effect, as in a modular/Eurorack rig, but without physical patch
cables:

1. Pressing a pad selects/highlights that module on screen.
2. It rebinds the 8 knobs to that module's 8 parameters.
3. The screen shows that module's control panel.

All 8 modules generate audio continuously and mix together, like layers of a
track. Pads never start or stop a module — they only change which module the
knobs and the panel are pointed at.

## Phase 1 (this MVP)

Fixed signal graph, no free patching:

```
Kick, Bass, Lead, Pad/Chord, Arp/Sequencer, Percussion  --sum-->  mix bus
                                                                     |
                                                            Fx1: Filter/Delay
                                                                     |
                                                     Fx2: Reverb / Kick sidechain-comp
                                                                     |
                                                       master soft-clip --> output
```

The 8 pad slots:

| # | Module | What it does |
|---|--------|---------------|
| 1 | Kick | Euclidean-pattern kick: pitch-sweep sine + noise click, with swing |
| 2 | Bass | 16-step generative bassline (mutating pattern), saw/square through a resonant lowpass |
| 3 | Lead | Sparser generative line, an octave up, two-osc unison + vibrato |
| 4 | Pad / Chord | Diatonic 4-chord progression, holds each chord for N bars, detuned unison pad voice |
| 5 | Arp / Sequencer | 16-bit shift-register generative sequencer (Marbles/Turing-Machine style) |
| 6 | Percussion | Independent Euclidean hi-hat pattern, closed/open noise bursts, with swing |
| 7 | Fx1: Filter/Delay | Insert on the mix bus |
| 8 | Fx2: Reverb/Comp | Insert on the mix bus; sidechain-ducks from Kick's envelope |

All generative modules read from one shared sample-accurate 16th-note `Clock`
(`Source/Core/Clock.h`), so they stay in sync without needing MIDI clock or a
host transport.

Phase 2 (not built yet, deliberately deferred): a small mod matrix — 2-3
modulation sources (LFO, envelope follower) with a dropdown-style
source-to-destination UI, instead of a full patch-cable renderer. Kept out of
Phase 1 so the audio graph shape never has to change at runtime for the MVP.

## Architecture

- `Source/Core/ModuleSlot.h` — base class every module implements: `prepare()`,
  `process(buffer, midi, clock)`, 8 `ModuleParam`s, plus XML (de)serialisation
  for presets. Param values are atomic, because the UI thread (mouse) and the
  audio thread (incoming MIDI CC) both write them. Fx1/Fx2 reuse the same
  interface but treat `buffer` as the already-summed mix to modify in place
  rather than an empty buffer to fill.
- `Source/Core/SignalGraph.h/.cpp` — owns the `Clock` and all 8 concrete
  modules in the fixed order above; sums the 6 sound modules through a mix
  trim, then runs Fx1 and Fx2 on the bus in series and soft-clips the master.
- `Source/Core/Clock.h` — the shared 16th-note grid. Tick positions accumulate
  in double precision, so the grid never drifts from the nominal tempo and a
  tick landing on a block boundary is still delivered.
- `Source/Core/MidiMapper.h/.cpp` — pad-note and knob-CC tables. The numbers
  live in a `Profile` and can be changed at runtime; they are saved with the
  patch. Everything else in the app talks to modules by index, never by MIDI
  number.
- `Source/Core/PresetManager.h/.cpp` — saves/loads a full patch (all 8 modules'
  params + tempo + controller mapping) as one human-readable XML file.
- `Source/Modules/*` — the 8 concrete modules.
- `Source/UI/*` — `MixOverviewComponent` (pad row, also clickable with a mouse)
  and `ModulePanel` (8 rotary sliders bound to the selected module, also
  mouse-editable — the plugin doesn't require the MPK Mini to be usable for
  testing/sound design).
- `Tests/ModuleRackTests.cpp` — headless checks over the real signal graph.
- `Tools/RenderPreview.cpp` — renders a patch to a WAV with no host involved.

### Decisions made (the questions from the project brief)

1. **Reuse Horizon drum code, or write new voices?** New voices, written for
   this project. There is no drum-synth code in `digreon/Horizon-8` to reuse —
   that repo holds an earlier copy of this same scaffold.
2. **Separate project or a branch of HORIZON?** Its own repo, this one, with
   the Horizon project referenced rather than forked.
3. **Preset format?** XML, one file per patch, holding every module's params
   plus tempo and the controller mapping. Chosen so patches are easy to diff,
   inspect and hand-edit while designing modules.
4. **Hardcoded MPK Mini mapping, or remappable?** Remappable, but with an MPK
   Mini profile as the default. The mapping is data, not code: it is editable
   at runtime, travels with the saved patch, and switching controllers never
   touches module or UI code.

### Controller mapping

The default profile expects pads on notes 36-43 and knobs K1-K8 on CC 70-77;
a second profile is provided for controllers whose knobs send CC 1-8, and the
editor has a dropdown to switch between them. Pad and knob assignments are
user-editable on the MPK Mini itself and differ between generations, so check
yours in MPK Mini Editor — if the pads or knobs land on the wrong module,
that's the mapping, not the plugin.

## Building

CMake fetches JUCE (8.0.15) automatically.

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

On macOS this builds AU, AUv3 and Standalone; elsewhere VST3 and Standalone,
so the DSP and UI can be compiled and run on any dev machine. Add `-G Xcode`
on macOS if you want the Xcode project. The macOS CI job passes
`-DMODULERACK_BUILD_AUV3=OFF`, since the app-extension bundle hasn't been
validated on a bare runner — build AUv3 locally with the defaults.

For an iPad AUv3 host (e.g. Loopy Pro), build/archive the AUv3 target from the
generated Xcode project and install it onto the device with Xcode, the same as
any other AUv3 plugin.

### Hearing a patch without a host

```sh
./build/ModuleRackRender_artefacts/Release/ModuleRackRender out.wav 30 [preset.xml]
```

Renders the real signal graph offline to a WAV — useful for checking a DSP
change by ear from a machine that can't run the plugin.

## Status

The DSP core, the mapping and preset round-tripping are covered by the headless
tests, which build and pass on Linux and macOS in CI. The plugin, standalone
app and VST3 build clean on Linux. What is still unproven: the AU target is
compiled by CI but never loaded in a host, AUv3 has not been built at all here,
and no one has yet played the thing from an actual MPK Mini — so treat the pad
notes, the knob CCs and the AUv3 packaging as the first things to check on a
Mac.

### Known gaps

- Module parameters are not exposed as host-automatable
  `AudioProcessorValueTreeState` parameters; the MPK Mini (or the mouse) is the
  only way to change them. Worth revisiting for AUv3 hosts.
- Nothing recalls the module patterns themselves — a preset restores the knobs
  and the mapping, while the generative sequencers restart from their seeds.
- The Arp/Sequencer only plays its own voice; it can't drive notes into the
  other modules yet.
