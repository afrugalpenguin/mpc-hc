# MPC-HC modernisation: architecture decision record

Status: accepted, 29 Oct 2026
Local clone: C:\Users\russell\Documents\git\mpc-hc, branch `modernise` tracking `clsid2/develop`

## 1. Current state

### Lineage
- The original clone was `mpc-hc/mpc-hc` (upstream), final commit `7f14b47cf` on 7 Feb 2018. The project was abandoned at that point.
- The maintained continuation is `clsid2/mpc-hc` (MPC-HC 2.x, release 2.8.2 in Sep 2026, develop branch ahead of that). Its default branch is `develop`; `master` is releases only.
- `merge-base` of the two is exactly the 2018 commit, so upstream is a pure ancestor. clsid2 is 3,390 commits ahead (+370k / -527k lines across 2,101 files; most of the delta is deletions in `thirdparty` and `include`).
- Decision: work from `clsid2/develop`. Redoing five years of maintenance on the 2018 tree has no value.

### Stack (as of clsid2/develop, Oct 2026)
- C++ / MFC / Win32. Builds with VS 2019, 2022 or 2026. Windows 8.1 SDK optional for Win7 RTM compatibility. DirectX SDK no longer required. MSYS2 only if building LAV Filters from source (prebuilt binaries are an option). Python only for translations.
- Playback is DirectShow end to end. `FGManager` builds the filter graph; LAV Filters (ffmpeg) demux and decode; in-tree allocator presenters hook EVR / VMR9 / Sync and present via Direct3D 9. MPC Video Renderer (external, D3D11) is the recommended output.
- Subtitles: legacy VSFilter lineage (`src/Subtitles`, `src/SubPic`) plus libass + freetype + harfbuzz + fribidi.
- 21 git submodules (LAV Filters pulls ffmpeg, dav1d, libxml2, speex; plus libass stack, lcms2, soxr, SoundTouch, minhook, rapidjson, tinyxml2, nanosvg, stb, a `tests` submodule). `dav1d` is hosted on code.videolan.org, not GitHub.
- 42 translations as gettext PO compiled to resource DLLs.

### Shape of the application code
- `src/mpc-hc`: 196 .cpp files, 138k lines including headers.
- `MainFrm.cpp`: 25,654 lines (upstream 2018 had 16,984; it has grown). One class, 651 methods, 236 member fields, 357 message-map entries, 86 includes. Longest functions: `CloseMediaInternal` 555 lines, `OnTimer` 494 lines (every periodic task is a case in one switch), `OnGraphNotify` 444 lines (the DirectShow event pump, inside the window class).
- 20 UI source files call `IGraphBuilder` / `IBaseFilter` directly. There is no engine / UI boundary; the main window is the player.
- Nothing in the playback path is unit testable without a window.

### Dead weight still present
DoctorDump crash reporter, Logitech LCDUI, BDA TV tuner capture, DSM muxer, built-in HTTP remote-control web server (`WebServer.cpp`, `WebClientSocket.cpp`, `res/web`; source of several historical CVEs). Upstream's QuickTime / RealMedia / Shockwave graphs and DeCSS are already gone in clsid2.

### Product observation
The maintainer's own issue template states that most requests are for functionality that already exists. That is a discoverability failure: roughly 40 options pages and a menu tree unchanged since the mid-2000s. Everything is in the player; little of it is findable. This is a design goal for the new shell, not just a UI toolkit swap.

## 2. Options considered

### A. Toolchain modernisation only
Keep MFC and DirectShow; CMake, CI, delete dead subsystems.
- For: weeks of work, mechanical, zero user-visible risk, a prerequisite for anything else.
- Against: on clsid2 this is already done apart from CMake, CI and dead-code deletion. Does not touch `MainFrm`, so nothing becomes testable or maintainable. Demonstrates maintenance, not design.

### B. Engine extraction, then a new shell (strangler fig)
Carve the playback engine (graph lifecycle, renderers, subtitle pipeline, settings, events) out of `MainFrm` into a core library with a stable API. Keep the MFC shell running on top of it so every step ships. Then build a new shell against the same core.
- For: keeps the DirectShow filter ecosystem (LAV, madVR, MPC Video Renderer, xy-VSFilter), which is the reason MPC-HC still exists. Shippable at every stage. Produces a playback library that is testable without a window and reusable from any UI. Nobody else is doing this; the maintainer has explicitly declined to.
- Against: months of work. Untangling `MainFrm` is unglamorous. DirectShow renderers are HWND-aware, so the boundary leaks a window handle whatever the design. Two shells coexist for a long stretch. Windows App SDK has rough edges (packaging, D3D interop, no docking for free).

### C. Rewrite on Media Foundation + D3D11/12
- For: modern Microsoft stack, OS-provided HDR and hardware decode, fast to get a file playing.
- Against: loses every DirectShow filter and most of the format coverage and quality. MF decoders are far weaker than LAV; ffmpeg would have to be bolted back in by hand. No ASS renderer on that stack near VSFilter or libass quality. Not a port of MPC-HC; a new player competing with mpv, with almost nothing reused. Longest timeline, highest risk.

## 3. Decision

- Direction: **B**, with the useful parts of A as phase 0.
- Base: `clsid2/develop`.
- Scope: **core playback only**. Files, streams, DVD and Blu-ray folder playback, subtitles, audio tracks, pixel shaders, playlist. Drop capture, BDA TV tuner, web server, DSM muxing, LCDUI, DoctorDump.
- Engine: 100% C++, exposed through a **flat C ABI** (not C++/WinRT projections of DirectShow types). Surface: open, play, pause, stop, seek, rate, track enumeration and selection, subtitle control, event callbacks, and a presentation target (HWND / swap chain) supplied by the shell. No real-time path crosses the interop boundary.
- New shell: **WinUI 3 in C#** over P/Invoke to the C ABI. Reason: the engine surface is small and stable, P/Invoke over C is boring and reliable, and UI iteration is far faster in C# than C++/WinRT. Video presented into a `SwapChainPanel` via a D3D11 presenter.
- UX goals for the new shell: feature parity with the retained scope; searchable settings; a command palette; sensible defaults; a menu structure designed rather than accreted. Discoverability is a first-class requirement.
- Engineering rules: feature branches and PRs, conventional commits, CI green before merge, verification by running the build and the tests rather than by assertion.

## 4. Phase plan

### Phase 0: baseline (weeks)
1. Initialise submodules (`dav1d` must be fetched from a shell with access to code.videolan.org).
2. Confirm a clean build on the local VS install; record exact steps. This is the known-good baseline.
3. GitHub Actions CI building x64 Release on every PR.
4. Delete out-of-scope subsystems: web server, capture, BDA, DSM muxer, LCDUI, DoctorDump. Each deletion its own PR, CI green.
5. Decide on CMake vs keeping vcxproj. Not a blocker; vcxproj is acceptable if CI is green.

Outcome (29 Oct 2026): done. Baseline in `docs/Baseline-Build.md` (#2), CI on PRs to `modernise` (#4), removals #6 (web server), #8 (BDA and analog capture together, since they share the Open Device path), #10 (all muxers and BufferFilter, whose only user was capture), #12 (LCDUI), #14 (DoctorDump), and the seam map `docs/Engine-Seam.md` (#16), which also covers phase 1 step 1. The removal PRs were stacked and merged before CI existed on `modernise`, so CI ran once on the merged result rather than on each PR. DVD and Blu-ray playback were not yet tested at runtime after the removals. vcxproj kept (see open questions).

### Phase 1: engine extraction (months)
1. Identify the seam: `OnGraphNotify`, `CloseMediaInternal`, the open-file path (`OpenMedia` / `OpenMediaPrivate` and friends), and the state they share. Map every `MainFrm` member that the graph code touches.
2. Introduce `mpc-core` as a static library inside the solution. Move graph lifecycle, renderer selection, subtitle pipeline and playback state behind a C++ interface first. MFC shell calls the interface; behaviour unchanged. One extracted responsibility per PR.
3. Replace direct `IGraphBuilder` / `IBaseFilter` use in the 20 UI files with calls into the core.
4. Add the C ABI over the C++ interface. Add a headless test harness (open a file, play, seek, close) running in CI against a tiny sample set.
5. Exit criterion: MFC shell contains no DirectShow calls; the core plays media with no window other than the presentation target.

### Phase 2: new shell (months)
1. WinUI 3 / C# project consuming the C ABI. Minimal viable player: open, play, seek, tracks, subtitles, fullscreen.
2. Settings model redesigned around search and sensible defaults; migrate the retained options from the ~40 MFC pages.
3. Command palette; keyboard map import from existing accelerator tables.
4. Parity audit against the retained scope. Retire the MFC shell when parity is reached.

## 5. Open questions
- CMake or vcxproj for the long term. *Decided for now (29 Oct 2026, phase 0 step 5): keep vcxproj; `mpc-core` is added as a vcxproj in the existing solution. Revisit when the WinUI 3 shell lands.*
- Whether libass or the VSFilter lineage becomes the single subtitle renderer in the new shell, or both are retained.
- Packaging: MSIX vs unpackaged WinUI 3, and the installer story.
- Whether to keep the 42 translations in the new shell from day one or introduce them after parity.

## 6. Local housekeeping notes
- The original clone had 602 files showing as modified that were CRLF-only churn (index LF, working tree CRLF, no .gitattributes rule). Fixed by `git config core.autocrlf false` and `git checkout -- .`. Global `core.autocrlf` is unset; the cause was probably the tool that made the clone.
- Remotes: `origin` = mpc-hc/mpc-hc (dead), `clsid2` = clsid2/mpc-hc (live). Consider repointing `origin` to the personal fork when it exists.
