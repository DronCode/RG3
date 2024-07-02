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

		std::string rawCommentStr = rawComment->getFormattedText(sm, ctx.getDiagnostics());
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

		std::string typeName {};
		std::string enumPrettyName {};
		rg3::cpp::CppNamespace nameSpace {};

		Utils::getNamePrettyNameAndNamespaceForNamedDecl(enumDecl, typeName, enumPrettyName, nameSpace);

		// Location
		rg3::cpp::DefinitionLocation aDefLoc = Utils::getDeclDefinitionInfo(enumDecl);

		// Enum constants will be computed at CxxTypeVisitor::VisitEnumConstantDecl. Enum will be commited without any values

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

		m_collectedTypeIDs.emplace_back(enumDecl->getID()); // or use global id?

		if (typeName.find("::") != std::string::npos)
			m_collectedTypes.back()->setDeclaredInAnotherType(); // type name never contains :: when it's not declared inside another type

		return true;
	}

	bool CxxTypeVisitor::VisitEnumConstantDecl(clang::EnumConstantDecl* enumConstantDecl)
	{
		if (m_collectedTypes.empty() || m_collectedTypeIDs.empty()) return true; // wtf?
		if (m_collectedTypes.back()->getKind() != rg3::cpp::TypeKind::TK_ENUM) return true;

		if (const auto* parentEnumDecl = ::llvm::dyn_cast<clang::EnumDecl>(enumConstantDecl->getDeclContext()))
		{
			if (parentEnumDecl->getID() != m_collectedTypeIDs.back())
			{
				// Parent type is not same to our type. Skip entry
				return true;
			}
		}
		else
		{
			// Unable to identify parent type
			return true;
		}

		auto* pAsEnum = reinterpret_cast<cpp::TypeEnum*>(m_collectedTypes.back().get());

		// Calculate final value
		int64_t iVal = 0;

		if (const auto* pInitExpression = enumConstantDecl->getInitExpr(); pInitExpression && !pInitExpression->isValueDependent())
		{
			// Sometimes values are presented with expressions. So here we need to evaluate it
			clang::Expr::EvalResult r;
			pInitExpression->EvaluateAsInt(r, enumConstantDecl->getASTContext(), clang::Expr::SideEffectsKind::SE_NoSideEffects, true);
			iVal = r.Val.getInt().getExtValue();
		}
		else
		{
			// Usually this enough
			iVal = enumConstantDecl->getInitVal().getExtValue();
		}

		// Save
		cpp::EnumEntry& entry = pAsEnum->getEntries().emplace_back();
		entry.iValue = iVal;
		entry.sName = enumConstantDecl->getNameAsString();

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
					cppVisitor.foundFriends,
					cppVisitor.bIsStruct,
					cppVisitor.bTriviallyConstructible,
					cppVisitor.bHasCopyConstructor,
					cppVisitor.bHasCopyAssignOperator,
					cppVisitor.bHasMoveConstructor,
					cppVisitor.bHasMoveAssignOperator,
					cppVisitor.parentClasses
				)
			);

			m_collectedTypeIDs.emplace_back(cxxRecordDecl->getID());

			if (cppVisitor.bIsDeclaredInsideAnotherType)
			{
				m_collectedTypes.back()->setDeclaredInAnotherType();
			}
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
}