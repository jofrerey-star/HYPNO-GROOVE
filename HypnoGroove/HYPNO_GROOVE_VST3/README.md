# HYPNO GROOVE — VST3 House Drum Machine (v1.0 source)

A six-voice drum machine for FL Studio, built with C++17 and [JUCE](https://juce.com/).

**This ZIP contains the actual VST3 source project and Windows build automation — not a precompiled Windows `.vst3` binary.** Build it on Windows or with GitHub Actions. The DSP core has been tested locally; the full plugin has not yet been tested in FL Studio.

## What it does

- 6 drum lanes: kick, clap, closed hi-hat, open hi-hat, shaker and rim.
- 16-step sequencer with two independent patterns **A** and **B**.
- Built-in synthesized, royalty-free drum hits — works without any external samples.
- Load your own **WAV or AIFF** drum samples individually per lane.
- Host-synced playback (tempo, play/stop and song position): starts on beat one, loops every 16 steps.
- Automatable **Pattern A/B**, Swing, Humanize, Global Hit Probability, Master and six individual lane levels.
- State recall for pattern steps, parameters and local sample paths. If a sample is moved, the factory sound is used.
- One stereo output. Designed to sit on a channel in FL Studio's Channel Rack.

## Easiest: compile with GitHub Actions (Windows without installing tools)

1. Sign into [github.com](https://github.com) and create a **new, private repository**.
2. Upload the **contents** of this folder (including the hidden `.github` folder) into that repository. A simpler way if you have git installed: `git init`, commit these contents, and push to GitHub.
3. Open the repository's **Actions** tab and choose **Build HYPNO GROOVE for Windows**. Click **Run workflow**. The workflow also runs on pushes to `main`.
4. Once the build passes, open the workflow run and download the artifact named **HYPNO-GROOVE-Windows-VST3**.
5. Extract the ZIP. Copy the entire `HYPNO GROOVE.vst3` **folder** to:

   `C:\Program Files\Common Files\VST3\`

6. Restart FL Studio. Open **Options > Manage plugins > Find installed plugins**, then add **HYPNO GROOVE** as an instrument from the Plugin Picker or Channel Rack.
7. Set your song BPM (for example 124), select **SONG** or **PAT**, press **Play**, and the groove follows FL Studio's transport. Use the A/B control to change patterns.

> Note: GitHub runners need internet access to download JUCE 8.0.6. The initial build can take several minutes. If the workflow is disabled on a newly created repository, enable it under Actions.

## Compile locally on Windows

Requires **Visual Studio 2022** with the *Desktop development with C++* workload, **CMake 3.22+**, **Git**, and internet for the first JUCE fetch.

Open PowerShell inside this folder:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel 4
```

Look inside `build\HypnoGroove_artefacts\Release\VST3\` for `HYPNO GROOVE.vst3` and copy that folder into `C:\Program Files\Common Files\VST3\`.

## Making a pattern

- Click a lit square to mute a hit; click a dark square to add a hit.
- Select **PATTERN A** or **PATTERN B** at the top right. The two patterns are independent.
- Turn up **Swing** for shuffled offbeat notes, **Humanize** for velocity variations, or turn down **Probability** for occasional dropped notes.
- Move the six small horizontal volume sliders to balance instruments.
- Click **LOAD** on a row to choose your sample; click the circular arrow to return to its built-in sound.
- **RESET PATTERNS** restores the original A/B step arrangements (not sample selections).

Factory rhythm:

| Lane | Pattern A (steps) | Pattern B (steps) |
|---|---|---|
| Kick | 1,5,9,13 | 1,5,9,13,16 |
| Clap | 5,13 | 5,13 |
| Closed hat | 3,7,11,15 | 3,7,10,11,15 |
| Open hat | 7,15 | 7,15 |
| Shaker | 2,4,6,8,10,12,14,16 | 2,4,6,8,10,12,14 |
| Rim | 12 | 6,14 |

## Current limitations

- Pattern length is **16 steps**, not 32; each pattern loops every bar in 4/4.
- Mono playback for each sample, mixed to the plugin's stereo output (no panning or individual mixer outputs yet).
- A/B switching is immediate rather than quantized to the next bar.
- No MIDI note triggering or BeatStep Pro native MIDI mapping yet. The plugin exposes parameters that FL Studio can automate or link to MIDI controls.
- No export of step patterns as MIDI yet.
- Some hosts do not supply PPQ position to plugins. HYPNO GROOVE requires a host that supplies both transport state and PPQ position.
- This project is provided as a first version. Its standalone DSP tests pass, but JUCE/VST3 compilation and FL Studio compatibility **still need to be verified on a Windows build**.

## Testing the DSP core (without JUCE)

```sh
c++ -std=c++17 -O2 tests/core_tests.cpp -o core_tests
./core_tests
```

## Licensing

The original HYPNO GROOVE application code in this folder is released under the MIT License (see `LICENSE`).
JUCE is a separate dependency subject to JUCE's own license conditions. Review JUCE's license before commercial distribution.
