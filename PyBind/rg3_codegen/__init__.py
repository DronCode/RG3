"""
rg3_codegen - Python framework for RG3-based code generation.

This package provides utilities for writing CMake-integrated code generation
scripts that use RG3 to analyze C++ code and generate output files.

Usage in your codegen script:
    from rg3_codegen import RG3CodegenContext
    
    def generate_types(rg: RG3CodegenContext):
        # Access analyzed types
        for type_info in rg.analyzed_types:
            if type_info.kind == rg3py.CppTypeKind.TK_STRUCT_OR_CLASS:
                # Generate code for this type
                pass
        
        # Declare generated files
        rg.write_generated_files("MyType.hpp", "MyType.cpp")

# Or use the context directly (injected by entry point)
"""

__version__ = "0.1.0"

from rg3_codegen.context import RG3CodegenContext
from rg3_codegen.writer import CodeWriter
from rg3_codegen.types import wrap_type, get_type_members

__all__ = [
    "RG3CodegenContext",
    "CodeWriter",
    "wrap_type",
    "get_type_members",
]
