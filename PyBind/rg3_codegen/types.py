"""
Types helpers - Utilities for working with analyzed C++ types.

Provides convenience functions for extracting type information
from rg3py analyzed types.
"""

from typing import List, Optional, Any


def wrap_type(type_obj: Any) -> dict:
    """
    Convert an rg3py type object to a plain dict.
    
    Useful for serialization or passing to templates.
    
    Args:
        type_obj: An rg3py CppBaseType / CppEnum / CppClass object
        
    Returns:
        Dict with type information
    """
    result = {
        "name": type_obj.name,
        "pretty_name": type_obj.pretty_name,
        "kind": type_obj.kind,
        "namespace": str(type_obj.namespace),
        "is_struct": getattr(type_obj, 'is_struct', None),
        "is_enum": hasattr(type_obj, 'entries'),
    }
    
    # Add class-specific info
    if hasattr(type_obj, 'properties'):
        result["properties"] = [
            {
                "name": p.name,
                "alias": p.alias,
                "type": p.type_info.get_name(),
                "visibility": p.visibility,
                "is_static": False,
            }
            for p in type_obj.properties
        ]
    
    if hasattr(type_obj, 'functions'):
        result["functions"] = [
            {
                "name": f.name,
                "owner": f.owner,
                "return_type": f.return_type.get_name(),
                "arguments": [
                    {"name": a.name, "type": a.type_info.get_name()}
                    for a in f.arguments
                ],
                "is_static": f.is_static,
                "is_const": f.is_const,
                "is_noexcept": f.is_noexcept,
            }
            for f in type_obj.functions
        ]
    
    # Add enum-specific info
    if hasattr(type_obj, 'entries'):
        result["entries"] = [
            {"name": e.name, "value": e.value}
            for e in type_obj.entries
        ]
        result["is_scoped"] = type_obj.is_scoped
        result["underlying_type"] = type_obj.underlying_type
    
    # Add parent types
    if hasattr(type_obj, 'parent_types'):
        result["parent_types"] = [
            {"name": pt.info.pretty_name, "inheritance": pt.inheritance}
            for pt in type_obj.parent_types
        ]
    
    return result


def get_type_members(type_obj: Any) -> dict:
    """
    Get all members (properties + functions) of a class type.
    
    Args:
        type_obj: A rg3py CppClass object
        
    Returns:
        Dict with 'properties' and 'functions' lists
    """
    result = {"properties": [], "functions": []}
    
    if hasattr(type_obj, 'properties'):
        for p in type_obj.properties:
            result["properties"].append({
                "name": p.name,
                "type": p.type_info.get_name(),
                "visibility": p.visibility,
            })
    
    if hasattr(type_obj, 'functions'):
        for f in type_obj.functions:
            result["functions"].append({
                "name": f.name,
                "return_type": f.return_type.get_name(),
                "arguments": [
                    {"name": a.name, "type": a.type_info.get_name()}
                    for a in f.arguments
                ],
                "is_static": f.is_static,
                "is_const": f.is_const,
            })
    
    return result


def is_runtime_type(type_obj: Any) -> bool:
    """
    Check if a type has the @runtime tag.
    
    Args:
        type_obj: An rg3py type object
        
    Returns:
        True if the type has @runtime tag
    """
    try:
        return type_obj.tags.has_tag("runtime")
    except Exception:
        return False


def find_type_by_name(types: List[Any], name: str) -> Optional[Any]:
    """
    Find a type by its simple or fully-qualified name.
    
    Args:
        types: List of rg3py types
        name: Type name to search for
        
    Returns:
        The type if found, None otherwise
    """
    for t in types:
        if t.name == name or t.pretty_name == name:
            return t
    return None


def get_public_members(type_obj: Any, visibility_filter: int = 0) -> dict:
    """
    Get only public members of a class.
    
    Args:
        type_obj: A rg3py CppClass object
        visibility_filter: CppClassEntryVisibillity value (0=public)
        
    Returns:
        Dict with filtered properties and functions
    """
    members = get_type_members(type_obj)
    
    if visibility_filter == 0:  # public only
        members["properties"] = [
            p for p in members["properties"] if p["visibility"] == 0
        ]
        members["functions"] = [
            f for f in members["functions"] if f.get("visibility", 0) == 0
        ]
    
    return members
