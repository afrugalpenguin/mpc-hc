# Baseline build

A verified, repeatable x64 Release build of the `modernise` branch, and every deviation from
[Compilation.md](Compilation.md) that was needed to get it green.

Verified on 2026-10-29 at commit `9d9f6fbf0` (`clsid2/develop`).

## Result

```text
.\build.bat Build x64 MPCHC Release
```

- Exit code 0.
- `bin\mpc-hc_x64\mpc-hc64.exe`, `mpciconlib.dll`, `LAVFilters64\LAV{Audio,Video,Splitter}.ax`
  and 44 `Lang\mpcresources.*.dll`.
- `bin\logs\mpc-hc_errors_Release_x64.log` empty.
- Clean build about 6 minutes (LAV Filters' ffmpeg is most of it). An incremental rebuild with
  nothing changed takes about 22 seconds.

## Toolchain

| Component | Version | Notes |
|---|---|---|
| Visual Studio Build Tools 2022 | 17.14.21 | Not the full IDE. MSVC 14.44.35207, toolset v143. |
| VS components | | C++ build tools x86/x64, ATL, **MFC** (`VC.ATLMFC`), **Clang** (`VC.Llvm.Clang`, `VC.Llvm.ClangToolset`) |
| clang-cl | 19.1.5 | Shipped with the VS components above. ffmpeg compiles with it when present. |
| Windows SDK | 10.0.26100.0 | The 8.1 SDK that `src\platform.props` defaults to is **not** installed. |
| NASM | 3.02 | `winget install NASM.NASM` |
| Git for Windows | 2.51.2 | |
| Python | 3.14.0 + `polib` 1.2.0 | Needed even for the `MPCHC` config, see below. |

Adding the missing VS components to an existing Build Tools install (run from an elevated
PowerShell; `--passive` fails with exit code 5007 if it is not elevated from the start):

```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\Installer\setup.exe" modify `
  --installPath "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools" `
  --add Microsoft.VisualStudio.Component.VC.ATLMFC `
  --add Microsoft.VisualStudio.Component.VC.Llvm.Clang `
  --add Microsoft.VisualStudio.Component.VC.Llvm.ClangToolset `
  --passive --norestart
```

## Steps

```powershell
cd C:\Users\russell\Documents\git\mpc-hc
git submodule update --init --recursive --jobs 4
winget install --id NASM.NASM -e
python -m pip install --user polib
# create build.user.bat (below)
.\build.bat Build x64 MPCHC Release
```

The `tests` submodule stays uninitialised; it is `update = none` in `.gitmodules`.

### build.user.bat

```bat
@ECHO OFF
SET "MPCHC_VS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools"
SET "MPCHC_WINSDK_VER=10.0.26100.0"
SET "MPCHC_LAV_TOOLCHAIN=MSVC"
SET "PATH=%LOCALAPPDATA%\bin\NASM;%PATH%"
```

Save it with CRLF line endings.

## Deviations from Compilation.md

1. **`MPCHC_VS_PATH` is required with Build Tools.** `common.bat` `:SubVSPath` calls `vswhere`
   without `-products *`, and `vswhere` only searches Community, Professional and Enterprise by
   default. A Build Tools install is never found, and `build.bat` fails at once with
   "Not all build dependencies were found". Setting `MPCHC_VS_PATH` bypasses detection.
2. **Windows SDK 10.0.26100.0 instead of 8.1.** Compilation.md Part A step 2 allows a Windows 10
   SDK. The cost is that binaries need Windows 7 SP1 or later, which does not matter for this
   fork. GitHub's `windows-2022` runners do not have the 8.1 SDK either.
3. **`MPCHC_LAV_TOOLCHAIN=MSVC` when MSYS2 is installed.** `build_lavfilters.bat` picks the GCC
   ffmpeg path whenever `C:\msys64\mingw64\bin\gcc.exe` exists. A stock MSYS2 gcc is not the
   toolchain Part H describes, so pin MSVC (which uses clang-cl when installed).
4. **NASM is not on `PATH` after `winget`.** The per-user winget install puts it in
   `%LOCALAPPDATA%\bin\NASM` without changing `PATH`; `build.user.bat` adds it.
5. **Python and `polib` are needed for the `MPCHC` config.** `build.bat` runs `:SubResources`
   for every config except `Main`, and that syncs translations through
   `src\mpc-hc\mpcresources\sync.py`. Without `polib` the player builds and then the overall
   build fails with `ModuleNotFoundError: No module named 'polib'`. Compilation.md presents this
   as optional (Part F). Python 3.14 from `PATH` works; `MPCHC_PYTHON` is not needed.

## Known side effects and pitfalls

- **The translation sync rewrites tracked files.** After a build, all 45 files in
  `src\mpc-hc\mpcresources\PO\` show as modified. Two causes: they are rewritten with line endings
  that differ from the CRLF that `.gitattributes` pins for `*.po` (so every line differs;
  `git diff --ignore-cr-at-eol` shows only the real changes), and upstream
  has added strings to `mpc-hc.rc` without regenerating the `.po` files (about 6 new entries
  each). Discard them before committing:

  ```powershell
  git checkout -- src\mpc-hc\mpcresources\PO
  ```

- **`NoDefaultCurrentDirectoryInExePath`.** If this environment variable is set, cmd does not
  search the current directory, and `common.bat`'s relative `CALL "build.user.bat"` fails with
  "'build.user.bat' is not recognized". Clear it for the build shell:

  ```powershell
  Remove-Item Env:NoDefaultCurrentDirectoryInExePath -ErrorAction SilentlyContinue
  ```

- The clean build writes about 720 lines to `bin\logs\mpc-hc_warnings_Release_x64.log`. The
  log is overwritten on each build, so an incremental rebuild leaves it empty.

## Follow-ups

- `common.bat` `:SubVSPath`: pass `-products *` to `vswhere` so Build Tools is detected.
- Stop the translation sync dirtying the tree on a normal build (fix its line endings, or
  regenerate and commit the `.po` files once).
