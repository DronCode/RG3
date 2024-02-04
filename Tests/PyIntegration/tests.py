from typing import Optional
import pytest
import rg3py


def test_code_analyzer_base():
    analyzer: rg3py.CodeAnalyzer = rg3py.CodeAnalyzer.make()

    analyzer.set_code("""
    /// @runtime
    enum class MyCustomEnum { ENT_0 = 0, ENT_1 = 1, ENT_2 = 2 };
    """)
    analyzer.set_cpp_standard(rg3py.CppStandard.CXX_17)
    analyzer.analyze()

    assert len(analyzer.issues) == 0
    assert len(analyzer.types) == 1

    type0: rg3py.CppBaseType = analyzer.types[0]
    assert type0.kind == rg3py.CppTypeKind.TK_ENUM

    enum0: rg3py.CppEnum = type0
    assert enum0.is_scoped
    assert len(enum0.entries) == 3

    enum0_e0: rg3py.CppEnumEntry = enum0.entries[0]
    assert enum0_e0.name == "ENT_0"
    assert enum0_e0.value == 0

    enum0_e1: rg3py.CppEnumEntry = enum0.entries[1]
    assert enum0_e1.name == "ENT_1"
    assert enum0_e1.value == 1

    enum0_e2: rg3py.CppEnumEntry = enum0.entries[2]
    assert enum0_e2.name == "ENT_2"
    assert enum0_e2.value == 2

    assert len(type0.tags.items) == 1
    assert 'runtime' in type0.tags

    tag0: rg3py.Tag = type0.tags.get_tag('runtime')
    assert tag0.name == "runtime"
    assert len(tag0.arguments) == 0


def test_code_struct():
    analyzer: rg3py.CodeAnalyzer = rg3py.CodeAnalyzer.make()

    analyzer.set_code("""
        /// @runtime
        struct MyCoolStruct {
            /// @my_prop(123,321,true)
            int i32;
            
            /// @property(Awesome)
            bool b8;
        };
        """)
    analyzer.set_cpp_standard(rg3py.CppStandard.CXX_17)
    analyzer.analyze()

    assert len(analyzer.issues) == 0
    assert len(analyzer.types) == 1

    type0: rg3py.CppBaseType = analyzer.types[0]
    assert type0.kind == rg3py.CppTypeKind.TK_STRUCT_OR_CLASS

    struct0: rg3py.CppClass = type0
    assert struct0.is_struct
    assert struct0.name == "MyCoolStruct"
    assert struct0.pretty_name == "MyCoolStruct"
    assert str(struct0.namespace) == ""
    assert len(struct0.functions) == 0
    assert len(struct0.properties) == 2

    assert struct0.properties[0].name == "i32"
    assert struct0.properties[0].alias == "i32"
    assert struct0.properties[0].visibility == rg3py.CppClassEntryVisibillity.CEV_PUBLIC
    assert len(struct0.properties[0].tags.items) == 1
    assert 'my_prop' in struct0.properties[0].tags
    assert struct0.properties[0].tags.has_tag('my_prop')

    assert len(struct0.properties[0].tags.get_tag('my_prop').arguments) == 3
    assert str(struct0.properties[0].tags.get_tag('my_prop').arguments[0]) == '123'
    assert str(struct0.properties[0].tags.get_tag('my_prop').arguments[1]) == '321'
    assert str(struct0.properties[0].tags.get_tag('my_prop').arguments[2]) == 'True'
    assert struct0.properties[0].type_info.name == 'int'

    assert struct0.properties[1].name == "b8"
    assert struct0.properties[1].alias == "Awesome"
    assert struct0.properties[1].visibility == rg3py.CppClassEntryVisibillity.CEV_PUBLIC
    assert len(struct0.properties[1].tags.items) == 1
    assert 'property' in struct0.properties[1].tags
    assert len(struct0.properties[1].tags.get_tag('property').arguments) == 1
    assert str(struct0.properties[1].tags.get_tag('property').arguments[0]) == 'Awesome'
    assert struct0.properties[1].type_info.name == 'bool'


def test_analyzer_context_sample():
    analyzer_context: rg3py.AnalyzerContex = rg3py.AnalyzerContext.make()

    analyzer_context.set_headers(["samples/Header1.h"])
    analyzer_context.set_include_directories([rg3py.CppIncludeInfo("samples", rg3py.CppIncludeKind.IK_PROJECT)])

    analyzer_context.cpp_standard = rg3py.CppStandard.CXX_20
    analyzer_context.set_compiler_args(["-x", "c++-header"])
    analyzer_context.set_workers_count(2)

    analyzer_result: bool = analyzer_context.analyze()
    assert analyzer_result

    assert len(analyzer_context.issues) == 0
    assert len(analyzer_context.types) == 2

    # First type
    assert analyzer_context.types[0].name == "Type1Serializer"
    assert analyzer_context.types[0].kind == rg3py.CppTypeKind.TK_STRUCT_OR_CLASS
    assert analyzer_context.types[0].kind == rg3py.CppTypeKind.TK_STRUCT_OR_CLASS
    assert analyzer_context.types[0].tags.has_tag('runtime')

    assert analyzer_context.types[1].name == "Type1"
    assert analyzer_context.types[1].kind == rg3py.CppTypeKind.TK_STRUCT_OR_CLASS
    assert analyzer_context.types[1].tags.has_tag('runtime')
    assert analyzer_context.types[1].tags.has_tag('serializer')
    assert analyzer_context.types[1].tags.get_tag('serializer').arguments[0].as_type_ref().name == "samples::my_cool_sample::Type1Serializer"

    # Check that type resolvable
    resolved: Optional[rg3py.CppBaseType] = analyzer_context.get_type_by_reference(analyzer_context.types[1].tags.get_tag('serializer').arguments[0].as_type_ref())
    assert resolved is not None

    assert resolved.name == "Type1Serializer"
    assert resolved.hash == analyzer_context.types[0].hash
    assert resolved.pretty_name == analyzer_context.types[0].pretty_name


def test_check_analyzer_context_error_handling():
    analyzer_context: rg3py.AnalyzerContex = rg3py.AnalyzerContext.make()

    analyzer_context.set_headers(["samples/HeaderWithError.h"])
    analyzer_context.set_include_directories([rg3py.CppIncludeInfo("samples", rg3py.CppIncludeKind.IK_PROJECT)])

    analyzer_context.cpp_standard = rg3py.CppStandard.CXX_20
    analyzer_context.set_compiler_args(["-x", "c++-header"])

    analyzer_context.analyze()
    assert len(analyzer_context.issues) == 1
    assert analyzer_context.issues[0].kind == rg3py.CppCompilerIssueKind.IK_ERROR
    assert analyzer_context.issues[0].message == 'He he error here))'


# noinspection PyTypeChecker
def test_check_parent_types_resolver():
    analyzer_context: rg3py.AnalyzerContex = rg3py.AnalyzerContext.make()

    analyzer_context.set_headers(["samples/HeaderWithMultipleInheritance.h"])
    analyzer_context.set_include_directories([rg3py.CppIncludeInfo("samples", rg3py.CppIncludeKind.IK_PROJECT)])

    analyzer_context.cpp_standard = rg3py.CppStandard.CXX_20
    analyzer_context.set_compiler_args(["-x", "c++-header"])

    analyzer_context.analyze()

    assert len(analyzer_context.issues) == 0
    assert len(analyzer_context.types) == 2

    assert analyzer_context.types[0].pretty_name == "Parent"
    assert analyzer_context.types[0].kind == rg3py.CppTypeKind.TK_STRUCT_OR_CLASS

    assert analyzer_context.types[1].pretty_name == "Child"
    assert analyzer_context.types[1].kind == rg3py.CppTypeKind.TK_STRUCT_OR_CLASS

    c1: rg3py.CppClass = analyzer_context.types[0]
    c2: rg3py.CppClass = analyzer_context.types[1]

    assert len(c1.parent_types) == 0
    assert len(c2.parent_types) == 1

    c_parent: Optional[rg3py.CppBaseType] = analyzer_context.get_type_by_reference(c2.parent_types[0])
    assert c_parent is not None
    assert c_parent.hash == analyzer_context.types[0].hash
    assert c_parent.pretty_name == analyzer_context.types[0].pretty_name


def test_check_base_types():
    analyzer: rg3py.CodeAnalyzer = rg3py.CodeAnalyzer.make()

    analyzer.set_code("""
           #include <cstdint>
           #include <cstddef>
            
            /// @runtime
            struct MyCoolStruct {
                bool b8;
                uint8_t u8;
                int8_t i8;
                uint16_t u16;
                int16_t i16;
                uint32_t u32;
                int32_t i32;
                float f32;
                uint64_t u64;
                int64_t i64;
                double f64;
                size_t sz;
            };
            """)

    analyzer.set_compiler_args(["-x", "c++-header"])
    analyzer.set_cpp_standard(rg3py.CppStandard.CXX_20)
    analyzer.analyze()

    assert len(analyzer.issues) == 0

    c_class: rg3py.CppClass = analyzer.types[0]

    assert len(c_class.properties) == 12
    assert c_class.properties[0].type_info.name == "bool"
    assert c_class.properties[1].type_info.name == "uint8_t"
    assert c_class.properties[2].type_info.name == "int8_t"
    assert c_class.properties[3].type_info.name == "uint16_t"
    assert c_class.properties[4].type_info.name == "int16_t"
    assert c_class.properties[5].type_info.name == "uint32_t"
    assert c_class.properties[6].type_info.name == "int32_t"
    assert c_class.properties[7].type_info.name == "float"
    assert c_class.properties[8].type_info.name == "uint64_t"
    assert c_class.properties[9].type_info.name == "int64_t"
    assert c_class.properties[10].type_info.name == "double"
    assert c_class.properties[11].type_info.name == "size_t"
