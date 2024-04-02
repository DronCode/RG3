"""
This file contains all public available symbols & definitions for PyBind (rg3py.pyd)
Follow PyBind/source/PyBind.cpp for details
"""
from typing import List, Union, Optional


class CppStandard:
    CXX_11 = 11
    CXX_14 = 14
    CXX_17 = 17
    CXX_20 = 20
    CXX_23 = 23
    CXX_26 = 26
    CXX_DEFAULT = 11

class InheritanceVisibility:
    IV_PUBLIC = 0
    IV_PRIVATE = 1
    IV_PROTECTED = 2
    IV_VIRTUAL = 3

class CppEnumEntry:
    @property
    def name(self) -> str: ...

    @property
    def value(self) -> int: ...


class CppTypeKind:
    TK_NONE = 0
    TK_TRIVIAL = 1
    TK_ENUM = 2
    TK_STRUCT_OR_CLASS = 3

class CppEnum:
    @property
    def entries(self) -> List[CppEnumEntry]: ...

    @property
    def is_scoped(self) -> bool: ...

    @property
    def underlying_type(self) -> str: ...


class CppIncludeKind:
    IK_PROJECT = 0
    IK_SYSTEM = 1
    IK_SYSROOT = 2
    IK_THIRD_PARTY = 3
    IK_DEFAULT = IK_PROJECT


class CppIncludeInfo:
    def __init__(self, path: str, kind: CppIncludeKind): ...

    @property
    def path(self) -> str: ...

    @property
    def kind(self) -> CppIncludeKind: ...


class Location:
    def __init__(self, path: str, line: int, offset: int): ...

    @property
    def path(self) -> str: ...

    @property
    def line(self) -> int: ...

    @property
    def column(self) -> int: ...

    @property
    def angled(self) -> bool: ...


class CppNamespace:
    def __eq__(self, other) -> bool: ...
    def __ne__(self, other) -> bool: ...
    def __str__(self) -> str: ...
    def __repr__(self) -> str: ...


class TagArgumentType:
    AT_UNDEFINED = 0
    AT_BOOL = 1
    AT_FLOAT = 2
    AT_I64 = 3
    AT_STRING = 4
    AT_TYPEREF = 5


class TagArgument:
    def get_type(self) -> TagArgumentType: ...
    def as_bool(self) -> bool: ...
    def as_float(self) -> float: ...
    def as_i64(self) -> int: ...
    def as_string(self) -> str: ...
    def as_type_ref(self) -> Optional[CppTypeReference]: ...


class Tag:
    @property
    def name(self) -> str: ...

    @property
    def arguments(self) -> List[TagArgument]: ...

    def __eq__(self, other) -> bool: ...

    def __ne__(self, other) -> bool: ...

class Tags:
    @property
    def items(self) -> List[Tag]: ...

    def __contains__(self, tag: str) -> bool: ...

    def has_tag(self, tag: str) -> bool: ...

    def get_tag(self, tag: str) -> Tag: ...

class CppBaseType:
    @property
    def name(self) -> str: ...

    @property
    def hash(self) -> int: ...

    @property
    def kind(self) -> CppTypeKind: ...


    @property
    def namespace(self) -> CppNamespace: ...

    @property
    def location(self) -> Location: ...

    @property
    def pretty_name(self) -> str: ...

    @property
    def tags(self) -> Tags: ...

    def __str__(self) -> str: ...

    def __repr__(self) -> str: ...

    def __hash__(self) -> int: ...

    def __eq__(self, other) -> bool: ...

    def __ne__(self, other) -> bool: ...


class CppClassEntryVisibillity:
    CEV_PUBLIC = 0
    CEV_PRIVATE = 1
    CEV_PROTECTED = 2


class TypeStatement:
    @property
    def type_ref(self) -> CppTypeReference: ...

    @property
    def location(self) -> Optional[Location]: ...

    @property
    def is_const(self) -> bool: ...

    @property
    def is_const_ptr(self) -> bool: ...

    @property
    def is_ptr(self) -> bool: ...

    @property
    def is_ref(self) -> bool: ...

    @property
    def is_template(self) -> bool: ...

    @property
    def is_void(self) -> bool: ...

    def get_name(self) -> str: ...


class FunctionArgument:
    @property
    def type_info(self) -> TypeStatement: ...

    @property
    def name(self) -> str: ...

    @property
    def has_default_value(self) -> bool: ...


class CppClassFunction:
    @property
    def name(self) -> str: ...

    @property
    def owner(self) -> str: ...

    @property
    def visibility(self) -> CppClassEntryVisibillity: ...

    @property
    def tags(self) -> Tags: ...

    @property
    def is_static(self) -> bool: ...

    @property
    def is_const(self) -> bool: ...

    @property
    def return_type(self) -> TypeStatement: ...

    @property
    def arguments(self) -> List[FunctionArgument]: ...

    def __eq__(self, other) -> bool: ...

    def __ne__(self, other) -> bool: ...


class CppClassProperty:
    @property
    def name(self) -> str: ...

    @property
    def alias(self) -> str: ...

    @property
    def visibility(self) -> CppClassEntryVisibillity: ...

    @property
    def tags(self) -> Tags: ...

    @property
    def type_info(self) -> TypeStatement: ...

    def __eq__(self, other) -> bool: ...

    def __ne__(self, other) -> bool: ...


class ClassParent:
    @property
    def info(self) -> CppTypeReference: ...

    @property
    def inheritance(self) -> InheritanceVisibility: ...


class CppClass:
    @property
    def properties(self) -> List[CppClassProperty]: ...

    @property
    def functions(self) -> List[CppClassFunction]: ...

    @property
    def is_struct(self) -> bool: ...

    @property
    def is_trivial_constructible(self) -> bool: ...

    @property
    def parent_types(self) -> List[ClassParent]: ...


class CodeAnalyzer:
    @staticmethod
    def make() -> CodeAnalyzer: ...

    @property
    def types(self) -> List[CppBaseType]: ...

    @property
    def issues(self) -> List[CppCompilerIssue]: ...

    @property
    def definitions(self) -> List[str]: ...

    @property
    def ignore_runtime(self) -> bool: ...

    def set_ignore_non_runtime_types(self, value: bool): ...

    def is_non_runtime_types_ignored(self) -> bool: ...

    def set_code(self, code: str): ...

    def set_file(self, path: str): ...

    def set_cpp_standard(self, standard: CppStandard): ...

    def set_compiler_args(self, args: List[str]): ...

    def set_compiler_include_dirs(self, include_dirs: List[CppIncludeInfo]): ...

    def add_include_dir(self, inc: CppIncludeInfo): ...

    def add_project_include_dir(self, dir: str): ...

    def get_definitions(self) -> List[str]: ...

    def set_definitions(self, defs: List[str]): ...

    def analyze(self): ...


class AnalyzerContext:
    @staticmethod
    def make() -> AnalyzerContext: ...

    @property
    def workers_count(self) -> int: ...

    @property
    def types(self) -> List[CppBaseType]: ...

    @property
    def issues(self) -> List[CppCompilerIssue]: ...

    @property
    def headers(self) -> List[str]: ...

    @property
    def include_directories(self) -> List[CppIncludeInfo]: ...

    @property
    def cpp_standard(self) -> CppStandard: ...

    @property
    def compiler_args(self) -> List[str]: ...

    @property
    def ignore_runtime_tag(self) -> bool: ...

    def set_workers_count(self, count: int): ...

    def set_headers(self, headers: List[str]): ...

    def set_include_directories(self, include_dirs: List[CppIncludeInfo]): ...

    def set_compiler_args(self, args: List[str]): ...

    def set_compiler_defs(self, defs: List[str]): ...

    def get_type_by_reference(self, ref: CppTypeReference) -> Optional[CppBaseType]: ...

    def analyze(self) -> bool: ...


class CppCompilerIssueKind:
    IK_NONE = 0
    IK_WARNING = 1
    IK_INFO = 2
    IK_ERROR = 3


class CppCompilerIssue:
    @property
    def kind(self) -> CppCompilerIssueKind: ...

    @property
    def source_file(self) -> str: ...

    @property
    def message(self) -> str: ...


class CppTypeReference:
    @property
    def name(self) -> str: ...

class ClangRuntime:
    @staticmethod
    def get_version() -> str: ...

    @staticmethod
    def detect_system_include_sources() -> Union[str, List[str]]: ...

