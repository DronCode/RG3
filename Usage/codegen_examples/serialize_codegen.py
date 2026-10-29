"""
serialize_codegen.py - Generate serialization code from C++ types.

This example analyzes C++ structs with @runtime tag and generates
serialization wrappers. It demonstrates:
1. Accessing analyzed types via RG3
2. Using CodeWriter for formatted output
3. Generating header files from class definitions
4. Declaring generated files for CMake

Usage:
    # Via CMake (two-pass discovery)
    add_codegen(
        TARGET serialize_codegen
        TARGET_NAME my_lib
        HEADERS include/example_types.hpp
        CODEGEN_SCRIPT serialize_codegen.py
    )
    
    # Directly (for testing)
    python serialize_codegen.py
"""

import rg3py
from rg3_codegen import RG3CodegenContext


def generate_serialize_header(rg: RG3CodegenContext, type_info) -> str:
    """Generate serialization header for a single struct type."""
    name = type_info.name
    namespace = str(type_info.namespace)
    simple_name = name.split("::")[-1] if "::" in name else name
    
    header_path = f"gen/{simple_name}_serialize.hpp"
    
    # Build include guard
    guard = f"GEN_{simple_name}_SERIALIZE_HPP"
    
    lines = [
        f"#pragma once",
        "",
        f'// Auto-generated serialization code for {name}',
        "",
        f'#include "{simple_name}.hpp"',
        f"#include <string>",
        f"#include <sstream>",
        "",
    ]
    
    # Add namespace wrappers
    if namespace:
        for ns_part in namespace.split("::"):
            lines.append(f"namespace {ns_part} {{")
        lines.append("")
    
    # Add Serialize template specialization
    lines.append("namespace gen {")
    lines.append("")
    lines.append("    template <>")
    lines.append("    struct Serialize {")
    lines.append(f"        using Type = {simple_name};")
    lines.append("")
    
    # serialize method
    lines.append("        static std::string serialize(const Type& obj) {")
    lines.append("            std::ostringstream ss;")
    lines.append("            ss << \"{\";")
    
    for prop in type_info.properties:
        prop_name = prop.name
        lines.append(f'            ss << "{prop_name}=" << obj.{prop_name} << ", ";')
    
    lines.append('            ss << "}";')
    lines.append("            return ss.str();")
    lines.append("        }")
    lines.append("")
    
    # deserialize stub
    lines.append("        static Type deserialize(const std::string& data) {")
    lines.append("            Type result{};")
    lines.append("            // TODO: implement deserialization")
    lines.append("            return result;")
    lines.append("        }")
    lines.append("    };")
    lines.append("")
    lines.append("} // namespace gen")
    lines.append("")
    
    # Close namespace
    if namespace:
        for _ in namespace.split("::"):
            lines.append("}")
        lines.append("")
    
    content = "\n".join(lines)
    
    # Write the file (no-op in discover mode, writes in generate mode)
    rg.write_file(header_path, content)
    
    return header_path


def main(rg: RG3CodegenContext = None):
    """Main codegen entry point."""
    if rg is None:
        raise RuntimeError(
            "RG3 context is required. "
            "Run via rg3_codegen_entry.py or pass a RG3CodegenContext instance."
        )
    
    print(f"[codegen] Analyzing {rg.type_count} types from {len(rg.headers)} header(s)...")
    
    generated = []
    
    for type_info in rg.types:
        # Only process structs/classes (not enums, not trivial types)
        if type_info.kind != rg3py.CppTypeKind.TK_STRUCT_OR_CLASS:
            continue
        
        name = type_info.name
        
        # Skip if no properties (empty struct)
        if not hasattr(type_info, 'properties') or not type_info.properties:
            print(f"  Skipping {name} (no properties)")
            continue
        
        print(f"  Generating for: {name} ({len(type_info.properties)} properties)")
        
        header_path = generate_serialize_header(rg, type_info)
        generated.append(header_path)
    
    # Declare all generated files for CMake
    if generated:
        rg.declare_outputs(*generated)
        mode = "discover" if rg.discover_only else "generate"
        print(f"[codegen] {mode} mode: {len(generated)} output(s) {'declared' if rg.discover_only else 'written'}")
    else:
        print("[codegen] No outputs generated (no matching types found)")


if __name__ == "__rg3_codegen__":
    main()
