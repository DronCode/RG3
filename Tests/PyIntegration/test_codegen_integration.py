"""
Integration tests for RG3 codegen CMake integration.

These tests verify:
1. Discover mode correctly reports output files without writing
2. Generate mode correctly writes output files
3. Generated files contain expected content
4. The entry point handles errors gracefully
"""

import json
import os
import sys
import tempfile
import shutil

# Add the PyBind directory to path so we can import the entry point
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "PyBind", "source"))
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "PyBind"))

from rg3_codegen_entry import load_input_manifest, run_codegen, write_output_manifest


# ---- Paths ----
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.join(SCRIPT_DIR, "..", "..")
ENTRY_POINT = os.path.join(PROJECT_ROOT, "PyBind", "source", "rg3_codegen_entry.py")
EXAMPLE_CODEGEN = os.path.join(PROJECT_ROOT, "Usage", "codegen_examples", "serialize_codegen.py")
EXAMPLE_HEADERS_DIR = os.path.join(PROJECT_ROOT, "Usage", "codegen_examples", "include")
EXAMPLE_HEADER = os.path.join(EXAMPLE_HEADERS_DIR, "example_types.hpp")


def create_test_manifest(output_dir: str) -> dict:
    """Create a test input manifest."""
    return {
        "headers": [EXAMPLE_HEADER],
        "sources": [],
        "include_dirs": [EXAMPLE_HEADERS_DIR],
        "compile_definitions": [],
        "cpp_standard": 20,
        "output_dir": output_dir,
        "codegen_script": EXAMPLE_CODEGEN,
    }


def test_discover_mode():
    """Test that discover mode reports outputs without writing files."""
    print("\n=== Test: Discover Mode ===")
    
    with tempfile.TemporaryDirectory() as tmpdir:
        manifest = create_test_manifest(tmpdir)
        result = run_codegen(manifest, discover_only=True)
        
        # Should succeed
        assert result["status"] == "success", f"Discover failed: {result['message']}"
        
        # Should have discovered output files
        outputs = result.get("outputs", [])
        print(f"Discovered outputs: {outputs}")
        assert len(outputs) > 0, "No outputs discovered"
        
        # Verify expected files are in the list
        output_basenames = [os.path.basename(f) for f in outputs]
        
        # Should have serialization files for User, Product, Order (not enum)
        assert any("User_serialize.hpp" in f for f in output_basenames), \
            f"User_serialize.hpp not found in {output_basenames}"
        assert any("Product_serialize.hpp" in f for f in output_basenames), \
            f"Product_serialize.hpp not found in {output_basenames}"
        assert any("Order_serialize.hpp" in f for f in output_basenames), \
            f"Order_serialize.hpp not found in {output_basenames}"
        
        # Files should NOT exist on disk (discover mode doesn't write)
        for output in outputs:
            full_path = os.path.join(tmpdir, output) if not os.path.isabs(output) else output
            assert not os.path.exists(full_path), \
                f"File {output} should NOT exist in discover mode"
        
        print(f"  ✓ {len(outputs)} outputs discovered, files not written")


def test_generate_mode():
    """Test that generate mode writes output files."""
    print("\n=== Test: Generate Mode ===")
    
    with tempfile.TemporaryDirectory() as tmpdir:
        manifest = create_test_manifest(tmpdir)
        result = run_codegen(manifest, discover_only=False)
        
        # Should succeed
        assert result["status"] == "success", f"Generate failed: {result['message']}"
        
        # Should have generated files
        outputs = result.get("outputs", [])
        print(f"Generated outputs: {outputs}")
        assert len(outputs) > 0, "No files generated"
        
        # Verify files exist on disk
        for output in outputs:
            full_path = os.path.join(tmpdir, output) if not os.path.isabs(output) else output
            assert os.path.exists(full_path), f"Generated file not found: {full_path}"
        
        # Verify file content
        for output in outputs:
            full_path = os.path.join(tmpdir, output) if not os.path.isabs(output) else output
            with open(full_path, "r", encoding="utf-8") as f:
                content = f.read()
            
            # Should contain #pragma once
            assert "#pragma once" in content, \
                f"{output} missing #pragma once"
            
            # Should contain Serialize struct
            assert "struct Serialize" in content, \
                f"{output} missing Serialize struct"
            
            # Should contain include for original header
            type_name = output.replace("_serialize.hpp", "").split("/")[-1]
            assert f'#include "{type_name}.hpp"' in content, \
                f"{output} missing #include for {type_name}"
            
            print(f"  ✓ {os.path.basename(output)}: valid content")


def test_manifest_roundtrip():
    """Test that write_output_manifest and load_input_manifest work correctly."""
    print("\n=== Test: Manifest Roundtrip ===")
    
    with tempfile.TemporaryDirectory() as tmpdir:
        input_path = os.path.join(tmpdir, "input.json")
        output_path = os.path.join(tmpdir, "output.json")
        
        # Write input
        manifest = create_test_manifest(tmpdir)
        json.dump(manifest, open(input_path, "w"))
        
        # Read it back
        loaded = load_input_manifest(input_path)
        assert loaded["headers"] == manifest["headers"]
        assert loaded["output_dir"] == manifest["output_dir"]
        assert loaded["codegen_script"] == manifest["codegen_script"]
        
        # Write output
        output_data = {
            "outputs": ["file1.hpp", "file2.cpp"],
            "status": "success",
            "message": "test",
        }
        write_output_manifest(output_path, output_data)
        
        # Read it back
        with open(output_path) as f:
            read_back = json.load(f)
        assert read_back["outputs"] == output_data["outputs"]
        assert read_back["status"] == output_data["status"]
        
        print("  ✓ Manifest read/write works correctly")


def test_error_handling():
    """Test error handling for missing codegen script."""
    print("\n=== Test: Error Handling ===")
    
    with tempfile.TemporaryDirectory() as tmpdir:
        manifest = create_test_manifest(tmpdir)
        manifest["codegen_script"] = "/nonexistent/path/codegen.py"
        
        result = run_codegen(manifest)
        assert result["status"] == "error"
        assert "not found" in result["message"].lower()
        print(f"  ✓ Error handled: {result['message']}")


if __name__ == "__main__":
    tests = [
        test_discover_mode,
        test_generate_mode,
        test_manifest_roundtrip,
        test_error_handling,
    ]
    
    passed = 0
    failed = 0
    
    for test in tests:
        try:
            test()
            passed += 1
        except AssertionError as e:
            print(f"  ✗ FAILED: {e}")
            failed += 1
        except Exception as e:
            print(f"  ✗ ERROR: {type(e).__name__}: {e}")
            failed += 1
    
    print(f"\n{'='*50}")
    print(f"Results: {passed} passed, {failed} failed")
    
    sys.exit(1 if failed else 0)
