# abbey
Foundational utilities for C++ applications.

## Building
Requires CMake, Ninja, and MSVC (all included with Visual Studio Build Tools).
Run from a **Developer PowerShell for VS** (so `cl` is on the PATH):

```powershell
cmake --preset debug          # configure (once, or after editing CMakeLists.txt)
cmake --build --preset debug  # build into build/debug/
ctest --preset debug          # run the tests
```

Use `release` instead of `debug` for an optimized build.

## Contributing
This is a personal project and I'm not accepting pull requests. You're welcome
to fork it, and bug reports via issues are appreciated.