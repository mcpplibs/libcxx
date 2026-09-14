# libcxx (source package)

libc++ and libc++abi, **compiled with the consuming program's own flags**, for a
hosted target whose toolchain payload cannot supply a static libc++ built for
that target.

```toml
[target.'cfg(os = "ios")'.dependencies]
llvm.libcxx               = "22.1.8.3"
llvm.compiler-rt-builtins = "22.1.8.5"
```

mcpp reports `c++-abi libc++ (llvm.libcxx@22.1.8.3, graph)`, links the program
with `-nostdlib++`, and the artefact carries no reference to the system's
`libc++.1.dylib`. The version is upstream's: every file under `llvm/` is
byte-identical to `llvmorg-22.1.8` (`llvm/UPSTREAM-REV`). The fourth segment is
the packaging revision, and it is written out in full because a bare
requirement in mcpp is an exact pin rather than a caret.

## The case that created it

mcpp's LLVM payload ships libc++ headers, a std module source and static
archives built for the machine the payload runs on. On the iOS rows the
archives are macOS objects and ld64 refuses them, so the engine linked the
SDK's libc++ under the payload's headers. The two are different releases
(libc++ 22 against libc++ 19 on Xcode 16.4), and an inline function in the
newer headers referenced a symbol the older dylib does not export:

```
Undefined symbols for architecture arm64:
  "std::__1::__hash_memory(void const*, unsigned long)"
  "std::__1::__atomic_notify_all_global_table"
```

A standard library is configured for one C library and compiled against its
headers; a program that merely finds headers is not the same thing. This
package carries the whole library, so the headers a translation unit is
compiled against, the module it imports and the objects it links are one
release by construction. The C library beneath stays the target's own: the
SDK's libSystem on Apple platforms, glibc or musl on Linux.

## What is here

| | |
| --- | --- |
| `llvm/libcxx`, `llvm/libcxxabi` | upstream, unmodified |
| `llvm/libc/shared`, `llvm/libc/src/__support` | llvm-libc's floating-point utilities, which libc++'s `from_chars` includes |
| `generated/__config_site` | libc++'s configure product, with the values the official 22.1.8 payload was built with |
| `generated/__assertion_handler` | upstream's default handler, verbatim |
| `generated/std.cppm`, `generated/std.compat.cppm` | the module sources, configured from upstream's templates |

libunwind is not carried. On Apple platforms the unwinder is libSystem's; on
Linux the payload's is linked by the driver.

## Two copies of libc++ in one process

Every framework on an Apple platform loads the system's libc++. This package's
objects are compiled with hidden visibility and with libc++'s own visibility
annotations disabled, so the two copies are kept apart by the dynamic loader
rather than unified; mcpp's `graph_runtime_compile_flags` applies the same
visibility to every unit of the graph on Mach-O. C++ standard-library objects
do not cross the boundary to a system framework, and Apple's public interfaces
carry none.

## The language level of its own sources

libc++ 22 is written for C++23 and upstream compiles it at that level; its
consumers may be at any level. `[package] standard = "c++23"` states this, and
from mcpp 2026.9.15.2 the engine compiles this package's implementation units at
c++23 while the std module stays at the consuming graph's level, so a program at
`standard = "c++20"` imports `std` over this package. An engine before
2026.9.15.2 compiles every unit at the graph's level, and a c++20 program then
fails inside `src/filesystem/format_string.h` (`resize_and_overwrite`).

## A shared library above this package

This package's objects are linked into the program. A dependency built as a C++
shared library cannot use the program's copy, because the copy is hidden, and
mcpp 2026.9.15.2 refuses that build before compiling. The program states a
private copy for shared libraries to lift the refusal:

```toml
[build]
cxx_runtime = { shared = "self-contained" }
```

Each shared library then links this package's objects itself. Each image holds
its own type information for libc++'s classes, so an exception of a standard
library class thrown in the shared library is not caught by that class in the
program; a class the shared library defines is.

## Where it is measured

`examples/import-std` is a program that imports `std` and exercises the two
paths above (`std::unordered_map<std::string, int>` and
`std::atomic<int>::notify_all`). The workflow builds it on every row the
package claims, and runs it where the runner can:

| row | runner | what is asserted |
| --- | --- | --- |
| `x86_64-linux-gnu` (glibc) | ubuntu-24.04 | the report names this package as the C++ layer, `ldd` lists no libc++, the program prints `1-2-3` |
| `aarch64-macos` (native) | macos-15 | the same, with `otool -L` in place of `ldd` |
| `aarch64-ios-sim` | macos-15 | the report, no libc++ in the load commands, and the program runs under the simulator through `simctl-run` |
| `x86_64-ios-sim` | macos-15 | the report, `LC_BUILD_VERSION` names the simulator platform, no libc++ in the load commands (the host cannot run it) |
| `aarch64-ios` | macos-15 | the report, `LC_BUILD_VERSION` names iOS, no libc++ in the load commands (a device build cannot be run without a signature) |

`examples/import-std-cxx20` is the same program at `standard = "c++20"`; it is
built on every row and run where the runner can. `examples/shared-dependency`
is a program over a shared library dependency with a private copy stated; it is
built and run on `x86_64-linux-gnu` and `aarch64-macos`, and the shared library
is asserted to carry no undefined libc++ reference and no libc++ dependency.

Windows is not claimed (`[package] platforms`): libc++ over the MSVC runtime
takes a configuration this package does not carry. A `workflow_dispatch` probe
job measures that row on request and is not a gate. Its reading under mcpp
2026.9.14.1 on `x86_64-pc-windows-msvc`: the report names this package as the
C++ layer, and the std module precompile stops at
`invalid exception model 'dwarf' for target 'x86_64-pc-windows-msvc'`, because
the engine's graph-runtime flags select DWARF exceptions on PE, which the MSVC
target refuses. Claiming Windows would therefore need an engine change as well
as the package's own Windows configuration.

Design record: mcpp-community/mcpp, `.agents/docs/2026-09-13-630-*.md`, §5.
