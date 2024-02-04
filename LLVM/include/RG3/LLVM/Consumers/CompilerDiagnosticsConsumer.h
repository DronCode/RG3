#pragma once

#include <RG3/LLVM/CodeAnalyzer.h>
#include <clang/Basic/Diagnostic.h>

namespace rg3::llvm::consumers
{
	class CompilerDiagnosticsConsumer : public clang::DiagnosticConsumer
	{
	 public:
		explicit CompilerDiagnosticsConsumer(rg3::llvm::AnalyzerResult& analyzerResult);

		void HandleDiagnostic(clang::DiagnosticsEngine::Level DiagLevel, const clang::Diagnostic& Info) override;

	 private:
		rg3::llvm::AnalyzerResult& m_analyzerResult;
	};
}