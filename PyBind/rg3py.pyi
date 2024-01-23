"""
This file contains all public available symbols & definitions for PyBind (rg3py.pyd)
Follow PyBind/source/PyBind.cpp for details
"""
from typing import List, Union


class CppStandard:
    CXX_11 = 11
    CXX_14 = 14
    CXX_17 = 17
    CXX_20 = 20
    CXX_23 = 23
    CXX_26 = 26
    CXX_DEFAULT = 11


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
    TK_TEMPLATE_SPECIALIZATION = 4


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


class Tag:
    @property
    def name(self) -> str: ...

    @property
    def arguments(self) -> List[TagArgument]: ...

    def __eq__(self, other) -> bool: ...

    def __ne__(self, other) -> bool: ...

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
    def tags(self) -> List[Tag]: ...

    def __str__(self) -> str: ...

    def __repr__(self) -> str: ...

    def __hash__(self) -> int: ...

    def __eq__(self, other) -> bool: ...

    def __ne__(self, other) -> bool: ...


class CppClassEntryVisibillity:
    CEV_PRIVATE = 0
    CEV_PROTECTED = 1
    CEV_PUBLIC = 2


class CppClassFunction:
    @property
    def name(self) -> str: ...

    @property
    def owner(self) -> str: ...

    @property
    def visibility(self) -> CppClassEntryVisibillity: ...

    @property
    def tags(self) -> List[Tag]: ...

    @property
    def is_static(self) -> bool: ...

    @property
    def is_const(self) -> bool: ...

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
    def tags(self) -> List[Tag]: ...

    def __eq__(self, other) -> bool: ...

    def __ne__(self, other) -> bool: ...


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
    def parent_types(self) -> List[str]: ...


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
    def __init__(self, path: str): ...


class ClangRuntime:
    @staticmethod
    def get_version() -> str: ...

    @staticmethod
    def detect_system_include_sources() -> Union[str, List[str]]: ...

