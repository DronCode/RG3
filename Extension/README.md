RG3
====

RG3 - is a backend & frontend for processing & analyzing C++ code. It provides information about types into Python frontend to future codegen stage.
We've using LLVM as our backend to prepare code. 

See [our GitHub](https://github.com/DronCode/RG3) for more details.

Install
--------

Make sure that your system has clang (any version):
 * **macOS**: you need to install XCode (tested on 15.x but should work everywhere)
 * **Window**: you need to install clang 17.x or later and add it into PATH 
 * **Linux**: gcc & g++ at least 13 version (temporary limitation, planned to fix at 0.0.4)
 * **Other platforms & archs**: Contact us in [our GitHub](https://github.com/DronCode/RG3).

It's a better way to use RG3 inside virtualenv:
```shell
python3 -m venv venv
source ./venv/bin/activate
pip install rg3py
```

Usage:
------

Sample code analyze code from buffer:

```python
from rg3py import CodeAnalyzer, CppStandard, CppCompilerIssueKind, CppTypeKind, CppClass, FunctionArgument
from typing import List


def prepare_args(args: List[FunctionArgument]) -> str:
    return ','.join(f'{arg.type_info.get_name()} {arg.name}' for arg in args)


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

        /// @runtime
        struct MyGeniusStruct
        {};

        struct SomeThirdPartyStruct
        {
            float fProperty = 42.f;
            MyGeniusStruct sGenius {};

            static bool IsGeniusDesc(bool bCanReplace) const;
        };

        template <typename T> struct ThirdPartyRegistrator {};
        template <> struct 
            __attribute__((annotate("RG3_RegisterRuntime")))
            __attribute__((annotate("RG3_RegisterField[fProperty:Property]")))
            __attribute__((annotate("RG3_RegisterField[sGenius:GeniusData]")))
            __attribute__((annotate("RG3_RegisterFunction[IsGeniusDesc]")))
        ThirdPartyRegistrator<SomeThirdPartyStruct>
        {
            using Type = SomeThirdPartyStruct;
        };
    }
    """)

analyzer.set_cpp_standard(CppStandard.CXX_17)
analyzer.analyze()

for t in analyzer.types:
    print(f"We have a type {t.pretty_name} ({t.kind})")

    if t.kind == CppTypeKind.TK_STRUCT_OR_CLASS:
        as_class: CppClass = t
        for prop in as_class.properties:
            print(f"\tProperty {prop.name} (aka {prop.alias}) of type {prop.type_info.get_name()}")

        for func in as_class.functions:
            args_as_str: str = prepare_args(func.arguments)
            print(f"\tFunction {'static' if func.is_static else ''} {func.return_type.get_name()} {func.name}({args_as_str}){' const' if func.is_const else ''}")
```

expected output is
```text
We have a type my::cool::name_space::ECoolEnum (TK_ENUM)
We have a type my::cool::name_space::MyGeniusStruct (TK_STRUCT_OR_CLASS)
We have a type my::cool::name_space::SomeThirdPartyStruct (TK_STRUCT_OR_CLASS)
	Property fProperty (aka Property) of type float
	Property sGenius (aka GeniusData) of type MyGeniusStruct
	Function static bool IsGeniusDesc(bool bCanReplace)
```

Features
---------

 * Supported Windows (x86_64), Linux (x86_64) and macOS (x86_64 and ARM64)
 * Supported C++03, 11, 14, 17, 20, 23 (26 in theory, need to migrate to next LLVM)
 * Supported threads in analysis on native side (see Tests/PyIntegration/test.py test: **test_analyzer_context_sample** for example)
 * Statically linked, no external dependencies (except Clang instance on machine)
 * Special macro definitions to hide unnecessary code
 * Template specializations reporting
 * Anonymous registration without changes in third party code

Current limitations
-------------------

Project focused on work around C/C++ headers (C++ especially). Feel free to fork project & add support of anything what you want :)

Third Party libraries
----------------------

 * LLVM 16.0.4 - our main backend of C++ analysis
 * Boost 1.81.0 - python support & process launcher
 * FMT - string formatter
 * googletest - for internal unit testing
 * pytest - for python side unit testing