---
name: build_project
description: Configure and build lexgine with CMake for a given preset, honouring the out-of-tree build directory convention and the required x64 vcvars environment, and report the compiler diagnostics. Takes one argument - the CMake preset name (e.g. vs, clion-debug, clion-release). Use when asked to build the project, compile the engine, rebuild after a change, or check that the code still compiles.
argument-hint: "<preset>"
allowed-tools:
  - Read
  - Grep
  - Glob
  - PowerShell
  - Bash(ls *)
  - Bash(cat *)
---

# Build lexgine

Argument: `$ARGUMENTS`

## 1. Resolve the preset

The preset is the single argument. Valid configure presets live in `CMakePresets.json`
at the repository root: read it and match the argument against the non-hidden
`configurePresets` names. Currently these are `vs`, `clion-debug` and `clion-release`.

- No argument given: default to `vs`.
- Argument does not match any configure preset: list the available presets and stop.

## 2. Resolve the build directory

Build directories live **outside** the source tree, at

```
../cmake-builds/lexgine/<preset>
```

relative to the repository root, i.e. `C:/Repositories/personal/cmake-builds/lexgine/<preset>`
for a repo at `C:/Repositories/personal/lexgine`.

`CMakePresets.json` deliberately leaves `binaryDir` unset, so the build tree **must**
be passed with `-B` on every configure. The root `CMakeLists.txt` aborts with a fatal
error when the binary directory lies inside the source tree, so a forgotten `-B`
fails loudly instead of littering the repo.

## 3. Run every cmake command inside the x64 vcvars environment

**This is mandatory for every preset, for both configure and build.** Plain `cmake`
invocations from the agent's default shell do not work:

- `3rd_party/DirectXTex` has a custom build step that runs `Shaders/CompileShaders.cmd`,
  which locates `fxc.exe` through `%WindowsSdkVerBinPath%`. Without vcvars that step
  fails and takes `DirectXTex` down with it, and with it every engine target that
  depends on it - `common`, `imgui`, `texture_converter`, `api` and `swe_demo`, so
  `swe.exe` never gets built.
- The `clion-*` presets additionally pin `CMAKE_C_COMPILER`/`CMAKE_CXX_COMPILER` to
  `cl`, which is only on `PATH` inside vcvars, so their **configure** step needs it too.

Use the **PowerShell** tool. Git Bash mangles the quoting around the vcvars path, so do
not drive this from the Bash tool. Locate vcvars through `vswhere` rather than
hardcoding a Visual Studio edition or version:

```powershell
$vs = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
cmd /c "call `"$vcvars`" >nul && <cmake command>"
```

Chain several cmake commands in one `cmd /c` with `&&` so vcvars is paid for once, e.g.
configure followed immediately by the build.

`vcvars64.bat` prints `'vswhere.exe' is not recognized as an internal or external
command` to stderr on this machine. That line is **benign noise from inside vcvars** -
it still sets the environment correctly. Do not report it as a build failure.

## 4. Configure

Skip this step when `<build-dir>/CMakeCache.txt` already exists and neither
`CMakeLists.txt` nor `CMakePresets.json` nor any `CMakeLists.txt` under the source
tree changed since the cache was written; otherwise run, from the repository root:

```
cmake --preset <preset> -B ../cmake-builds/lexgine/<preset>
```

wrapped as described in step 3. When in doubt, just run it - re-configuring an
up-to-date cache is cheap.

## 5. Build

The `vs` preset is a multi-config Visual Studio generator and needs `--config`;
the `clion-*` presets are single-config Ninja builds where `--config` is ignored.

```
cmake --build ../cmake-builds/lexgine/<preset> --config <Debug|Release>
```

wrapped as described in step 3.

Choose the configuration as follows:

- `vs`: use `Debug` unless the user asked for a release build.
- `clion-debug` / `clion-release`: the configuration is baked into the preset; pass
  the matching `--config` value anyway so the command stays uniform.

`--parallel` is not needed: Ninja parallelises by default, and MSBuild already builds
projects in parallel for the `vs` preset.

Builds are long and their logs are large. Redirect the output to a file under
`$CLAUDE_JOB_DIR/tmp` (or the system temp directory when that is unset), report the
exit code, and grep the log for `error` and `warning` instead of pulling the whole
thing into context.

## 6. Report

Artifacts land in `<build-dir>/bin/<config>/`; the demo executable is `swe.exe`.

Report the outcome honestly:

- On success: state the preset, the configuration and the build directory, and
  surface any compiler **warnings** that appeared, grouped by file. Warnings coming
  from `3rd_party/` are pre-existing noise - summarise them in one line and keep the
  detail for warnings in `engine/` and the demo sources.
- On failure: show the first compiler errors verbatim with their `file:line`, do not
  summarise them away, and do not claim the build succeeded. Name the targets that
  never got built as a result. If the failure is in code touched during this session,
  fix it and rebuild; otherwise report it and ask how to proceed.

Do not run the resulting binaries as part of this skill.
