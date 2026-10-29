# CLAUDE.md

## What this is

A fork of MPC-HC (Media Player Classic - Home Cinema), based on `clsid2/mpc-hc`
(the maintained 2.x line), not the dead `mpc-hc/mpc-hc` upstream. Work happens
on the `modernise` branch, which tracks `clsid2/develop`.

Long-term goal: extract the playback engine out of `src/mpc-hc/MainFrm.cpp`
into a core library behind a flat C ABI, then build a new WinUI 3 (C#) shell on
top of it. The MFC shell keeps running on the extracted core throughout, so
every step ships. Scope is core playback only: files, streams, DVD and Blu-ray
folders, subtitles, audio tracks, pixel shaders, playlist. Capture, BDA TV
tuner, the HTTP web server, DSM muxing, LCDUI and the DoctorDump crash reporter
are out of scope and are being removed.

Discoverability is a first-class requirement for the new shell: the existing
player has the features, users cannot find them.

Full decision record and phase plan: `docs/architecture-decision.md`.
Phase 0 (baseline build, CI, dead-code removal, seam mapping) is done; see
`docs/Baseline-Build.md` and `docs/Engine-Seam.md`. Phase 1 (engine
extraction into `mpc-core`) is under way.

## Repo facts

- C++ / MFC / Win32, DirectShow playback, LAV Filters as internal codecs.
- Builds with Visual Studio 2022 or 2026 via `build.bat`; see `docs/Compilation.md`.
- 21 git submodules. `dav1d` is on code.videolan.org, the rest on GitHub.
- `src/mpc-hc/MainFrm.cpp` is ~25k lines, one class, 650+ methods. The engine
  seam is `OnGraphNotify`, `CloseMediaInternal` and the `OpenMedia*` path.
- 20 UI source files call `IGraphBuilder` / `IBaseFilter` directly; these are
  the call sites that extraction must redirect through the core.
- `.gitattributes` pins EOL for `.sh`, `.po` and `mpc-hc.rc`. Leave
  `core.autocrlf` false in this clone.

## Working rules

- Feature branches and pull requests. Never commit directly to `modernise`.
- Conventional commits: `type(scope): description`, subject 72 chars or less.
- One concern per PR. A subsystem removal is one PR; an extracted
  responsibility is one PR.
- Verify by building. Run the build, show the output, then claim it works.
  Reliability, repeatability and observable progress over speed.
- Prefer fail-closed behaviour. If an extraction changes playback behaviour,
  stop and say so rather than papering over it.
- No code changes during read-only analysis tasks (for example seam mapping).
- Windows paths and PowerShell in any instructions written for the developer.
- AI attribution in commits and PRs is fine on this repo. Em dashes are fine.

## Build and check

```powershell
cd C:\Users\russell\Documents\git\mpc-hc
git submodule update --init --recursive --jobs 4
.\build.bat Build x64 MPCHC Release    # exe only, fastest baseline
.\build.bat Build x64 Main Release     # exe plus resource DLLs
```

Record any deviation from `docs/Compilation.md` that was needed to get a green
build in `docs/Baseline-Build.md`.
