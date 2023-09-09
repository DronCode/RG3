#include <RG3/LLVM/Visitors/CxxClassTypeVisitor.h>
#include <RG3/LLVM/Visitors/CxxTypeVisitor.h>
#include <RG3/Cpp/BuiltinTags.h>

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/SourceManager.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Comment.h>

#include <llvm/ADT/StringRef.h>

#include <RG3/LLVM/Utils.h>
#include <RG3/Cpp/TypeEnum.h>
#include <RG3/Cpp/TypeClass.h>
#include <RG3/Cpp/BuiltinTags.h>


namespace rg3::llvm::visitors
{
	CxxTypeVisitor::CxxTypeVisitor(std::vector<rg3::cpp::TypeBasePtr>& collectedTypes, const CompilerConfig& cc) : m_collectedTypes(collectedTypes), compilerConfig(cc)
	{
	}

	bool CxxTypeVisitor::VisitEnumDecl(clang::EnumDecl* enumDecl)
	{
		// Extract comment
		clang::ASTContext& ctx = enumDecl->getASTContext();
		clang::SourceManager& sm = ctx.getSourceManager();

		const clang::RawComment* rawComment = ctx.getRawCommentForDeclNoCache(enumDecl);
		if (!rawComment)
			return true;

		std::string rawCommentStr = rawComment->getRawText(sm).data();
		cpp::Tags tags = cpp::Tag::parseFromCommentString(rawCommentStr);

		// Check this somewhere else
		if (!tags.hasTag(std::string(rg3::cpp::BuiltinTags::kRuntime)) && !compilerConfig.bAllowCollectNonRuntimeTypes)
			return true;

		// Create entry
		bool bScoped = enumDecl->isScoped();
		cpp::EnumEntryVector entries;

		std::string typeName = enumDecl->getName().str();
		rg3::cpp::CppNamespace nameSpace;
		rg3::llvm::Utils::getDeclInfo(enumDecl, nameSpace);

		// Location
		rg3::cpp::DefinitionLocation aDefLoc = Utils::getDeclDefinitionInfo(enumDecl);

		for (const auto& enumerator : enumDecl->enumerators())
		{
			const std::string sEntryName = enumerator->getNameAsString();
			int64_t iEntryValue = enumerator->getInitVal().getExtValue();
			entries.emplace_back(sEntryName, iEntryValue);
		}

		// Take underlying type
		cpp::TypeReference underlyingTypeRef;
		clang::QualType underlyingType = enumDecl->getIntegerType();

		if (underlyingType->isBuiltinType() && underlyingType->getAs<clang::BuiltinType>()->getKind() == clang::BuiltinType::Int)
		{
			underlyingTypeRef = cpp::TypeReference();
		}
		else
		{
			underlyingTypeRef = cpp::TypeReference(underlyingType.getAsString());
		}

		m_collectedTypes.emplace_back(
			std::make_unique<rg3::cpp::TypeEnum>(
				typeName,
				nameSpace,
				aDefLoc,
				tags,
				entries,
				bScoped,
				underlyingTypeRef
			)
		);

		return true;
	}

	bool CxxTypeVisitor::VisitCXXRecordDecl(clang::CXXRecordDecl* cxxRecordDecl)
	{
		// Logic is too huge, we need to move logic into another unit
		visitors::CxxClassTypeVisitor cppVisitor { compilerConfig };
		cppVisitor.TraverseDecl(cxxRecordDecl);

		if (!cppVisitor.sClassName.empty())
		{
			m_collectedTypes.emplace_back(
				std::make_unique<rg3::cpp::TypeClass>(
					cppVisitor.sClassName,
					cppVisitor.sNameSpace,
					cppVisitor.sDefinitionLocation,
					cppVisitor.vTags,
					cppVisitor.foundProperties,
					cppVisitor.foundFunctions,
					cppVisitor.bIsStruct,
					cppVisitor.bTriviallyConstructible,
					cppVisitor.parentClasses
				)
			);
		}

		return true;
	}
}