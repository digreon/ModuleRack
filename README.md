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
- `Source/UI/PresetBrowser.h/.cpp` — the in-plugin patch list, deliberately free
  of system file dialogs so it behaves the same in an AUv3 extension.
- `Tests/ModuleRackTests.cpp` — headless checks over the real signal graph.
- `Tools/RenderPreview.cpp` — renders a patch to a WAV with no host involved.
- `Tools/RenderUiSnapshot.cpp` — renders the editor to PNGs offscreen, so a
  layout change can be looked at without a device or a display server.

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

## The iPad build

The iPad is the deployment target, and one build produces **both** ways of
running it:

- **A standalone app** you launch from the home screen — full-screen, plays on
  its own, needs no host. JUCE opens every MIDI input automatically on iOS, so
  an attached MPK Mini is picked up with no settings screen to visit.
- **An AUv3 plugin** for use inside a host (AUM, Loopy Pro, Cubasis,
  GarageBand), where it can be recorded, sequenced and mixed with other apps.

These are not an either/or, and the standalone app is not a development
leftover: on iOS an AUv3 can only be installed by installing an app that
contains it. Building the standalone app *is* how the plugin reaches the
device, and the same app is a usable instrument on its own.

The MPK Mini attaches over USB (camera adapter) or BLE MIDI in both cases.

```sh
cmake -B build-ios -G Xcode \
      -DCMAKE_SYSTEM_NAME=iOS \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 \
      -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM=YOURTEAMID \
      -DMODULERACK_APP_GROUP_ID=group.com.yourteam.modulerack
cmake --build build-ios --config Release -- -sdk iphoneos
```

Install the resulting app on the iPad with Xcode. It runs standalone from the
home screen, and the AUv3 inside it appears in every host's plugin list. The
headless test target is skipped automatically on iOS.

What the iOS configuration sets up, and why each one matters here:

| Setting | Why |
|---------|-----|
| `BACKGROUND_AUDIO_ENABLED` | Hosts keep playing when backgrounded or the screen locks; the carrier app has to allow it |
| `BACKGROUND_BLE_ENABLED`, `BLUETOOTH_PERMISSION_ENABLED` | A Bluetooth MIDI controller needs the permission and has to survive backgrounding. USB (camera adapter) needs neither |
| `REQUIRES_FULL_SCREEN FALSE` | Split View / Slide Over — an instrument gets used beside its host |
| `IPAD_SCREEN_ORIENTATIONS` (all four) | The host decides the orientation, not the plugin |
| `FILE_SHARING_ENABLED` | Patches are reachable from the Files app |
| `MODULERACK_APP_GROUP_ID` | **The one that needs your Apple developer account.** An AUv3 extension and its carrier app are separate sandboxes: without a shared App Group, a patch saved in one is invisible to the other. Set it to a group registered to your team; leave it unset and each side keeps its own presets |

Touch is the primary input on the panel: the pad row is 56pt tall (past Apple's
44pt minimum), the knob text boxes are read-only so tapping one doesn't summon a
keyboard the extension can't host, and the refresh timer leaves alone any knob a
finger is currently on. The editor is sized for landscape but shrinks to 480x340
for a narrow host pane.

Patches are saved and loaded through a list built into the plugin window (the
**Presets** button), not through a system file dialog. On iOS a file dialog is a
document picker, and modal system UI inside an AUv3 extension is unreliable —
so nothing here leaves the plugin's own window. Save needs no typing (a free
"Patch N" name is offered), and Delete asks for a second tap instead of opening
an alert.

**CPU:** all 8 modules render 82x faster than real time on one 2.8 GHz Xeon core
(~1.2% of a core), so an iPad has ample headroom. That figure is offline at a
512-sample block; measure on-device at your host's real block size before
trusting it for a big session.

## Building elsewhere

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

### Hearing a patch without a host

```sh
./build/ModuleRackRender_artefacts/Release/ModuleRackRender out.wav 30 [preset.xml]
```

Renders the real signal graph offline to a WAV — useful for checking a DSP
change by ear from a machine that can't run the plugin.

### Looking at the interface without a device

```sh
./build/ModuleRackUiSnapshot_artefacts/Release/ModuleRackUiSnapshot out-dir
```

Paints the editor into PNGs — the panel for two different modules, the preset
list, and a cramped 480x340 pane — with no window and no display server. Worth a
look after any layout change; it is how the compact layout's collapsed knobs and
the normalised 0..1 readouts were caught.

## Status

The DSP core, the mapping and preset round-tripping are covered by the headless
tests, which build and pass on Linux and macOS in CI. The plugin, standalone app
and VST3 build clean on Linux.

Nothing here has touched an Apple toolchain, so on the iPad side treat all of
this as written-but-unrun: the iOS CMake configuration has been checked keyword
by keyword against JUCE's own argument lists, not executed; the AUv3 bundle has
never been built; the plugin has never been loaded in a host; and no one has
played it from an actual MPK Mini. First things to check on the Mac, in order:
the AUv3 builds and installs, a host lists and loads it, the pads and knobs land
on the right modules, and the sound survives backgrounding.

### Known gaps

- Module parameters are not exposed as host-automatable
  `AudioProcessorValueTreeState` parameters; the MPK Mini (or the mouse) is the
  only way to change them. Worth revisiting for AUv3 hosts.
- Nothing recalls the module patterns themselves — a preset restores the knobs
  and the mapping, while the generative sequencers restart from their seeds.
- The Arp/Sequencer only plays its own voice; it can't drive notes into the
  other modules yet.
