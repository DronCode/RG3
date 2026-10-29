# RG3 Python Codegen Framework

Python framework for writing code generation scripts that integrate with CMake
and use RG3 to analyze C++ code.

## Quick Start

### 1. Write your codegen script (`codegen.py`):

```python
import rg3py
from rg3_codegen import RG3CodegenContext

def main(rg: RG3CodegenContext = None):
    """Main codegen entry point.
    
    This function is called by rg3_codegen_entry.py with rg context.
    Can also be called directly for testing (with rg=None).
    
    Works in both discover mode (configure time) and generate mode (build time).
    
    IMPORTANT: declare_outputs() replaces the list each time it's called.
    Call it ONCE at the end with ALL generated files (using * unpacking).
    """
    generated = []  # collect all output files
    
    if rg is None:
        raise RuntimeError(
            "RG3 context is required. Run via rg3_codegen_entry.py or "
            "pass a RG3CodegenContext instance."
        )
    
    for type_info in rg.types:
        if type_info.kind != rg3py.CppTypeKind.TK_STRUCT_OR_CLASS:
            continue
        
        name = type_info.name
        
        # Build content
        content = f'#pragma once\nstruct {name}Gen {{}}\n'
        
        # Write file (no-op in discover mode, writes in generate mode)
        output = f"gen/{name}_gen.hpp"
        rg.write_file(output, content)
        
        # Collect output file name (DO NOT call declare_outputs here!)
        generated.append(output)
    
    # Declare ALL outputs at once (required! CMake needs to know this)
    if generated:
        rg.declare_outputs(*generated)

if __name__ == "__rg3_codegen__":
    main()
```

### 2. Use in CMake (NO OUTPUTS needed!):

```cmake
include(RG3Codegen)

add_codegen(
    TARGET my_codegen
    TARGET_NAME my_library
    HEADERS include/my_types.hpp
    CODEGEN_SCRIPT codegen.py
    # OUTPUTS are discovered automatically!
)
```

## How It Works

### Two-Pass Discovery

```
Configure time (Pass 1):                    Build time (Pass 2):
┌──────────────────────┐                   ┌──────────────────────┐
│ CMake runs codegen    │                   │ CMake runs codegen   │
│ in --discover mode    │──outputs──→│ CMake knows outputs    │
│ Python reports        │   file list     │ actually generates   │
│ what WOULD be         │                   │ files on disk        │
│ generated (no write)  │                   │                      │
└──────────────────────┘                   └──────────────────────┘
```

1. **Pass 1 (configure)**: Python runs with `discover_only=True`. Files aren't written, but `declare_outputs()` still registers them. CMake learns the file list.
2. **Pass 2 (build)**: Python runs normally. Files are actually written.

### User Script Pattern

```python
def main(rg: RG3CodegenContext = None):
    generated = []  # collect all output files
    
    if rg is None:
        raise RuntimeError("RG3 context is required. Run via rg3_codegen_entry.py")
    
    # 1. Analyze types
    for t in rg.types:
        # 2. Generate content
        content = "...code..."
        output = f"gen/{t.name}.hpp"
        
        # 3. Write file (no-op in discover, writes in generate)
        rg.write_file(output, content)
        
        # 4. Collect file name (DON'T call declare_outputs in loop!)
        generated.append(output)
    
    # 5. Declare ALL outputs at once (required!)
    if generated:
        rg.declare_outputs(*generated)
```

## API Reference

### RG3CodegenContext

Main context object injected into your codegen script.

```python
# Access analyzed types
for type_info in rg.types:
    print(f"Type: {type_info.pretty_name}")

# Access properties and functions
for prop in type_info.properties:
    print(f"  Property: {prop.name} : {prop.type_info.get_name()}")

for func in type_info.functions:
    print(f"  Function: {func.name}({', '.join(a.name for a in func.arguments)})")

# Find specific type
my_type = rg.get_type_by_name("MyClass")

# Find types by namespace
namespace_types = rg.get_types_by_namespace("my::lib")

# Write files (no-op in discover mode)
rg.write_file("output.hpp", "#pragma once\n// content")

# Declare output files (required for CMake)
rg.declare_outputs("output1.hpp", "output2.cpp")

# Access context info
print(f"Modes: discover={rg.discover_only}")
print(f"Types: {rg.type_count}")
print(f"Errors: {rg.has_errors}")
```

### CodeWriter

Convenient helper for writing formatted code.

```python
with rg.writer("output.hpp") as w:
    w.writeln("#pragma once")
    with w.indent():
        w.writeln("#include <string>")
    w.writeln()
    with w.block("namespace my {"):
        w.writeln("class MyClass {};")
```

## CMake Integration

### add_codegen() Parameters

| Parameter | Required | Description |
|-----------|----------|-------------|
| `TARGET` | Yes | Unique codegen target name |
| `CODEGEN_SCRIPT` | Yes | Path to user's Python codegen script |
| `TARGET_NAME` | No | CMake target to link generated files to |
| `SOURCES` | No | Source files to analyze |
| `HEADERS` | No | Header files to analyze |
| `INCLUDE_DIRS` | No | Additional include directories |
| `DEFINES` | No | Additional compile definitions |

**Note:** `OUTPUTS` is NOT required. Python discovers them automatically.

### Complete CMake Example

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.26)
project(MyCodegenProject)

# Add RG3 module
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake/Modules")
include(RG3Codegen)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Define your C++ headers to analyze
set(MY_HEADERS "include/types.hpp" "include/interface.hpp")

# Define your codegen script
set(MY_CODEGEN "${CMAKE_SOURCE_DIR}/codegen.py")

# Create codegen - outputs discovered automatically!
add_codegen(
    TARGET my_codegen
    TARGET_NAME my_lib
    HEADERS ${MY_HEADERS}
    CODEGEN_SCRIPT ${MY_CODEGEN}
    INCLUDE_DIRS include
)

# Regular library target
add_library(my_lib src/main.cpp)
target_include_directories(my_lib PUBLIC include)
```

## Example: Full Codegen Script

```python
"""
codegen.py - Generate serialization code from C++ types.
"""
import rg3py
from rg3_codegen import RG3CodegenContext

def main(rg: RG3CodegenContext):
    """Main entry point."""
    generated = []
    
    for type_info in rg.types:
        # Only process structs/classes
        if type_info.kind != rg3py.CppTypeKind.TK_STRUCT_OR_CLASS:
            continue
        
        name = type_info.name
        header_path = f"gen/{name}_serialize.hpp"
        
        # Build content
        content = f"""#pragma once
#include "{name}.hpp"
#include <string>
#include <sstream>

namespace gen {{
    template <>
    struct Serialize {{
        static std::string serialize(const {name}& obj) {{
            std::ostringstream ss;
"""
        for prop in type_info.properties:
            content += f'            ss << "{prop.name}" << obj.{prop.name};\n'
        content += """            return ss.str();
        }
    }};
}
"""
        
        # Write file (no-op in discover mode)
        rg.write_file(header_path, content)
        
        # Declare output
        generated.append(header_path)
    
    # Declare all outputs for CMake
    if generated:
        rg.declare_outputs(*generated)
```

## Key Principles

1. **`declare_outputs()` is always required** — CMake needs to know what files to expect
2. **`write_file()` is safe in both modes** — no-op in discover, writes in generate
3. **Same script for both modes** — no conditional logic needed
4. **Python discovers, CMake tracks** — you don't need to pre-declare outputs in CMake

## CI / Testing

### Testing the codegen integration

Run the integration tests:

```bash
cd Tests/PyIntegration
python test_codegen_integration.py
```

This tests:
- **Discover mode**: Python reports outputs without writing files
- **Generate mode**: Python writes actual files
- **Content validation**: Generated files have correct structure
- **Error handling**: Missing scripts are handled gracefully

### Example header

The `Usage/codegen_examples/include/example_types.hpp` file contains
sample C++ types (`User`, `Product`, `Order`, `OrderStatus`) that the
example codegen script analyzes. This is used both for CI testing and
as a reference for users.

### CI workflow (GitHub Actions)

```yaml
- name: Test RG3 Codegen
  run: |
    cd Tests/PyIntegration
    python test_codegen_integration.py
```

The test runs the codegen entry point in both discover and generate
modes, verifying that:
1. Correct output files are discovered
2. Generated files exist on disk after generate mode
3. Generated content has correct structure
