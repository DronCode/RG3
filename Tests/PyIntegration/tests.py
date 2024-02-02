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

    assert len(type0.tags) == 1
    assert 'runtime' in type0.tags

    tag0: rg3py.Tag = type0.tags['runtime']
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
    assert len(struct0.properties[0].tags) == 1
    assert 'my_prop' in struct0.properties[0].tags
    assert len(struct0.properties[0].tags['my_prop'].arguments) == 3
    assert str(struct0.properties[0].tags['my_prop'].arguments[0]) == '123'
    assert str(struct0.properties[0].tags['my_prop'].arguments[1]) == '321'
    assert str(struct0.properties[0].tags['my_prop'].arguments[2]) == 'True'
    assert struct0.properties[0].type_name == 'int'

    assert struct0.properties[1].name == "b8"
    assert struct0.properties[1].alias == "Awesome"
    assert struct0.properties[1].visibility == rg3py.CppClassEntryVisibillity.CEV_PUBLIC
    assert len(struct0.properties[1].tags) == 1
    assert 'property' in struct0.properties[1].tags
    assert len(struct0.properties[1].tags['property'].arguments) == 1
    assert str(struct0.properties[1].tags['property'].arguments[0]) == 'Awesome'
    assert struct0.properties[1].type_name == 'bool'
