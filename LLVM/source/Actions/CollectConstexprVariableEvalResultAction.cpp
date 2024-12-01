#include <RG3/LLVM/Actions/CollectConstexprVariableEvalResultAction.h>
#include <RG3/LLVM/Consumers/CollectConstexprVariableEvalResult.h>


namespace rg3::llvm::actions
{
	std::unique_ptr<clang::ASTConsumer> CollectConstexprVariableEvalResultAction::CreateASTConsumer(clang::CompilerInstance&, clang::StringRef)
	{
		auto pConsumer = std::make_unique<consumers::CollectConstexprVariableEvalResult>();
		pConsumer->aExpectedVariables = aExpectedVariables;
		pConsumer->pEvaluatedVariables = pEvaluatedVariables;

		return std::move(pConsumer);
	}
}