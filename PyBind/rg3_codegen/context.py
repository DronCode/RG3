"""
RG3CodegenContext - Main context object for RG3 code generation.

This class wraps rg3py's CodeAnalyzer/AnalyzerContext results and provides
a convenient interface for codegen scripts to:
- Access analyzed C++ types, functions, properties
- Declare output files (for CMake to track)
- Write generated output files
- Track which files were generated
"""

import os
from typing import List, Optional, Any


class RG3CodegenContext:
    """
    Context object injected into user's codegen script.
    
    Usage:
        # Access analyzed types
        for type_info in rg.analyzed_types:
            ...
        
        # Declare generated files (works in both discover and generate mode)
        rg.declare_outputs("output1.hpp", "output2.cpp")
        
        # Write files (no-op in discover mode, writes in generate mode)
        rg.write_file("output.hpp", "#pragma once\n// content")
    
    Attributes:
        analyzed_types: List of analyzed C++ types from rg3py
        issues: Any compiler issues found during analysis
        headers: Header files that were analyzed
        sources: Source files that were analyzed
        include_dirs: Include directories used
        compile_definitions: Compile definitions used
        cpp_standard: C++ standard number (11, 14, 17, 20, etc.)
        output_dir: Directory where generated files should be written
    """
    
    def __init__(
        self,
        analyzed_types: list,
        issues: list,
        output_dir: str,
        headers: Optional[List[str]] = None,
        sources: Optional[List[str]] = None,
        include_dirs: Optional[List[str]] = None,
        compile_definitions: Optional[List[str]] = None,
        cpp_standard: int = 11,
        discover_only: bool = False,
    ):
        self.analyzed_types = analyzed_types
        self.issues = issues
        self.headers = headers or []
        self.sources = sources or []
        self.include_dirs = include_dirs or []
        self.compile_definitions = compile_definitions or []
        self.cpp_standard = cpp_standard
        self.output_dir = output_dir
        self.discover_only = discover_only
        self._generated_files: List[str] = []
        self._writer: Optional['CodeWriter'] = None
    
    @property
    def types(self) -> list:
        """Alias for analyzed_types (more convenient name)."""
        return self.analyzed_types
    
    @property
    def type_count(self) -> int:
        """Number of analyzed types."""
        return len(self.analyzed_types)
    
    @property
    def has_errors(self) -> bool:
        """True if any compiler errors were found."""
        try:
            import rg3py
            return any(
                issue.kind == rg3py.CppCompilerIssueKind.IK_ERROR
                for issue in self.issues
            )
        except ImportError:
            return len(self.issues) > 0
    
    def declare_outputs(self, *file_paths: str):
        """
        Declare output file paths for CMake to track.
        
        This must be called at the end of your codegen script.
        CMake uses this list to know what files to expect.
        
        Works in both discover mode (configure time) and generate mode (build time).
        
        Args:
            *file_paths: Variable number of file paths (relative to output_dir)
        
        Example:
            rg.declare_outputs("MyClass.hpp", "MyClass.cpp")
        """
        self._generated_files = list(file_paths)
    
    # Alias for backward compatibility
    def write_generated_files(self, *file_paths: str):
        """Alias for declare_outputs()."""
        self.declare_outputs(*file_paths)
    
    def get_generated_files(self) -> List[str]:
        """Get the list of declared output files."""
        return self._generated_files
    
    def get_type_by_name(self, name: str) -> Optional[Any]:
        """
        Find an analyzed type by its name (pretty name or simple name).
        
        Args:
            name: Type name to search for
            
        Returns:
            The type if found, None otherwise
        """
        for t in self.analyzed_types:
            if t.name == name or t.pretty_name == name:
                return t
        return None
    
    def get_types_by_namespace(self, namespace: str) -> List[Any]:
        """
        Get all types in a specific namespace.
        
        Args:
            namespace: Namespace string (e.g., "my::lib")
            
        Returns:
            List of types in that namespace
        """
        result = []
        ns_str = str(namespace)
        for t in self.analyzed_types:
            try:
                if str(t.namespace) == ns_str:
                    result.append(t)
            except Exception:
                pass
        return result
    
    @property
    def writer(self) -> 'CodeWriter':
        """
        Get a CodeWriter instance for convenient file writing.
        
        Usage:
            with rg.writer("output.hpp") as w:
                w.writeln("#pragma once")
        
        Returns:
            A CodeWriter bound to the output directory
        """
        from rg3_codegen.writer import CodeWriter
        return CodeWriter(self.output_dir)
    
    def write_file(self, filename: str, content: str):
        """
        Write content to a file in the output directory.
        
        In discover mode, this is a NO-OP (files are not actually written).
        In generate mode, the file is written to disk.
        
        Args:
            filename: Output filename (relative to output_dir)
            content: File content
            
        Example:
            rg.write_file("output.hpp", "#pragma once\n")
            rg.declare_outputs("output.hpp")
        """
        if self.discover_only:
            # In discover mode, just acknowledge but don't write
            return
        
        filepath = os.path.join(self.output_dir, filename)
        os.makedirs(os.path.dirname(filepath), exist_ok=True)
        with open(filepath, "w", encoding="utf-8") as f:
            f.write(content)
    
    def __repr__(self) -> str:
        mode = "discover" if self.discover_only else "generate"
        return (
            f"RG3CodegenContext(types={self.type_count}, "
            f"headers={len(self.headers)}, "
            f"mode={mode}, "
            f"output_dir={self.output_dir})"
        )
