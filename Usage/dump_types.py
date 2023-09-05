import time
from rg3py_ext import CodeAnalyzer, CppStandard, CppCompilerIssueKind, CppTypeKind, CppEnum, CppClassProperty, CppClassFunction, CppClass, Tag
from typing import List, Dict


def issue_kind_to_string(kind: CppCompilerIssueKind) -> str:
    if kind == CppCompilerIssueKind.IK_NONE:
        return "????"

    if kind == CppCompilerIssueKind.IK_WARNING:
        return "WARNING"

    if kind == CppCompilerIssueKind.IK_INFO:
        return "INFO"

    if kind == CppCompilerIssueKind.IK_ERROR:
        return "ERROR"

    return "UNKNOWN"

def do_analyze_source_code():
    analyzer: CodeAnalyzer = CodeAnalyzer.make()
    analyzer.set_code("""
    namespace ecs {
        /**
        * @runtime
        * @ecs
        * @serialize(Serializers)
        **/
        struct Component
        {
        };
        /**
         * @runtime
         * @ecs
         **/
        struct System
        {};

        /**
         * @runtime
         * @ecs
         **/
        struct Entity
        {
        };
    }

    namespace mygame
    {
        /**
        * @runtime
        * @serialize
        **/
        class PlayerEntity : public ecs::Entity
        {};
        
        /**
        * @runtime
        * @serialize
        **/
        class PlayerComponent : public ecs::Component
        {};
        
        /**
        * @runtime
        **/
        class CameraMovementSystem : public ecs::System
        {};
    }
    """)

    analyzer.set_cpp_standard(CppStandard.CXX_17)
    analyzer.analyze()

    if len(analyzer.issues) > 0:
        print(f"We have compiler issues ({len(analyzer.issues)}):")
        for issue in analyzer.issues:
            print(f"[{issue_kind_to_string(issue.kind)}]: {issue.message} from {issue.source_file}")

    print(f"Found {len(analyzer.types)} type(s)")
    for cpp_type in analyzer.types:
        tags: Dict[str, Tag] = cpp_type.tags

        if 'runtime' in tags:
            print(" > RUNTIME <")

        if 'serialize' in tags:
            print(" > SERIALIZE < ")

        # print(f"Tags count: {len(cpp_type.tags)}")
        if cpp_type.kind == CppTypeKind.TK_ENUM:
            e: CppEnum = cpp_type

            print(f"Enum: '{e.name}' (namespace: '{e.namespace}') declared in '{e.location.path}':{e.location.line}")
            for entry in e.entries:
                print(f"\t * {entry.name} = {entry.value}")
        elif cpp_type.kind == CppTypeKind.TK_STRUCT_OR_CLASS:
            c: CppClass = cpp_type

            print(f"Class/Struct: '{c.name}' (namespace: '{c.namespace}') declared int '{c.location.path}':{c.location.line}")
            print(f"IsStruct: {c.is_struct}")
            print(f"TrivialConstructible: {c.is_trivial_constructible}")

            for prop in c.properties:
                print(f"\t({prop.visibility}) {prop.name} (aka {prop.alias})")

            for func in c.functions:
                print(f"\t({func.visibility}) {func.owner}::{func.name} (const={func.is_const}|static={func.is_static})")

            if len(c.parent_types) > 0:
                print(f"Parents: {','.join(c.parent_types)}")

            # print(f"Parent types: {len(c.parent_types)}")
        else:
            print(f"Type: {cpp_type.name} declared in {cpp_type.location.path}:{cpp_type.location.line}")


st = time.time()
# --------------------------------------------------------
do_analyze_source_code()
# --------------------------------------------------------
et = time.time()

# get the execution time
elapsed_time = et - st
print(f'Execution time: {elapsed_time} seconds')
