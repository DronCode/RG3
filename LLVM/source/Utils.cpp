#include <RG3/LLVM/Utils.h>
#include <clang/Basic/SourceManager.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Decl.h>
#include <llvm/Support/Casting.h>
#include <filesystem>


namespace rg3::llvm
{
	void Utils::getDeclInfo(const clang::Decl* decl, rg3::cpp::CppNamespace& nameSpace)
	{
		const clang::DeclContext* context = decl->getDeclContext();
		while (context) {
			if (const clang::NamespaceDecl* nsDecl = ::llvm::dyn_cast<clang::NamespaceDecl>(context)) {
#ifdef __APPLE__
				// v0.0.3 workaround: I hate macOS for this. __1 inside namespace is an awful practice. So, I'm fixing that by this shitty fix, sorry :(
				// !!! Pls, fix me later !!!
				if (nsDecl->getName().str() == "__1")
				{
					context = context->getParent();
					continue;
				}
#endif

				nameSpace.prepend(nsDecl->getName().str());
			}
			context = context->getParent();
		}
	}

	cpp::DefinitionLocation Utils::getDeclDefinitionInfo(const clang::Decl* decl)
	{
		clang::ASTContext& ctx = decl->getASTContext();
		clang::SourceManager& sm = ctx.getSourceManager();

		clang::SourceRange declSourceRange = decl->getSourceRange();
		clang::SourceLocation startLoc = declSourceRange.getBegin();

		std::string locationPath;
		if (auto l = sm.getFilename(startLoc); !l.empty())
		{
			// Path defined and not empty
			locationPath = l.data();
		}
		else
		{
			// Let's try to locate type by another way (maybe it's better than previous approach, who knows)
			clang::PresumedLoc pr = sm.getPresumedLoc(decl->getBeginLoc());

			if (pr.isValid())
			{
				locationPath = pr.getFilename();
			}
			else
			{
				locationPath = {};
			}
		}

		unsigned int locationLine = sm.getSpellingLineNumber(startLoc);
		unsigned int offset = sm.getSpellingColumnNumber(startLoc);

		return {
			std::filesystem::path{ locationPath },
			static_cast<int>(locationLine),
			static_cast<int>(offset)
		};
	}

	cpp::ClassEntryVisibility Utils::getDeclVisibilityLevel(const clang::Decl* decl)
	{
		if (decl->getAccess() == clang::AS_public)
			return cpp::ClassEntryVisibility::CEV_PUBLIC;

		if (decl->getAccess() == clang::AS_private)
			return cpp::ClassEntryVisibility::CEV_PRIVATE;

		if (decl->getAccess() == clang::AS_protected)
			return cpp::ClassEntryVisibility::CEV_PROTECTED;

		// Unknown case
		return cpp::ClassEntryVisibility::CEV_PRIVATE;
	}

	std::string Utils::getNormalizedTypeRef(const std::string& typeName)
	{
		// Here stored known cases when we need to replace one type to another
		static const std::unordered_map<std::string, std::string> s_Replacement {
			{ "_Bool", "bool" }
		};

		if (auto it = s_Replacement.find(typeName); it != s_Replacement.end())
		{
			return it->second;
		}

		return typeName;
	}

	void Utils::fillTypeStatementFromQualType(rg3::cpp::TypeStatement& typeStatement, clang::QualType qt, const clang::ASTContext& astContext)
	{
		const clang::SourceManager& sm = astContext.getSourceManager();

		typeStatement.sTypeRef = cpp::TypeReference(rg3::llvm::Utils::getNormalizedTypeRef(qt.getAsString()));

		typeStatement.bIsConst = qt.isConstQualified();

		if (qt->isPointerType() || qt->isReferenceType())
		{
			typeStatement.bIsPointer = qt->isPointerType();
			typeStatement.bIsReference = qt->isReferenceType();
			typeStatement.sTypeRef = cpp::TypeReference(rg3::llvm::Utils::getNormalizedTypeRef(qt->getPointeeType().getUnqualifiedType().getAsString()));
			typeStatement.bIsPtrConst = qt->getPointeeType().isConstQualified();
		}

		if (qt->getAs<clang::TemplateSpecializationType>())
		{
			typeStatement.bIsTemplateSpecialization = true;
		}

		// Try extract location info
		const clang::Type* pType = typeStatement.bIsReference || typeStatement.bIsPointer ? qt->getPointeeType().getUnqualifiedType().getTypePtr() : qt.getTypePtr();
		if (const auto* pRecord = pType->getAs<clang::RecordType>())
		{
			const clang::RecordDecl* pRecordDecl = pRecord->getDecl();
			if (pRecordDecl)
			{
				clang::SourceLocation location = pRecordDecl->getLocation();
				clang::PresumedLoc presumedLoc = sm.getPresumedLoc(location);

				if (presumedLoc.isValid())
				{
					typeStatement.sDefinitionLocation.emplace(
						std::filesystem::path(presumedLoc.getFilename()),
						presumedLoc.getLine(),
						presumedLoc.getColumn()
					);

					/*
					 * Maybe it's better, who knows...
					typeStatement.sDefinitionLocation.emplace(
							std::filesystem::path(presumedLoc.getIncludeLoc().printToString(sm)),
							presumedLoc.getLine(),
							presumedLoc.getColumn()
					);
					 */
				}
			}
		}
	}

	std::string Utils::getPrettyNameOfDecl(clang::NamedDecl* pDecl)
	{
		if (!pDecl) return {};

		const clang::DeclContext* pDeclContext = pDecl->getDeclContext();

		if (!pDeclContext->isNamespace() && !pDeclContext->isRecord()) {
			// We aren't namespace or record (struct, class)
			return pDecl->getNameAsString();
		}

		clang::NamedDecl* parentDecl = nullptr;
		if (pDeclContext->isNamespace())
		{
#ifdef __APPLE__
			bool bAllowed = true;

			// v0.0.3 workaround: I hate macOS for this. __1 inside namespace is an awful practice. So, I'm fixing that by this shitty fix, sorry :(
			// !!! Pls, fix me later !!!
			if (auto* pNamespaceDecl = ::llvm::dyn_cast<clang::NamespaceDecl>(pDeclContext))
			{
				bAllowed = pNamespaceDecl->getNameAsString() != "__1";
			}

			parentDecl = bAllowed ? clang::NamespaceDecl::castFromDeclContext(pDeclContext) : nullptr;
#else
			parentDecl = clang::NamespaceDecl::castFromDeclContext(pDeclContext);
#endif
		}
		else if (pDeclContext->isRecord())
		{
			parentDecl = clang::RecordDecl::castFromDeclContext(pDeclContext);
		}

		std::string parentName {};

		if (parentDecl)
		{
			parentName = getPrettyNameOfDecl(parentDecl);
		}

		if (!parentName.empty())
		{
			return parentName + "::" + pDecl->getNameAsString();
		}

		return pDecl->getNameAsString();
	}
}