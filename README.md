RG3
====

![Build Status](https://github.com/DronCode/RG3/actions/workflows/build.yml/badge.svg)

RG3 - is a backend & frontend for processing & analyzing C++ code. It provides information about types into Python frontend to future codegen stage.

Requirements
------------

Windows
-------

 * Compiled LLVM (>= 16.0.0) on local machine and environment variables `CLANG_DIR` and `LLVM_DIR`
   * My `CLANG_DIR` is `B:/Projects/llvm/build/lib/cmake/clang`
   * My `LLVM_DIR` is `B:/Projects/llvm/build/lib/cmake/llvm`
 * Statically compiled Boost (>=1.81.0) with boost python and environment variable `BOOST_ROOT`
   * My `BOOST_ROOT` is `B:/Projects/Boost/build_binaries`
 * Python 3.10 (or later) with development files
 * Installed clang instance (allowed to have latest stable version)

Linux
------

Tested on Ubuntu (apt based systems)

 * Compiled LLVM (compiled manually)
 * Boost >= 1.81 (`sudo apt-get install libboost-dev libboost-system-dev libboost-filesystem-dev libboost-python-dev`)
 * Python 3.10 (or later) with development files (`sudo apt-get install python3.10 python3.10-dev python3.10-venv`) (**venv is optional**)
 * Installed gcc instance (for Windows required `clang`, but for Linux `gcc` is enough)
 * CMake & Ninja

Build
-----

```shell
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
```

When everything is done your plugin file `rg3py.pyd` will be in `build` folder.

Usage:
------

Just copy `rg3py.pyi` (hints for IDE) and `rg3py.pyd` (native extension itself) into folder with your python code.

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

 - [ ] Implement multithread for analyzer
 - [ ] Add support of async operations on Python side
 - [ ] Add releases from CI jobs into PyPi
 - [ ] Support template deduction & aliasing
 - [ ] Implement integration tests & embed it into GitHub Actions
 - [ ] Support macOS & OSX Frameworks (lookup via `xcrun`)
 - [ ] Support Linux & support GCC as source for SDK paths


Current limitations
-------------------

 * Currently I'm focused on Windows support. Linux & MacOS support will be later.
 * Project WILL NOT support code inside function scope. I'm focused only on header analysis. Feel free to fork with project and make which analysis what you want.
 * Multithreading: not supported now, but will be supported later.
 * Context: not supported. Will do it later. 
   * Workaround: main context focused in CodeAnalyzer, so you able to make 1 context per thread, that should be enough. 
