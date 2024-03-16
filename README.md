RG3
====

[![Build RG3](https://github.com/DronCode/RG3/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/DronCode/RG3/actions/workflows/build.yml)
![RG3 PyPU](https://img.shields.io/pypi/v/rg3py)

RG3 - is a backend & frontend for processing & analyzing C++ code. It provides information about types into Python frontend to future codegen stage.

Requirements
------------

Windows
-------

 * Compiled LLVM (>= 16.0.0) on local machine and environment variables `CLANG_DIR` and `LLVM_DIR`
   * My `Clang_DIR` is `B:/Projects/llvm/build/lib/cmake/clang`
   * My `LLVM_DIR` is `B:/Projects/llvm/build/lib/cmake/llvm`
 * Statically compiled Boost (>=1.81.0) with boost python and environment variable `BOOST_ROOT`
   * My `BOOST_ROOT` is `B:/Projects/Boost/build_binaries`
 * Python 3.10 (or later) with development files
 * Installed clang instance (allowed to have latest stable version)

Linux
------

Tested on Ubuntu (apt based systems)

 * Compiled LLVM (compiled manually)
 * Boost >= 1.81 (`sudo apt-get install libboost-dev libboost-system-dev libboost-filesystem-dev libboost-python-dev`). **NOTE:** You able to install boost and link it dynamically, but official rg3py build uses own compiled boost for static linkage with `-fPIC` flag.
 * Python 3.10 (or later) with development files (`sudo apt-get install python3.10 python3.10-dev python3.10-venv`) (**venv is optional**)
 * Installed gcc instance (for Windows required `clang`, but for Linux `gcc` is enough). Warning: **GCC-13** is a minimum supported version for CI builds!
 * CMake & Ninja

macOS
------

Tested on macOS
 * Compiled LLVM (compiled manually, see .github/workflows/build.yml for details)
 * Boost >= 1.81
 * Homebrew (see dependencies inside .github/workflows/build.yml)
 * Python 3.10 or later installed with [PyEnv](https://github.com/pyenv/pyenv)
 * Installed & configured XCode (tested on 15.x, check that XCode able to build projects and available via `xcrun`)
 * CMake & Ninja (installed via Homebrew)

Build
-----

**Recommended way:** Since version 0.0.1 package is available on [PyPI](https://pypi.org/project/rg3py/) and you able to install it 

```
pip install rg3py
```

**Build from source code:**

```shell
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
```

When everything is done your plugin file `rg3py.pyd` or `rg3py.so` will be in `build` folder. Use **.github/workflows/build.yml** file as reference to build RG3 on your platform.

Usage:
------

Copy binaries or install [PyPI package](https://pypi.org/project/rg3py/)

Sample code:

```python
from rg3py import CodeAnalyzer, CppStandard, CppCompilerIssueKind

analyzer: CodeAnalyzer = CodeAnalyzer.make()
analyzer.set_code("""
    namespace my::cool::name_space {
        /**
        * @runtime
        **/
        enum class ECoolEnum : int {
            CE_FIRST_ENTRY = 0,
            CE_ANOTHER_ENTRY = 0xFFEE,
            CE_DUMMY = 256
        };
    }
    """)

analyzer.set_cpp_standard(CppStandard.CXX_17)
analyzer.analyze()

for t in analyzer.types:
    print(f"We have a type {t.pretty_name} ({t.kind})")
```

and output will be
```text
We have a type my::cool::name_space::ECoolEnum (TK_ENUM)
```

Project state
-------------

Now project ready to discover medium code bases. Supported STL discover on Windows via installed clang (it's required!).

Feature Checklist
-----------------

 - [x] Implement multithread for analyzer
 - [ ] Add support of async operations on Python side
 - [ ] Support template deduction & aliasing
 - [x] Implement integration tests & embed it into GitHub Actions
 - [x] Support macOS & OSX Frameworks (lookup via `xcrun`)


Current limitations
-------------------

 * Project WILL NOT support code inside function scope. I'm focused only on header analysis. Feel free to fork with project and make which analysis what you want.
