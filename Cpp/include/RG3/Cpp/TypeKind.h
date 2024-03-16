#pragma once


namespace rg3::cpp
{
    enum class TypeKind : int
    {
        TK_NONE = 0,				///< See TypeBase
        TK_TRIVIAL,   				///< See TypeBase
        TK_ENUM,      				///< See TypeEnum
        TK_STRUCT_OR_CLASS, 		///< See TypeClass
        TK_ALIAS,                   ///< See TypeAlias
        TK_TEMPLATE_SPECIALIZATION
    };
}