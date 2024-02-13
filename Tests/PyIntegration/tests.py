from typing import Optional, List
import pytest
import rg3py
import os


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
    assert struct0.properties[0].type_info.type_ref.name == 'int'

    assert struct0.properties[1].name == "b8"
    assert struct0.properties[1].alias == "Awesome"
    assert struct0.properties[1].visibility == rg3py.CppClassEntryVisibillity.CEV_PUBLIC
    assert len(struct0.properties[1].tags.items) == 1
    assert 'property' in struct0.properties[1].tags
    assert len(struct0.properties[1].tags.get_tag('property').arguments) == 1
    assert str(struct0.properties[1].tags.get_tag('property').arguments[0]) == 'Awesome'
    assert struct0.properties[1].type_info.get_name() == 'bool'


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
    assert analyzer_context.types[1].tags.get_tag('serializer').arguments[
               0].as_type_ref().name == "samples::my_cool_sample::Type1Serializer"

    # Check that type resolvable
    resolved: Optional[rg3py.CppBaseType] = analyzer_context.get_type_by_reference(
        analyzer_context.types[1].tags.get_tag('serializer').arguments[0].as_type_ref())
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

    c_parent: Optional[rg3py.CppBaseType] = analyzer_context.get_type_by_reference(c2.parent_types[0].info)
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
    assert c_class.properties[0].type_info.get_name() == "bool"
    assert c_class.properties[1].type_info.get_name() == "uint8_t"
    assert c_class.properties[2].type_info.get_name() == "int8_t"
    assert c_class.properties[3].type_info.get_name() == "uint16_t"
    assert c_class.properties[4].type_info.get_name() == "int16_t"
    assert c_class.properties[5].type_info.get_name() == "uint32_t"
    assert c_class.properties[6].type_info.get_name() == "int32_t"
    assert c_class.properties[7].type_info.get_name() == "float"
    assert c_class.properties[8].type_info.get_name() == "uint64_t"
    assert c_class.properties[9].type_info.get_name() == "int64_t"
    assert c_class.properties[10].type_info.get_name() == "double"
    assert c_class.properties[11].type_info.get_name() == "size_t"


def test_check_field_decl_type_info():
    """
    Very basic test. Check that we can handle types (custom and builtin)
    """
    analyzer: rg3py.CodeAnalyzer = rg3py.CodeAnalyzer.make()

    analyzer.set_code("""
                struct SomeType {};
    
                /// @runtime
                struct MyCoolStruct {
                    /// @property(MyProp)
                    const int* pFieldPtr { nullptr };
                    
                    /// @property
                    SomeType s_Entry;
                    
                    /// @property
                    int& g_Health;
                };
                """)

    analyzer.set_compiler_args(["-x", "c++-header"])
    analyzer.set_cpp_standard(rg3py.CppStandard.CXX_20)
    analyzer.analyze()

    assert len(analyzer.issues) == 0
    assert len(analyzer.types) == 1

    assert analyzer.types[0].name == "MyCoolStruct"
    assert analyzer.types[0].kind == rg3py.CppTypeKind.TK_STRUCT_OR_CLASS

    c_class: rg3py.CppClass = analyzer.types[0]
    assert len(c_class.properties) == 3
    assert len(c_class.functions) == 0
    assert c_class.properties[0].name == "pFieldPtr"
    assert c_class.properties[0].alias == "MyProp"
    assert c_class.properties[0].type_info.is_const is False
    assert c_class.properties[0].type_info.is_const_ptr is True
    assert c_class.properties[0].type_info.is_ptr is True
    assert c_class.properties[0].type_info.is_ref is False
    assert c_class.properties[0].type_info.is_template is False
    assert c_class.properties[0].type_info.is_void is False
    assert c_class.properties[0].type_info.location is None
    assert c_class.properties[0].type_info.get_name() == "int"

    assert c_class.properties[1].name == "s_Entry"
    assert c_class.properties[1].alias == "s_Entry"
    assert c_class.properties[1].type_info.location is not None
    assert c_class.properties[1].type_info.location.path == "id0.hpp"
    # Line & column not checking, it's UB in some cases
    assert c_class.properties[1].type_info.is_const is False
    assert c_class.properties[1].type_info.is_const_ptr is False
    assert c_class.properties[1].type_info.is_ptr is False
    assert c_class.properties[1].type_info.is_ref is False
    assert c_class.properties[1].type_info.is_void is False
    assert c_class.properties[1].type_info.is_template is False
    assert c_class.properties[1].type_info.get_name() == "SomeType"

    assert c_class.properties[2].name == "g_Health"
    assert c_class.properties[2].alias == "g_Health"
    assert c_class.properties[2].type_info.is_const is False
    assert c_class.properties[2].type_info.is_const_ptr is False
    assert c_class.properties[2].type_info.is_ptr is False
    assert c_class.properties[2].type_info.is_ref is True
    assert c_class.properties[2].type_info.is_template is False
    assert c_class.properties[2].type_info.is_void is False
    assert c_class.properties[2].type_info.location is None
    assert c_class.properties[2].type_info.type_ref.name == "int"


def test_check_member_function_arguments_and_return_type():
    """
    Very basic test. Check that we can handle types (custom and builtin)
    """
    analyzer: rg3py.CodeAnalyzer = rg3py.CodeAnalyzer.make()

    analyzer.set_code("""
                    /// @runtime
                    struct DamageInfo {
                        /// @property(vDir)
                        float direction[3] { .0f };
                        
                        /// @property(fPower)
                        float power { .0f };
                    };

                    /// @runtime
                    struct PlanetComponent {
                        bool RegisterDamage(const DamageInfo& damageInfo);
                        void MarkAsKilled(bool b_byDamage = false);
                        
                        /// @taggable(true)
                        static DamageInfo* const GetLastKnownDamage();
                        
                        bool CanHandleDamage(const DamageInfo* const pDamage = nullptr) const;
                    };
                    """)

    analyzer.set_compiler_args(["-x", "c++-header"])
    analyzer.set_cpp_standard(rg3py.CppStandard.CXX_20)
    analyzer.analyze()

    assert len(analyzer.issues) == 0
    assert len(analyzer.types) == 2

    assert analyzer.types[0].name == "DamageInfo"
    assert analyzer.types[0].kind == rg3py.CppTypeKind.TK_STRUCT_OR_CLASS

    assert analyzer.types[1].name == "PlanetComponent"
    assert analyzer.types[1].kind == rg3py.CppTypeKind.TK_STRUCT_OR_CLASS

    c_class0: rg3py.CppClass = analyzer.types[0]
    c_class1: rg3py.CppClass = analyzer.types[1]

    # NOTE: Here we need to support arrays in v0.0.3. For now I'm interpreting this type as float[3]
    assert len(c_class0.properties) == 2
    assert len(c_class0.functions) == 0
    assert c_class0.properties[0].name == 'direction'
    assert c_class0.properties[0].alias == 'vDir'
    assert c_class0.properties[0].type_info.get_name() == 'float[3]'

    assert c_class0.properties[1].name == 'power'
    assert c_class0.properties[1].alias == 'fPower'
    assert c_class0.properties[1].type_info.get_name() == 'float'

    assert len(c_class1.properties) == 0
    assert len(c_class1.functions) == 4

    assert c_class1.functions[0].name == 'RegisterDamage'
    assert c_class1.functions[0].owner == 'PlanetComponent'
    assert c_class1.functions[0].is_const is False
    assert c_class1.functions[0].is_static is False
    assert c_class1.functions[0].return_type.get_name() == 'bool'
    assert len(c_class1.functions[0].arguments) == 1
    assert c_class1.functions[0].arguments[0].has_default_value is False
    assert c_class1.functions[0].arguments[0].name == 'damageInfo'
    assert c_class1.functions[0].arguments[0].type_info.get_name() == 'DamageInfo'
    assert c_class1.functions[0].arguments[0].type_info.is_const is False
    assert c_class1.functions[0].arguments[0].type_info.is_const_ptr is True
    assert c_class1.functions[0].arguments[0].type_info.is_ptr is False
    assert c_class1.functions[0].arguments[0].type_info.is_template is False
    assert c_class1.functions[0].arguments[0].type_info.is_void is False
    assert c_class1.functions[0].arguments[0].type_info.location is not None
    assert c_class1.functions[0].arguments[0].type_info.location.path == 'id0.hpp'

    assert c_class1.functions[1].name == 'MarkAsKilled'
    assert c_class1.functions[1].owner == 'PlanetComponent'
    assert c_class1.functions[1].is_const is False
    assert c_class1.functions[1].is_static is False
    assert c_class1.functions[1].return_type.get_name() == 'void'
    assert c_class1.functions[1].return_type.is_void is True
    assert len(c_class1.functions[1].arguments) == 1
    assert c_class1.functions[1].arguments[0].has_default_value is True
    assert c_class1.functions[1].arguments[0].name == 'b_byDamage'
    assert c_class1.functions[1].arguments[0].type_info.get_name() == 'bool'
    assert c_class1.functions[1].arguments[0].type_info.is_const is False
    assert c_class1.functions[1].arguments[0].type_info.is_const_ptr is False
    assert c_class1.functions[1].arguments[0].type_info.is_ptr is False
    assert c_class1.functions[1].arguments[0].type_info.is_template is False
    assert c_class1.functions[1].arguments[0].type_info.is_void is False
    assert c_class1.functions[1].arguments[0].type_info.location is None

    assert c_class1.functions[2].name == 'GetLastKnownDamage'
    assert c_class1.functions[2].owner == 'PlanetComponent'
    assert c_class1.functions[2].tags.has_tag('taggable')
    assert len(c_class1.functions[2].tags.get_tag('taggable').arguments) == 1
    assert c_class1.functions[2].tags.get_tag('taggable').arguments[0].get_type() == rg3py.TagArgumentType.AT_BOOL
    assert c_class1.functions[2].tags.get_tag('taggable').arguments[0].as_bool(False) is True
    assert len(c_class1.functions[2].arguments) == 0
    assert c_class1.functions[2].return_type.get_name() == "DamageInfo"
    assert c_class1.functions[2].return_type.is_const is True
    assert c_class1.functions[2].return_type.is_const_ptr is False
    assert c_class1.functions[2].return_type.is_ptr is True
    assert c_class1.functions[2].return_type.is_ref is False
    assert c_class1.functions[2].return_type.is_void is False
    assert c_class1.functions[2].return_type.location is not None
    assert c_class1.functions[2].return_type.location.path == "id0.hpp"
    assert c_class1.functions[2].return_type.type_ref.name == "DamageInfo"
    assert c_class1.functions[2].is_static is True
    assert c_class1.functions[2].is_const is False

    assert c_class1.functions[3].name == 'CanHandleDamage'
    assert c_class1.functions[3].return_type.get_name() == "bool"
    assert c_class1.functions[3].return_type.location is None
    assert c_class1.functions[3].is_const is True
    assert c_class1.functions[3].is_static is False
    assert len(c_class1.functions[3].arguments) == 1
    assert c_class1.functions[3].arguments[0].name == "pDamage"
    assert c_class1.functions[3].arguments[0].has_default_value is True
    assert c_class1.functions[3].arguments[0].type_info.get_name() == "DamageInfo"
    assert c_class1.functions[3].arguments[0].type_info.is_const is True
    assert c_class1.functions[3].arguments[0].type_info.is_const_ptr is True
    assert c_class1.functions[3].arguments[0].type_info.is_void is False
    assert c_class1.functions[3].arguments[0].type_info.is_template is False
    assert c_class1.functions[3].arguments[0].type_info.is_ptr is True
    assert c_class1.functions[3].arguments[0].type_info.is_ref is False


def test_check_clang_runtime():
    assert len(rg3py.ClangRuntime.get_version()) > 0
    assert "Clang" in rg3py.ClangRuntime.get_version()

    found_inc_paths: List[str] = rg3py.ClangRuntime.detect_system_include_sources()
    assert len(found_inc_paths) > 1  # NOTE: In windows it usually 8, on macOS and Linux could be less/more.


def test_check_type_inside_type():
    analyzer: rg3py.CodeAnalyzer = rg3py.CodeAnalyzer.make()

    analyzer.set_code("""
namespace generic { namespace extended {
    /// @runtime
    struct MyStruct
    {
        /// @runtime
        enum class MyCoolEnum : int 
        {};
    };
}}
                       """)

    analyzer.set_compiler_args(["-x", "c++-header"])
    analyzer.set_cpp_standard(rg3py.CppStandard.CXX_17)
    analyzer.analyze()

    assert len(analyzer.issues) == 0
    assert len(analyzer.types) == 2

    assert analyzer.types[0].name == "MyStruct"
    assert analyzer.types[0].pretty_name == "generic::extended::MyStruct"

    assert analyzer.types[1].name == "MyCoolEnum"
    assert analyzer.types[1].pretty_name == "generic::extended::MyStruct::MyCoolEnum"


def test_check_type_alias_and_template_spec_basic():
    analyzer: rg3py.CodeAnalyzer = rg3py.CodeAnalyzer.make()

    analyzer.set_file("samples/HeaderWithUsingDecls.h")
    analyzer.set_compiler_args(["-x", "c++-header"])
    analyzer.set_cpp_standard(rg3py.CppStandard.CXX_17)
    analyzer.analyze()

    assert len(analyzer.issues) == 0
    assert len(analyzer.types) == 5

    assert analyzer.types[0].name == "MyComponent"
    assert analyzer.types[0].pretty_name == "engine::MyComponent"
    assert analyzer.types[0].kind == rg3py.CppTypeKind.TK_STRUCT_OR_CLASS

    assert analyzer.types[1].name == "Ptr"
    assert analyzer.types[1].pretty_name == "engine::MyComponent::Ptr"
    assert analyzer.types[1].kind == rg3py.CppTypeKind.TK_ALIAS

    assert os.path.basename(analyzer.types[1].location.path) == "HeaderWithUsingDecls.h"

    as_alias1: rg3py.CppAlias = analyzer.types[1]
    assert as_alias1.target_location is not None
    assert os.path.basename(as_alias1.target_location.path) == "HeaderWithUsingDecls.h"
    assert as_alias1.target_description.get_name() == "not_std::shared_ptr<engine::MyComponent>"
    assert as_alias1.target_description.is_template is True
    assert as_alias1.target_description.is_ref is False
    assert as_alias1.target_description.is_const is False
    assert as_alias1.target_description.is_ptr is False
    assert as_alias1.target_description.is_void is False
    assert as_alias1.target_description.is_const_ptr is False

    # -----------------------------------------------------------------
    assert analyzer.types[2].name == "i32"
    assert analyzer.types[2].pretty_name == "xzibit_stl::i32"

    as_alias2: rg3py.CppAlias = analyzer.types[2]
    assert as_alias2.target_location is None
    assert as_alias2.target.name == "int"
    assert as_alias2.target_description.get_name() == "int"
    assert as_alias2.target_description.is_ptr is False
    assert as_alias2.target_description.is_const is False
    assert as_alias2.target_description.is_const_ptr is False
    assert as_alias2.target_description.is_template is False
    assert as_alias2.target_description.is_ref is False
    assert as_alias2.target_description.is_void is False

    # -----------------------------------------------------------------
    assert analyzer.types[3].name == "u32"
    assert analyzer.types[3].pretty_name == "xzibit_stl::u32"

    as_alias3: rg3py.CppAlias = analyzer.types[3]
    assert as_alias3.target_location is None

    assert as_alias3.target.name == "unsigned int"
    assert as_alias3.target_description.get_name() == "unsigned int"
    assert as_alias3.target_description.is_ptr is False
    assert as_alias3.target_description.is_const is False
    assert as_alias3.target_description.is_const_ptr is False
    assert as_alias3.target_description.is_template is False
    assert as_alias3.target_description.is_ref is False
    assert as_alias3.target_description.is_void is False
    # -----------------------------------------------------------------
    assert analyzer.types[4].name == "bytes"
    assert analyzer.types[4].pretty_name == "xzibit_stl::bytes"

    as_alias4: rg3py.CppAlias = analyzer.types[4]
    assert as_alias4.target_location is None

    assert as_alias4.target.name == "unsigned char *"
    assert as_alias4.target_description.get_name() == "unsigned char *"
    assert as_alias4.target_description.is_ptr is True
    assert as_alias4.target_description.is_const is False
    assert as_alias4.target_description.is_const_ptr is False
    assert as_alias4.target_description.is_template is False
    assert as_alias4.target_description.is_ref is False
    assert as_alias4.target_description.is_void is False


def test_check_global_type_aliases():
    analyzer: rg3py.CodeAnalyzer = rg3py.CodeAnalyzer.make()

    analyzer.set_code("""
    /// @runtime
    struct MyStruct
    {
        /// @runtime
        typedef const MyStruct* CPtr;
        
        /// @runtime
        typedef unsigned int ID;
    };
""")

    analyzer.set_compiler_args(["-x", "c++-header"])
    analyzer.set_cpp_standard(rg3py.CppStandard.CXX_17)
    analyzer.analyze()

    assert len(analyzer.issues) == 0
    assert len(analyzer.types) == 3

    assert analyzer.types[0].name == "MyStruct"
    assert analyzer.types[0].pretty_name == "MyStruct"
    assert analyzer.types[0].kind == rg3py.CppTypeKind.TK_STRUCT_OR_CLASS

    assert analyzer.types[1].name == "CPtr"
    assert analyzer.types[1].pretty_name == "MyStruct::CPtr"
    assert analyzer.types[1].kind == rg3py.CppTypeKind.TK_ALIAS

    as_alias1: rg3py.CppAlias = analyzer.types[1]
    assert as_alias1.target.name == "MyStruct"
    assert as_alias1.target_location is not None
    assert as_alias1.target_description.is_ref is False
    assert as_alias1.target_description.is_ptr is True
    assert as_alias1.target_description.is_const_ptr is True
    assert as_alias1.target_description.is_template is False
    assert as_alias1.target_description.is_const is False

    as_alias2: rg3py.CppAlias = analyzer.types[2]
    assert as_alias2.target.name == "unsigned int"
    assert as_alias2.target_location is None
    assert as_alias2.target_description.is_ref is False
    assert as_alias2.target_description.is_ptr is False
    assert as_alias2.target_description.is_const_ptr is False
    assert as_alias2.target_description.is_template is False
    assert as_alias2.target_description.is_const is False
