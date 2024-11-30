#include <RG3/LLVM/Consumers/CollectConstexprVariableEvalResult.h>

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/ASTConsumer.h>


namespace rg3::llvm::consumers
{
	class ConstexprVisitor : public clang::RecursiveASTVisitor<ConstexprVisitor> {
	 private:
		std::unordered_set<std::string> m_aExpectedVariables {};
		std::unordered_map<std::string, VariableValue>* m_pEvaluatedVariables { nullptr };
		clang::ASTContext& m_sContext;

	 public:
		explicit ConstexprVisitor(clang::ASTContext& context, std::unordered_map<std::string, VariableValue>* pEvaluatedValues, const std::unordered_set<std::string>& aExpectedVariables)
			: m_sContext(context), m_aExpectedVariables(aExpectedVariables), m_pEvaluatedVariables(pEvaluatedValues)
		{
		}

		bool VisitVarDecl(clang::VarDecl* pVarDecl)
		{
			std::string sName = pVarDecl->getNameAsString();

			if (pVarDecl->isConstexpr() && m_aExpectedVariables.contains(sName))
			{
				auto* pEvaluated = pVarDecl->getEvaluatedValue();
				if (pEvaluated)
				{
					if (pVarDecl->getType()->isBooleanType())
					{
						(*m_pEvaluatedVariables)[sName] = pEvaluated->getInt().getBoolValue();
					}
					else if (pVarDecl->getType()->isSignedIntegerType())
					{
						(*m_pEvaluatedVariables)[sName] = pEvaluated->getInt().getExtValue();
					}
					else if (pVarDecl->getType()->isUnsignedIntegerType())
					{
						(*m_pEvaluatedVariables)[sName] = pEvaluated->getInt().getZExtValue();
					}
					else if (pVarDecl->getType()->isFloatingType())
					{
						(*m_pEvaluatedVariables)[sName] = pEvaluated->getFloat().convertToFloat();
					}
					else if (pVarDecl->getType()->isDoubleType())
					{
						(*m_pEvaluatedVariables)[sName] = pEvaluated->getFloat().convertToDouble();
					}
					else if (pVarDecl->getType()->isPointerType() && pVarDecl->getType()->getPointeeType()->isCharType())
					{
						if (pEvaluated->isLValue())
						{
							if (auto pStrValue = ::llvm::dyn_cast<clang::StringLiteral>(pEvaluated->getLValueBase().get<const clang::Expr*>()))
							{
								(*m_pEvaluatedVariables)[sName] = pStrValue->getString().str();
							}
						}
					}
				}
			}
			return true; // Continue traversal
		}
	};

	CollectConstexprVariableEvalResult::CollectConstexprVariableEvalResult() = default;

	void CollectConstexprVariableEvalResult::HandleTranslationUnit(clang::ASTContext& ctx)
	{
		ConstexprVisitor visitor { ctx, pEvaluatedVariables, aExpectedVariables };
		visitor.TraverseDecl(ctx.getTranslationUnitDecl());
	}
}