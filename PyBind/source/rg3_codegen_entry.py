"""
rg3_codegen_entry.py - CMake entry point for RG3 code generation.

This script bridges CMake and Python-based code generation using rg3py.
It supports two modes:

  1. DISCOVER mode (configure time):
     python rg3_codegen_entry.py <input.json> <output.json> --discover
     
     Runs the codegen script to discover what files it WILL generate.
     Does NOT actually write any output files.
     Writes a JSON manifest with the list of discovered output files.

  2. GENERATE mode (build time, default):
     python rg3_codegen_entry.py <input.json> <output.json>
     
     Runs the codegen script to actually generate output files.
     Writes a JSON manifest with the list of generated files.

Input manifest (JSON):
{
    "sources": [...],               # source files to analyze
    "headers": [...],               # header files to analyze
    "include_dirs": [...],          # include directories
    "compile_definitions": [...],   # -D definitions
    "cpp_standard": 20,             # C++ standard number
    "output_dir": "...",            # where to write generated files
    "codegen_script": "..."         # path to user's codegen script
}

Output manifest (JSON):
{
    "outputs": [...],               # list of generated output files (relative paths)
    "status": "success|error",
    "message": "...",
    "warning": "..."                # optional warning
}
"""

import sys
import os
import json
import importlib.util
from pathlib import Path


def load_input_manifest(path: str) -> dict:
    """Load and validate the input JSON manifest."""
    with open(path, "r", encoding="utf-8") as f:
        data = json.load(f)
    
    required_keys = ["headers", "output_dir", "codegen_script"]
    for key in required_keys:
        if key not in data:
            raise ValueError(f"Missing required key in manifest: {key}")
    
    return data


def write_output_manifest(path: str, data: dict):
    """Write the output JSON manifest."""
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)


def run_codegen(manifest: dict, discover_only: bool = False) -> dict:
    """
    Run the user's codegen script with RG3.
    
    Args:
        manifest: Input manifest dict
        discover_only: If True, only discover outputs without generating files
        
    Returns:
        Output manifest dict with outputs, status, message
    """
    output_dir = manifest["output_dir"]
    codegen_script = manifest["codegen_script"]
    headers = manifest.get("headers", [])
    sources = manifest.get("sources", [])
    include_dirs = manifest.get("include_dirs", [])
    compile_definitions = manifest.get("compile_definitions", [])
    cpp_standard = manifest.get("cpp_standard", 11)
    
    # Ensure output directory exists (even in discover mode)
    os.makedirs(output_dir, exist_ok=True)
    
    # Add RG3 framework path to sys.path
    # The framework is in RG3/PyBind/rg3_codegen/
    entry_point_path = os.path.abspath(__file__)
    rg3_framework_dir = os.path.join(
        os.path.dirname(entry_point_path),
        "..", "rg3_codegen"
    )
    rg3_framework_dir = os.path.abspath(rg3_framework_dir)
    
    if rg3_framework_dir not in sys.path:
        sys.path.insert(0, rg3_framework_dir)
    
    # Build the analysis context
    analyzed_types = []
    issues = []
    
    try:
        import rg3py
        
        # Map cpp_standard to rg3py CppStandard enum
        std_map = {
            11: rg3py.CppStandard.CXX_11,
            14: rg3py.CppStandard.CXX_14,
            17: rg3py.CppStandard.CXX_17,
            20: rg3py.CppStandard.CXX_20,
            23: rg3py.CppStandard.CXX_23,
            26: rg3py.CppStandard.CXX_26,
        }
        cpp_std = std_map.get(cpp_standard, rg3py.CppStandard.CXX_DEFAULT)
        
        # Analyze headers if provided
        if headers:
            # For multi-file analysis, use AnalyzerContext
            if len(headers) > 1:
                context = rg3py.AnalyzerContext.make()
                context.set_headers(headers)
                context.cpp_standard = cpp_std
                
                for inc_dir in include_dirs:
                    context.add_project_include_dir(inc_dir)
                
                if compile_definitions:
                    context.set_compiler_defs(compile_definitions)
                
                context.analyze()
                analyzed_types = list(context.types)
                issues = list(context.issues)
            else:
                # Single file analysis
                header_path = headers[0]
                analyzer = rg3py.CodeAnalyzer.make()
                analyzer.set_file(header_path)
                analyzer.set_cpp_standard(cpp_std)
                
                for inc_dir in include_dirs:
                    analyzer.add_project_include_dir(inc_dir)
                
                if compile_definitions:
                    analyzer.set_definitions(compile_definitions)
                
                analyzer.analyze()
                analyzed_types = list(analyzer.types)
                issues = list(analyzer.issues)
        
        # Create the RG3Codegen context for the user's script
        from rg3_codegen import RG3CodegenContext
        rg3_context = RG3CodegenContext(
            analyzed_types=analyzed_types,
            issues=issues,
            output_dir=output_dir,
            headers=headers,
            sources=sources,
            include_dirs=include_dirs,
            compile_definitions=compile_definitions,
            cpp_standard=cpp_standard,
            discover_only=discover_only,
        )
        
        # Check if the codegen script exists
        codegen_script_abs = os.path.abspath(codegen_script)
        if not os.path.exists(codegen_script_abs):
            return {
                "outputs": [],
                "status": "error",
                "message": f"Codegen script not found: {codegen_script_abs}",
            }
        
        # Execute the codegen script in a controlled namespace
        # The script should define a main() function or write to rg3 context
        namespace = {
            "__file__": codegen_script_abs,
            "__name__": "__rg3_codegen__",
            "rg3": rg3_context,  # convenience alias
        }
        
        with open(codegen_script_abs, "r", encoding="utf-8") as f:
            script_code = f.read()
        
        exec(script_code, namespace)
        
        # Try to call main(rg3) if it exists
        if "main" in namespace and callable(namespace["main"]):
            namespace["main"](rg3_context)
        
        # Get discovered/generated files
        generated_files = rg3_context.get_generated_files()
        
        if not generated_files:
            return {
                "outputs": [],
                "status": "warning" if discover_only else "error",
                "message": "No files were declared. "
                           + ("In discover mode, call rg3.declare_outputs() in your script."
                              if discover_only
                              else "Make sure to call rg3.write_generated_files() in your codegen script."),
            }
        
        if discover_only:
            # In discover mode, just return the file names (don't check existence)
            return {
                "outputs": list(generated_files),
                "status": "success",
                "message": f"Discovered {len(generated_files)} output(s) (discover mode)",
            }
        else:
            # In generate mode, verify files exist
            actual_files = []
            for f in generated_files:
                full_path = os.path.join(output_dir, f) if not os.path.isabs(f) else f
                if os.path.exists(full_path):
                    actual_files.append(f)
                else:
                    return {
                        "outputs": [],
                        "status": "error",
                        "message": f"Expected output not found: {full_path}. "
                                   f"Did your codegen script create it?",
                    }
            
            return {
                "outputs": actual_files,
                "status": "success",
                "message": f"Generated {len(actual_files)} files",
            }
    
    except SystemExit:
        # Script called sys.exit() - treat as error
        return {
            "outputs": [],
            "status": "error",
            "message": "Codegen script called sys.exit()",
        }
    except Exception as e:
        return {
            "outputs": [],
            "status": "error",
            "message": f"Codegen failed: {str(e)}",
        }


def main():
    """Main entry point."""
    args = sys.argv[1:]
    
    if len(args) < 2:
        print(f"Usage: {sys.argv[0]} <input_manifest> <output_manifest> [--discover]",
              file=sys.stderr)
        sys.exit(1)
    
    input_manifest = args[0]
    output_manifest = args[1]
    discover_only = "--discover" in args
    
    try:
        manifest = load_input_manifest(input_manifest)
        result = run_codegen(manifest, discover_only=discover_only)
        write_output_manifest(output_manifest, result)
        
        if result["status"] == "success":
            print(f"[RG3] {result['message']}")
            sys.exit(0)
        elif result["status"] == "warning":
            print(f"[RG3] {result['message']}", file=sys.stderr)
            sys.exit(0)  # Warnings are non-fatal (especially in discover mode)
        else:
            print(f"[RG3] {result['message']}", file=sys.stderr)
            sys.exit(1)
    
    except json.JSONDecodeError as e:
        print(f"[RG3] Invalid JSON in manifest: {e}", file=sys.stderr)
        write_output_manifest(output_manifest, {
            "outputs": [],
            "status": "error",
            "message": f"Invalid manifest JSON: {str(e)}",
        })
        sys.exit(1)
    
    except Exception as e:
        print(f"[RG3] Unexpected error: {e}", file=sys.stderr)
        write_output_manifest(output_manifest, {
            "outputs": [],
            "status": "error",
            "message": f"Unexpected error: {str(e)}",
        })
        sys.exit(1)


if __name__ == "__main__":
    main()
