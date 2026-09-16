[![CI build/master](https://github.com/alexbibov/lexgine/actions/workflows/cmake-single-platform.yml/badge.svg)](https://github.com/alexbibov/lexgine/actions/workflows/cmake-single-platform.yml)

# lexgine
3D graphics rendering framework based on Direct3D 12

## Building

The build is driven by CMake presets (`CMakePresets.json`), one per IDE:

| Preset | Generator | Build directory |
| --- | --- | --- |
| `vs` | Visual Studio 18 2026, x64 | `build/vs` (`Debug` and `Release` in one solution) |
| `clion-debug` | Ninja | `build/clion-debug` |
| `clion-release` | Ninja | `build/clion-release` |

Every configuration writes its artifacts into a configuration-specific
sub-directory of its build tree: executables and shared libraries land in
`bin/<Config>`, import and static libraries in `bin/<Config>/lib`. Debug and
release artifacts therefore never mix, even for the multi-configuration
Visual Studio solution.

### Visual Studio

```
cmake --preset vs
```

Open `build/vs/lexgine.sln` and pick the `Debug` or `Release` configuration, or
build from the command line:

```
cmake --build --preset vs-debug
cmake --build --preset vs-release
```

Both commands must run from a Visual Studio developer command prompt, because
the DirectXTex shader compilation step requires `fxc.exe` from the Windows SDK.

### CLion

Enable the `clion-debug` and `clion-release` presets in
*Settings | Build, Execution, Deployment | CMake*; CLion picks them up from
`CMakePresets.json` and uses its bundled Ninja.

From the command line, in an x64 developer shell
(`Launch-VsDevShell.ps1 -Arch amd64 -HostArch amd64`, since the Developer
PowerShell defaults to x86):

```
cmake --preset clion-debug
cmake --build --preset clion-debug
```

### Tests

```
ctest --preset vs-debug
```

A test preset exists for every build preset.

Add machine-local presets to `CMakeUserPresets.json`, which is not tracked --
for example to inherit from `vs` with a different Visual Studio generator.
