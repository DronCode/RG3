"""
CodeWriter - Convenient helper for writing generated code files.

Provides a simple API with writeln() and indent support for generating
C++ header and source files.
"""

import os
from contextlib import contextmanager
from typing import Optional


class CodeWriter:
    """
    Helper for writing formatted code files.
    
    Usage:
        writer = CodeWriter(output_dir)
        with writer.open("MyClass.hpp") as w:
            w.writeln("#pragma once")
            with w.indent():
                w.writeln('#include "MyClass_fwd.hpp"')
            w.writeln()
            w.writeln("namespace my {")
            with w.indent():
                w.writeln("class MyClass {};")
            w.writeln("}")
        writer.close()
    
    Or as a context manager:
        with CodeWriter(output_dir) as w:
            w.writeln("content")
    """
    
    def __init__(self, output_dir: str = ""):
        self.output_dir = output_dir
        self._filepath: Optional[str] = None
        self._file = None
        self._indent_level = 0
        self._indent_size = 4
        self._lines: list = []
    
    def open(self, filename: str) -> 'CodeWriter':
        """
        Open a file for writing.
        
        Args:
            filename: Output filename relative to output_dir
            
        Returns:
            self (for chaining)
        """
        self._filepath = os.path.join(self.output_dir, filename)
        os.makedirs(os.path.dirname(self._filepath), exist_ok=True)
        self._file = open(self._filepath, "w", encoding="utf-8")
        self._lines = []
        return self
    
    def close(self):
        """Close the file."""
        if self._file:
            self._file.close()
            self._file = None
            self._filepath = None
    
    @contextmanager
    def indent(self, level: int = 1):
        """Context manager for indentation blocks."""
        old_level = self._indent_level
        self._indent_level += level
        try:
            yield self
        finally:
            self._indent_level = old_level
    
    @contextmanager
    def block(self, open_line: str, close_line: str = "}", indent: int = 1):
        """Context manager for code blocks with open/close braces."""
        self.writeln(open_line)
        with self.indent(indent):
            yield self
        self.writeln(close_line)
    
    def writeln(self, line: str = "", *args, **kwargs):
        """
        Write a line with automatic indentation.
        
        Args:
            line: Line content (supports format strings)
            *args, **kwargs: Arguments for str.format()
        """
        if args or kwargs:
            line = line.format(*args, **kwargs)
        indent_str = " " * (self._indent_level * self._indent_size)
        if line:
            self._lines.append(f"{indent_str}{line}")
        else:
            self._lines.append("")
    
    def write(self, text: str):
        """Write text without newline."""
        if self._file:
            self._file.write(text)
    
    def __enter__(self):
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()
        return False
    
    @property
    def content(self) -> str:
        """Get the accumulated content as a string."""
        return "\n".join(self._lines)
    
    def save(self):
        """Save accumulated lines to the file."""
        if self._file:
            self._file.write("\n".join(self._lines) + "\n")
