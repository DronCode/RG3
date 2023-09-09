#include <RG3/LLVM/Actions/ExtractTypesFromTU.h>
#include <RG3/LLVM/Consumers/CollectTypesFromTU.h>


namespace rg3::llvm::actions
{
	std::unique_ptr<clang::ASTConsumer> ExtractTypesFromTUAction::CreateASTConsumer(clang::CompilerInstance&, clang::StringRef)
	{
		return std::make_unique<consumers::CollectTypesFromTUConsumer>(foundTypes, compilerConfig);
	}
}