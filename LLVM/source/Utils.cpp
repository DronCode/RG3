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
			// Unknown path, maybe abstract thing or smth else
			locationPath = {};
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
}