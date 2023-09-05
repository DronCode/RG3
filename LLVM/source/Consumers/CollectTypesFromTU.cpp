#include <RG3/LLVM/Consumers/CollectTypesFromTU.h>
#include <RG3/LLVM/Visitors/CxxTypeVisitor.h>


namespace rg3::llvm::consumers
{
	CollectTypesFromTUConsumer::CollectTypesFromTUConsumer(std::vector<rg3::cpp::TypeBasePtr>& vCollectedTypes)
		: clang::ASTConsumer(), collectedTypes(vCollectedTypes)
	{
	}

	// Should return true when we need to continue iteration over declarations and ignore current decl
	bool shouldIgnoreClangDecl(const clang::Decl* decl)
	{
		using DeclKind = clang::Decl::Kind;
		return decl->getKind() == DeclKind::NamespaceAlias || decl->getKind() == DeclKind::Namespace;
	}

	void CollectTypesFromTUConsumer::HandleTranslationUnit(clang::ASTContext& ctx)
	{
		rg3::llvm::visitors::CxxTypeVisitor visitor { collectedTypes };
		visitor.TraverseDecl(ctx.getTranslationUnitDecl());
	}
}