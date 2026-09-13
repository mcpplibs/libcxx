# libcxx (source package)

libc++ and libc++abi, **compiled with the consuming program's own flags**, for a
hosted target whose toolchain payload cannot supply a static libc++ built for
that target.

```toml
[target.'cfg(os = "ios")'.dependencies]
llvm.libcxx               = "22.1.8.1"
llvm.compiler-rt-builtins = "22.1.8.5"
```

mcpp reports `c++-abi libc++ (llvm.libcxx@22.1.8.1, graph)`, links the program
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

## Where it is measured

`examples/import-std` is a program that imports `std` and exercises the two
paths above (`std::unordered_map<std::string, int>` and
`std::atomic<int>::notify_all`). The workflow builds it on Linux with
`llvm@22.1.8` against glibc, which is the combination the engine change was
first measured on (a prebuilt C library under a graph C++ runtime), and on a
macOS runner for `aarch64-ios-sim`, where it runs under the simulator.

Design record: mcpp-community/mcpp, `.agents/docs/2026-09-13-630-*.md`, §5.
