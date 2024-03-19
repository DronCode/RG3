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
#include <RG3/Cpp/TypeAlias.h>
#include <RG3/Cpp/BuiltinTags.h>

#include <cassert>


namespace rg3::llvm::visitors
{
	static cpp::Tags getTagsForDecl(clang::Decl* pDecl, bool& bHasComment)
	{
		// Extract comment
		clang::ASTContext& ctx = pDecl->getASTContext();
		clang::SourceManager& sm = ctx.getSourceManager();

		const clang::RawComment* rawComment = ctx.getRawCommentForDeclNoCache(pDecl);
		if (!rawComment)
		{
			bHasComment = false;
			return {};
		}

		std::string rawCommentStr = rawComment->getRawText(sm).data();
		bHasComment = !rawCommentStr.empty();
		return cpp::Tag::parseFromCommentString(rawCommentStr);
	}

	CxxTypeVisitor::CxxTypeVisitor(std::vector<rg3::cpp::TypeBasePtr>& collectedTypes, const CompilerConfig& cc) : m_collectedTypes(collectedTypes), compilerConfig(cc)
	{
	}

	bool CxxTypeVisitor::VisitEnumDecl(clang::EnumDecl* enumDecl)
	{
		if (!enumDecl->isCompleteDefinition())
			return true; // skip incomplete declarations

		// Extract tags
		bool bHasComment = false;
		cpp::Tags tags = getTagsForDecl(enumDecl, bHasComment);

		if (!bHasComment && !compilerConfig.bAllowCollectNonRuntimeTypes)
		{
			// skip this decl
			return true;
		}

		// Check this somewhere else
		if (!tags.hasTag(std::string(rg3::cpp::BuiltinTags::kRuntime)) && !compilerConfig.bAllowCollectNonRuntimeTypes)
			return true;

		// Create entry
		bool bScoped = enumDecl->isScoped();
		cpp::EnumEntryVector entries;

		std::string typeName = enumDecl->getName().str();
		std::string enumPrettyName = Utils::getPrettyNameOfDecl(enumDecl);
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
				enumPrettyName,
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
					cppVisitor.sClassPrettyName,
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

		// Save all extra types
		std::move(std::begin(cppVisitor.vFoundExtraTypes),
				  std::end(cppVisitor.vFoundExtraTypes),
				  std::back_inserter(m_collectedTypes));

		return true;
	}

	void fillTypeStatementFromUnderlyingType(rg3::cpp::TypeStatement& stmt, clang::QualType underlyingType, clang::ASTContext& astCtx)
	{
		auto canonicalType = underlyingType.getCanonicalType();

		rg3::llvm::Utils::fillTypeStatementFromQualType(stmt, canonicalType, astCtx);

		std::string sFinalName;

		// take canonical & make printing policy
		clang::PrintingPolicy policy = astCtx.getLangOpts();
		policy.SuppressTagKeyword = true;
		policy.SuppressScope = false;

		if (underlyingType.isCanonical())
		{
			sFinalName = underlyingType.getAsString(policy);
		}
		else
		{
			if (canonicalType->isPointerType())
			{
				sFinalName = canonicalType->getPointeeType().getUnqualifiedType().getAsString(policy);
			}
			else
			{
				// It's finalized behaviour (see Tests_TypeAliasing::CheckTrivialTypeAliasing test for details!)
				sFinalName = canonicalType.getAsString(policy);

				// Probably it's template spec? need verify data
				stmt.bIsTemplateSpecialization = underlyingType->getAs<clang::TemplateSpecializationType>() != nullptr;
			}
		}

		stmt.sTypeRef = rg3::cpp::TypeReference {
			sFinalName
		};
	}

	bool CxxTypeVisitor::VisitTypedefDecl(clang::TypedefDecl* typedefDecl)
	{
		return HandleNamedTypedefDecl(::llvm::dyn_cast<clang::TypedefNameDecl>(typedefDecl));
	}

	bool CxxTypeVisitor::VisitTypeAliasDecl(clang::TypeAliasDecl* typeAliasDecl)
	{
		return HandleNamedTypedefDecl(::llvm::dyn_cast<clang::TypedefNameDecl>(typeAliasDecl));
	}

	bool CxxTypeVisitor::VisitTypedefNameDecl(clang::TypedefNameDecl* typedefNameDecl)
	{
		return HandleNamedTypedefDecl(typedefNameDecl);
	}

	bool CxxTypeVisitor::HandleNamedTypedefDecl(clang::TypedefNameDecl* typedefNameDecl)
	{
		if (!typedefNameDecl) { return true; }

		// Extract tags
		bool bHasComment = false;
		cpp::Tags tags = getTagsForDecl(typedefNameDecl, bHasComment);

		if (!bHasComment && !compilerConfig.bAllowCollectNonRuntimeTypes)
		{
			return true;
		}

		// Check this somewhere else
		if (!tags.hasTag(std::string(rg3::cpp::BuiltinTags::kRuntime)) && !compilerConfig.bAllowCollectNonRuntimeTypes)
			return true;

		// Extract base info
		const std::string sName = typedefNameDecl->getNameAsString();
		const std::string sPrettyName = Utils::getPrettyNameOfDecl(typedefNameDecl);
		rg3::cpp::CppNamespace sNamespace {};
		Utils::getDeclInfo(typedefNameDecl, sNamespace);

		rg3::cpp::DefinitionLocation sDefLoc = Utils::getDeclDefinitionInfo(typedefNameDecl);

		// Get base information about canonical type
		rg3::cpp::TypeStatement stmt {};
		fillTypeStatementFromUnderlyingType(stmt, typedefNameDecl->getUnderlyingType(), typedefNameDecl->getASTContext());

		// Save found type
		m_collectedTypes.emplace_back(std::make_unique<rg3::cpp::TypeAlias>(sName, sPrettyName, sNamespace, sDefLoc, tags, stmt));

		return true;
	}
}