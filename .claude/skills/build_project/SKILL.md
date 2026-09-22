---
name: build_project
description: Configure and build lexgine with CMake for a given preset, honouring the out-of-tree build directory convention, and report the compiler diagnostics. Takes one argument - the CMake preset name (e.g. vs, clion-debug, clion-release). Use when asked to build the project, compile the engine, rebuild after a change, or check that the code still compiles.
argument-hint: "<preset>"
allowed-tools:
  - Read
  - Grep
  - Glob
  - Bash(cmake *)
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

## 3. Configure

Skip this step when `<build-dir>/CMakeCache.txt` already exists and neither
`CMakeLists.txt` nor `CMakePresets.json` nor any `CMakeLists.txt` under the source
tree changed since the cache was written; otherwise run, from the repository root:

```
cmake --preset <preset> -B ../cmake-builds/lexgine/<preset>
```

When in doubt, just run it - re-configuring an up-to-date cache is cheap.

## 4. Build

The `vs` preset is a multi-config Visual Studio generator and needs `--config`;
the `clion-*` presets are single-config Ninja builds where `--config` is ignored.

```
cmake --build ../cmake-builds/lexgine/<preset> --config <Debug|Release>
```

Choose the configuration as follows:

- `vs`: use `Debug` unless the user asked for a release build.
- `clion-debug` / `clion-release`: the configuration is baked into the preset; pass
  the matching `--config` value anyway so the command stays uniform.

`--parallel` is not needed: Ninja parallelises by default, and MSBuild already builds
projects in parallel for the `vs` preset.

## 5. Report

Artifacts land in `<build-dir>/bin/<config>/`; the demo executable is `swe.exe`.

Report the outcome honestly:

- On success: state the preset, the configuration and the build directory, and
  surface any compiler **warnings** that appeared, grouped by file.
- On failure: show the first compiler errors verbatim with their `file:line`, do not
  summarise them away, and do not claim the build succeeded. If the failure is in
  code touched during this session, fix it and rebuild; otherwise report it and ask
  how to proceed.

Do not run the resulting binaries as part of this skill.
