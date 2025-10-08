# Lexgine Public API Reference

This document describes the public APIs exposed by the `api` project after CMake configuration, with concise explanations and usage examples.

- Version: see `engine/version`
- Namespace root: `lexgine::api`
- Linking model: runtime dynamic linking of exported engine functions via `HMODULE`

## Table of Contents
- Ioc
- LexgineObject
- LinkResult
- is_call_possible helpers

---

## Ioc
Header: `api/ioc.h`

Opaque base for imported objects with runtime-link infrastructure.

Key members:
- `static LinkResult link(HMODULE module)` — resolves IOC-related entry points.
- `void const* getNative() const` / `void* getNative()` — returns underlying pointer.
- `void reset()` — releases owned resources.

Constructors (protected):
- `Ioc(common::ImportedOpaqueClass ioc_name, deleter_type deleter)` — allocates backing buffer using size from engine.
- `Ioc(std::shared_ptr<Ioc> const& ptr)` — wraps a shared-owned object.
- `Ioc(Ioc* ptr)` — wraps a non-owning pointer.

Example:
```cpp
#include <api/ioc.h>
#include <api/link_result.h>

// Load engine and link Ioc symbols
HMODULE engineModule = LoadLibraryW(L"/path/to/lexgine.dll");
lexgine::api::LinkResult linkRes = lexgine::api::Ioc::link(engineModule);
if (!linkRes) {
    // handle missing symbols
}

// Construct some derived IOC type (example assumes an enum value and deleter are known)
// lexgine::api::Ioc obj{ lexgine::common::ImportedOpaqueClass::SOME_CLASS, &some_deleter };
```

---

## LexgineObject
Header: `api/lexgine_object.h`

Base class for engine objects providing identity and naming helpers.

Key members:
- `static LinkResult link(HMODULE module)` — resolves required engine calls.
- `GUID asUUID() const` — returns UUID of the object.
- `std::string getStringName() const` — returns human-friendly name.
- `void setStringName(std::string const& new_name)` — sets friendly name.
- `static uint64_t aliveEntities()` — total count of live engine entities (debug aid).

Example:
```cpp
#include <api/lexgine_object.h>

HMODULE engineModule = LoadLibraryW(L"/path/to/lexgine.dll");
auto res = lexgine::api::LexgineObject::link(engineModule);
if (!res) { /* handle error */ }

// Suppose you have a handle to an actual engine object wrapping
// lexgine::api::LexgineObject obj = ...;
// GUID id = obj.asUUID();
// obj.setStringName("Player");
// std::string name = obj.getStringName();
// uint64_t live = lexgine::api::LexgineObject::aliveEntities();
```

---

## LinkResult
Header: `api/link_result.h`

Helper to track and access dynamically linked API functions by name.

Key members:
- `LinkResult(HMODULE module)` — bind to a module handle.
- `FARPROC attemptLink(std::string const& api_to_link)` — try to resolve a function by name; caches result.
- `std::vector<std::string> getDanglingApis() const` — unresolved names.
- `std::vector<std::string> getLinkedApis() const` — resolved names.
- `explicit operator bool() const` — true iff all recorded names resolved.
- `FARPROC operator[](std::string const& api_name)` — get cached pointer or `nullptr`.

Iteration helpers: begin/end/cbegin/cend over the internal map.

Example:
```cpp
#include <api/link_result.h>

HMODULE mod = LoadLibraryW(L"/path/to/lexgine.dll");
lexgine::api::LinkResult lr{mod};
FARPROC fn = lr.attemptLink("lexgineCoreObjectGetUUID");
if (!fn) {
    auto missing = lr.getDanglingApis();
}
```

---

## is_call_possible helpers
Header: `api/is_call_possible.h`

Macros to generate compile-time checks that a type `T` exposes a method named `api_name` with a matching signature.

- `DECLARE_IS_CALL_SUPPORTED_REFLEXION(api_name)` — expands to `is_api_<api_name>_supported<T, R(Args...)>` and
  `is_api_<api_name>_supported_const<T, R(Args...)>` trait classes.
- Each trait defines `static constexpr bool value`.

Example:
```cpp
#include <type_traits>
#include <api/is_call_possible.h>

struct Foo { int bar(double) const; };

DECLARE_IS_CALL_SUPPORTED_REFLEXION(bar);

static_assert(is_api_bar_supported_const<Foo, int(double)>::value, "Foo::bar not found");
```

---

## Runtime link entrypoint (generated)
A convenience `linkLexgineApi(std::wstring const&)` is generated during configuration and placed under `build/api/runtime*` headers.
It links `lexgine::api` classes including `Ioc`, `LexgineObject`, plus exported IOcs discovered in the engine headers.

Usage sketch:
```cpp
#include <api/runtime.h>

auto map = lexgine::api::linkLexgineApi(L"/path/to/lexgine.dll");
// map["lexgine::api::LexgineObject"] holds a LinkResult for that type
```

Notes:
- The exact set of generated runtime headers depends on engine headers and tokens processed during CMake configuration.
- On non-Windows platforms, `HMODULE`/WinAPI types must be provided via compatibility shims if compiling tools that consume headers only.
