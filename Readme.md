# Media Player Classic - Home Cinema (modernisation fork)

[![Build](https://github.com/afrugalpenguin/mpc-hc/actions/workflows/build.yml/badge.svg?branch=modernise)](https://github.com/afrugalpenguin/mpc-hc/actions/workflows/build.yml)

Media Player Classic - Home Cinema (MPC-HC) is a free and open-source video and audio player for Windows. Being a DirectShow player, it allows flexibility and extensibility through third party filters such as LAV Filters, madVR and MPC Video Renderer.

**This is a fork, not the official MPC-HC.** It is based on [clsid2/mpc-hc](https://github.com/clsid2/mpc-hc), the maintained MPC-HC 2.x line. For releases, downloads and support, use clsid2's project. Work here happens on the `modernise` branch.

## What this fork is for

The goal is to separate the playback engine from the window it lives in, then build a new shell on top of it:

1. **Extract the engine.** Today the main window (`src/mpc-hc/MainFrm.cpp`, about 25k lines in one class) *is* the player. The graph lifecycle, renderer selection, subtitle pipeline and playback state move into a core library, `mpc-core`, one responsibility at a time. The existing MFC player keeps running on the extracted core throughout, so every step ships and nothing regresses silently.
2. **Expose it through a flat C ABI**, testable without a window.
3. **Build a new WinUI 3 (C#) shell** on that ABI, designed around discoverability: searchable settings, a command palette, sensible defaults. The current player has the features; users cannot find them.

Scope is core playback only: files, streams, DVD and Blu-ray folders, subtitles, audio tracks, pixel shaders and the playlist. The full reasoning and phase plan are in the [architecture decision record](docs/architecture-decision.md).

## Status

- **Phase 0 (baseline): done.** Verified build steps ([docs/Baseline-Build.md](docs/Baseline-Build.md)), CI on every pull request, out-of-scope subsystems removed, and the engine seam mapped ([docs/Engine-Seam.md](docs/Engine-Seam.md)).
- **Phase 1 (engine extraction): in progress.** `mpc-core` exists as a static library with unit tests that run in CI; responsibilities are moving into it.
- **Phase 2 (new shell): not started.**

### Removed in this fork

These were removed as out of scope; the options, menu items and command-line switches went with them:

- The built-in HTTP web interface (`/webport`)
- Digital (BDA/DVB) TV and analog capture: File > Open Device, the Capture options page, `/device`, `/dvbscan*`
- The DSM, Matroska and WAV muxers (playing `.dsm` files is unaffected)
- Logitech LCD support
- The DoctorDump crash reporter (`/nocrashreporter` is still accepted and ignored)

Stored settings for these are left untouched, so going back to an official build loses nothing.

## Building

See [docs/Compilation.md](docs/Compilation.md) for the full instructions and [docs/Baseline-Build.md](docs/Baseline-Build.md) for the exact, verified steps on Visual Studio 2022 Build Tools, including the deviations needed for a green build. In short, from PowerShell:

```powershell
git submodule update --init --recursive --jobs 4
.\build.bat Build x64 MPCHC Release
.\bin\lib\Release_x64\mpc-core-tests.exe   # mpc-core unit tests
```

The player ends up in `bin\mpc-hc_x64\mpc-hc64.exe`. To keep it from sharing settings with an installed MPC-HC, put an empty `mpc-hc64.ini` next to it (portable mode).

## Repository layout

| Path | What |
|---|---|
| `src/mpc-hc` | The MFC player (the shell, and for now most of the engine) |
| `src/core` | `mpc-core`, the playback engine being extracted, and its tests (`src/core/tests`) |
| `src/filters`, `src/DSUtil`, `src/SubPic`, `src/Subtitles` | DirectShow filters, utilities and the subtitle renderers |
| `src/thirdparty` | Third-party code, mostly git submodules (LAV Filters, libass, ...) |
| `docs` | Decision record, build notes, seam map |

## Contributing

- Work on a `feature/`, `fix/` or `chore/` branch and open a pull request against `modernise`; one concern per pull request.
- Commit messages follow `type(scope): description` (conventional commits), subject line 72 characters or less.
- CI must be green before merge. Changes that move code into `mpc-core` must leave the player's behaviour unchanged; if one cannot, say so in the pull request.

## Upstream

Most of what follows is clsid2's description of the player, still accurate for this fork apart from the removals above. The player has been maintained by clsid2 and contributors since 2017; the internal codecs come from [LAV Filters](https://github.com/Nevcairiel/LAVFilters). If you want a stable player rather than a work in progress, use [clsid2/mpc-hc](https://github.com/clsid2/mpc-hc).

## Overview of features

A lot of people seem to be unaware of some of the awesome features that have been added to MPC-HC in the past years. Here is a list of useful options and features that everyone should know about:
* Modern GUI Theme (Dark or Light)<br/>
Options > Player > User Interface<br/>
When using modern theme it is also possible to change the height of the seekbar and size of the toolbar buttons.<br/>
* Video preview on the seekbar<br/>
Options > Player > User Interface
* Play HDR video<br/>
This requires using MPC Video Renderer (included) or [madVR](http://forum.doom9.org/showthread.php?t=146228).<br/>
After installation these renderers can be selected here:<br/>
Options > Playback > Output<br/>
Tip: If you want an easy to use all-in-one package that contains MPC-HC and these renderers, then use [K-Lite Codec pack](http://codecguide.com/download_kl.htm).
* Added support for playback of many new audio and video formats:<br/>
For example HEVC (H.265) / VVC (H.266) / AV1 video, and AC4 audio.<br/>
* Improved subtitle rendering performance, and ability to use libass. Plus support for WebVTT.
* Ability to search for subtitles<br/>
Press D for manual search.<br/>
Or enable automatic search in: Options > Subtitles > Misc<br/>
* Adjust playback speed<br/>
Menu > Play > Playback rate<br/>
The buttons in the player that control playback rate take a 2x step by default. This can be customized to smaller values (like 10%):<br/>
Options > Playback > Speed step<br/>
Adjusting playback speed works best in combination with the SaneAR or MPC Audio Renderer. These two renderers have automatic pitch correction.<br/>
Options > Playback > Output > Audio Renderer
* MPC-HC can remember playback position, so you can resume from that point later<br/>
Options > Player > History
* You can quickly seek through a video with Ctrl + Mouse Scrollwheel.
* You can jump to next/previous file in a folder by pressing PageUp/PageDown.
* You can perform automatic actions at end of file. For example to go to next file or close player.<br/>
Options > Playback > After Playback (permanent setting)<br/>
Menu > Play > After Playback (for current file only)
* A-B repeat<br/>
You can loop a segment of a video. Press \[ and \] to set start and stop markers.
* You can rotate/flip/mirror/stretch/zoom the video<br/>
Menu > View > Pan&Scan<br/>
This is also easily done with hotkeys (see below).
* There are lots of keyboard hotkeys and mouse actions to control the player. They can be customized as well.<br/>
Options > Player > Keys<br/>
Tip: there is a search box above the table.
* You can stream videos directly from Youtube and many other video websites<br/>
Put [yt-dlp.exe](https://github.com/yt-dlp/yt-dlp/releases) in the MPC-HC installation folder.<br/>
Then you can open website URLs in the player: Menu > File > Open File/URL<br/>
You can even download those videos: Menu > File > Save a copy<br/>
Tip: to be able to download in best quality with yt-dlp, it is recommended to also put [ffmpeg.exe](https://www.gyan.dev/ffmpeg/builds/) in the MPC-HC folder.<br/>
Several YDL configuration options are found here: Options > Advanced<br/>
This includes an option to specify the location of the .exe in case you don't want to put it in MPC-HC folder.<br/>
Note 1: You also need to install [Microsoft Visual C++ 2010 SP1 Redistributable Package (x86)](https://download.microsoft.com/download/1/6/5/165255E7-1014-4D0A-B094-B6A430A6BFFC/vcredist_x86.exe)<br/>
Note 2: [yt-dlp nightly build](https://github.com/yt-dlp/yt-dlp-nightly-builds/) (very latest version made daily)
Note 3: [yt-dlp fork with Win7 support](https://github.com/nicolaasjan/yt-dlp/releases)
* Besides all these (new) features, there have also been many bugfixes and internal improvements in the player in the past years that give better performance and stability.

## System Requirements

* Windows 7 / 8 / 8.1 / 10 / 11. Builds made with a Windows 10 SDK, as CI and [docs/Baseline-Build.md](docs/Baseline-Build.md) do, need Windows 7 SP1 or later.

## License

MPC-HC's code is licensed under [GPL v3](/COPYING.txt).
