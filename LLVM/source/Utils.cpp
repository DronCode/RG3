#include <RG3/LLVM/Utils.h>
#include <RG3/LLVM/Visitors/CxxTemplateSpecializationVisitor.h>
#include <RG3/LLVM/Visitors/CxxClassTypeVisitor.h>
#include <RG3/LLVM/Visitors/CxxTypeVisitor.h>
#include <clang/Basic/SourceManager.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Decl.h>
#include <llvm/Support/Casting.h>
#include <boost/algorithm/string.hpp>
#include <filesystem>
#include <fmt/format.h>


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

	bool Utils::getQualTypeBaseInfo(const clang::QualType& qualType, cpp::TypeBaseInfo& baseInfo, const clang::ASTContext& astContext)
	{
		if (qualType->isTypedefNameType()) // must be first!
		{
			// Alias. Just use alias name & location data
			if (auto* pAsAliasType = qualType->getAs<clang::TypedefType>())
			{
				if (auto* pAsAliasDecl = pAsAliasType->getDecl())
				{
					// Collect definition location
					baseInfo.sDefLocation = Utils::getDeclDefinitionInfo(pAsAliasDecl);

					// Get info
					Utils::getNamePrettyNameAndNamespaceForNamedDecl(pAsAliasDecl, baseInfo.sName, baseInfo.sPrettyName, baseInfo.sNameSpace);

					// Detect kind
					if (qualType->isRecordType())
					{
						baseInfo.eKind = cpp::TypeKind::TK_STRUCT_OR_CLASS;
					}
					else if (qualType->isEnumeralType() || qualType->isScopedEnumeralType())
					{
						baseInfo.eKind = cpp::TypeKind::TK_ENUM;
					}
					else
					{
						baseInfo.eKind = cpp::TypeKind::TK_TRIVIAL;
					}

					return true;
				}
			}

			return false;
		}

		if (qualType->isRecordType())
		{
			// No need to support full type analysis pipeline here. Just lookup as 'generic record' and trying to extract type
			if (auto* pAsRecord = qualType->getAsRecordDecl())
			{
				// Collect data
				Utils::getNamePrettyNameAndNamespaceForNamedDecl(pAsRecord, baseInfo.sName, baseInfo.sPrettyName, baseInfo.sNameSpace);

				// Collect definition location
				cpp::DefinitionLocation aDefLocation = Utils::getDeclDefinitionInfo(pAsRecord);

				// Save info
				baseInfo.sDefLocation = aDefLocation;
				baseInfo.eKind        = cpp::TypeKind::TK_STRUCT_OR_CLASS;
				return true;
			}

			return false;
		}

		if (qualType->isEnumeralType() || qualType->isScopedEnumeralType())
		{
			// C/C++ enum
			if (auto* pAsEnumType = qualType->getAs<clang::EnumType>())
			{
				if (auto* pAsEnumDecl = pAsEnumType->getDecl())
				{
					CompilerConfig cc {};
					cc.bAllowCollectNonRuntimeTypes = true;

					std::vector<rg3::cpp::TypeBasePtr> vCollected {};

					visitors::CxxTypeVisitor visitor { vCollected, cc };
					visitor.TraverseDecl(pAsEnumDecl);

					if (!vCollected.empty() && vCollected[0]->getKind() == cpp::TypeKind::TK_ENUM && !vCollected[0]->getPrettyName().empty())
					{
						Utils::getNamePrettyNameAndNamespaceForNamedDecl(pAsEnumDecl, baseInfo.sName, baseInfo.sPrettyName, baseInfo.sNameSpace);

						baseInfo.eKind = cpp::TypeKind::TK_ENUM;
//						baseInfo.sName = vCollected[0]->getName();
//						baseInfo.sNameSpace = vCollected[0]->getNamespace();
//						baseInfo.sPrettyName = vCollected[0]->getPrettyName();
						baseInfo.sDefLocation = vCollected[0]->getDefinition();

						return true;
					}
				}
			}

			return false;
		}

		if (qualType->isBuiltinType())
		{
			// Builtin type (int, float, etc...). Namespace not supported here
			if (auto* pAsBuiltinType = qualType->getAs<clang::BuiltinType>())
			{
				// It's builtin, just register a type
				// Our builtins are store as generic rg3::cpp::TypeBase instances
				clang::PrintingPolicy typeNamePrintingPolicy { astContext.getLangOpts() };
				typeNamePrintingPolicy.SuppressTagKeyword = true;
				typeNamePrintingPolicy.SuppressScope = false;
				typeNamePrintingPolicy.Bool = true;

				baseInfo.eKind = cpp::TypeKind::TK_TRIVIAL;
				baseInfo.sName = baseInfo.sPrettyName = pAsBuiltinType->getNameAsCString(typeNamePrintingPolicy);
				baseInfo.sNameSpace = {};
				baseInfo.sDefLocation = {};
				return true;
			}
		}

		// Not supported yet
		return false;
	}

	void Utils::fillTypeStatementFromQualType(rg3::cpp::TypeStatement& typeStatement, clang::QualType qt, const clang::ASTContext& astContext)
	{
		const clang::SourceManager& sm = astContext.getSourceManager();

		{
			cpp::TypeBaseInfo typeBaseInfo {};
			if (!getQualTypeBaseInfo(qt, typeBaseInfo, astContext))
			{
				// Use "pure" view
				typeStatement.sTypeRef = cpp::TypeReference(qt.getUnqualifiedType().getAsString());
				typeStatement.sBaseInfo = {}; // invalid in this case
			}
			else
			{
				// Use correct view
				typeStatement.sTypeRef = cpp::TypeReference(typeBaseInfo.sPrettyName);
				typeStatement.sBaseInfo = typeBaseInfo;
			}
		}

		typeStatement.bIsConst = qt.isConstQualified();

		if (qt->isPointerType() || qt->isReferenceType())
		{
			cpp::TypeBaseInfo typeBaseInfo {};
			if (!getQualTypeBaseInfo(qt->getPointeeType().getUnqualifiedType(), typeBaseInfo, astContext))
			{
				// Use "pure" view
				typeStatement.sTypeRef = cpp::TypeReference(qt->getPointeeType().getUnqualifiedType().getAsString());
				typeStatement.sBaseInfo = {}; // override to invalid
			}
			else
			{
				// Use correct view
				typeStatement.sTypeRef = cpp::TypeReference(typeBaseInfo.sPrettyName);
				typeStatement.sBaseInfo = typeBaseInfo; // override
			}

			typeStatement.bIsPointer = qt->isPointerType();
			typeStatement.bIsReference = qt->isReferenceType();
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

	void collectDeclNameUntilNamespace(const clang::NamedDecl* pDecl, std::vector<std::string>& parts)
	{
		if (!pDecl || ::llvm::isa<clang::NamespaceDecl>(pDecl))
		{
			return;
		}

		if (const clang::ClassTemplateSpecializationDecl* pAsTemplDecl = ::llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(pDecl))
		{
			// Ok, it's harder
			std::string sFinalName {};
			::llvm::raw_string_ostream ss { sFinalName };
			clang::PrintingPolicy pp { pDecl->getASTContext().getLangOpts() };

			pAsTemplDecl->getNameForDiagnostic(ss, pp, false);
			parts.insert(parts.begin(), sFinalName);
		}
		else
		{
			parts.insert(parts.begin(), pDecl->getNameAsString());
		}

		collectDeclNameUntilNamespace(::llvm::dyn_cast<clang::NamedDecl>(clang::NamedDecl::castFromDeclContext(pDecl->getLexicalDeclContext())), parts);
	}

	void Utils::getNamePrettyNameAndNamespaceForNamedDecl(const clang::NamedDecl* pDecl, std::string& sName, std::string& sPrettyName, cpp::CppNamespace& sNameSpace)
	{
		// Find a name
		std::vector<std::string> parts {};

		parts.reserve(3); // for most cases that's enough
		collectDeclNameUntilNamespace(pDecl, parts);

		// Join them
		sName = boost::algorithm::join(parts, "::");

		// Find namespace
		getDeclInfo(pDecl, sNameSpace);
		
		// Fill pretty name
		sPrettyName = sNameSpace.isEmpty() ? sName : fmt::format("{}::{}", sNameSpace.asString(), sName);
	}
}