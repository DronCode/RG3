#include <RG3/LLVM/CodeAnalyzer.h>
#include <RG3/LLVM/Actions/ExtractTypesFromTU.h>

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Comment.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Basic/LangOptions.h>

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Support/MemoryBufferRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/TargetParser/Triple.h>

#include <algorithm>
#include <utility>


namespace rg3::llvm
{
	AnalyzerResult::operator bool() const
	{
		return std::count_if(
			vIssues.begin(),
			vIssues.end(),
			[](const CompilerIssue& issue) -> bool {
				return 	issue.kind != CompilerIssue::IssueKind::IK_INFO &&
						issue.kind != CompilerIssue::IssueKind::IK_NONE;
			}) == 0;
	}

	CodeAnalyzer::CodeAnalyzer() = default;

	CodeAnalyzer::CodeAnalyzer(const std::filesystem::path& sourceFile, CompilerConfig compilerConfig)
		: m_source(sourceFile)
		, m_compilerConfig(std::move(compilerConfig))
	{
	}

	CodeAnalyzer::CodeAnalyzer(std::string sourceCodeBuffer, CompilerConfig compilerConfig)
		: m_source(sourceCodeBuffer)
		, m_compilerConfig(std::move(compilerConfig))
	{
	}

	void CodeAnalyzer::setSourceCode(const std::string& sourceCode)
	{
		m_source = sourceCode;
	}

	void CodeAnalyzer::setSourceFile(const std::filesystem::path& sourceFile)
	{
		m_source = sourceFile;
	}

	CompilerConfig& CodeAnalyzer::getCompilerConfig()
	{
		return m_compilerConfig;
	}


	AnalyzerResult CodeAnalyzer::analyze()
	{
		AnalyzerResult result;
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		clang::CompilerInstance compilerInstance;
		compilerInstance.createDiagnostics();

		std::vector<const char*> vCompilerArgs;
		vCompilerArgs.resize(m_compilerConfig.vCompilerArgs.size());

		for (size_t i = 0; i < m_compilerConfig.vCompilerArgs.size(); i++)
		{
			vCompilerArgs[i] = m_compilerConfig.vCompilerArgs[i].c_str();
		}

		// Set up CompilerInvocation
		auto invocation = std::make_shared<clang::CompilerInvocation>();
		clang::CompilerInvocation::CreateFromArgs(
			*invocation,
			::llvm::ArrayRef<const char*>(vCompilerArgs.data(), vCompilerArgs.data() + vCompilerArgs.size()),
			compilerInstance.getDiagnostics()
		);

		// Use C++20
		clang::LangStandard::Kind langKind;
		auto* langOptions = invocation->getLangOpts();
		langOptions->CPlusPlus = 1;

		switch (m_compilerConfig.cppStandard)
		{
			case CxxStandard::CC_11:
				langOptions->CPlusPlus11 = 1;
				langKind = clang::LangStandard::Kind::lang_cxx11;
				break;
			case CxxStandard::CC_14:
				langOptions->CPlusPlus14 = 1;
				langKind = clang::LangStandard::Kind::lang_cxx14;
				break;
			case CxxStandard::CC_17:
				langOptions->CPlusPlus17 = 1;
				langKind = clang::LangStandard::Kind::lang_cxx17;
				break;
			case CxxStandard::CC_20:
				langOptions->CPlusPlus20 = 1;
				langOptions->CPlusPlus2b = 1;
				langKind = clang::LangStandard::Kind::lang_cxx20;
				break;
			default:
				langOptions->CPlusPlus11 = 1;
				langKind = clang::LangStandard::Kind::lang_cxx11;
				break;
		}

		{
			std::vector<std::string> vIncs;

			::llvm::Triple triple;
			triple.setArch(::llvm::Triple::ArchType::x86_64, ::llvm::Triple::SubArchType::NoSubArch);
			triple.setEnvironment(::llvm::Triple::EnvironmentType::MSVC);
			triple.setOS(::llvm::Triple::OSType::Win32);
			triple.setVendor(::llvm::Triple::VendorType::UnknownVendor);
			triple.setObjectFormat(::llvm::Triple::ObjectFormatType::COFF);

			clang::LangOptions::setLangDefaults(*langOptions, clang::Language::CXX, triple, vIncs, langKind);
		}

		compilerInstance.setInvocation(invocation);

		// Set up FileManager and SourceManager
		compilerInstance.createFileManager();
		compilerInstance.createSourceManager(compilerInstance.getFileManager());

		// Set up FrontendOptions
		clang::FrontendOptions &opts = compilerInstance.getFrontendOpts();
		opts.ProgramAction = clang::frontend::ParseSyntaxOnly;

		opts.Inputs.clear();

		struct Visitor
		{
			clang::FrontendOptions& compilerOptions;

			void operator()(const std::filesystem::path& path)
			{
				clang::FrontendInputFile(
					::llvm::StringRef(path.string()),
					clang::InputKind(
						clang::Language::CXX,
						clang::InputKind::Format::Source,
						false, // NOT preprocessed
						clang::InputKind::HeaderUnitKind::HeaderUnit_User,
						true // is Header = true
					),
					false
				);
			}

			void operator()(const std::string& buffer)
			{
				compilerOptions.Inputs.push_back(
					clang::FrontendInputFile(
						::llvm::MemoryBufferRef(
							::llvm::StringRef(buffer.data()),
							::llvm::StringRef("id0.hpp")
						),
						clang::InputKind(
							clang::Language::CXX,
							clang::InputKind::Format::Source,
							false, // NOT preprocessed
							clang::InputKind::HeaderUnitKind::HeaderUnit_User,
							true // is Header = true
						),
						false
					)
				);
			}
		};

		// Prepare compiler instance
		std::visit(Visitor(opts), m_source);

		// Run actions
		{
			rg3::llvm::actions::ExtractTypesFromTUAction findTypesAction { result.vFoundTypes, m_compilerConfig };
			if (!compilerInstance.ExecuteAction(findTypesAction))
			{
				result.vIssues.push_back(
					AnalyzerResult::CompilerIssue(
						AnalyzerResult::CompilerIssue::IssueKind::IK_ERROR,
						{},
						"Failed to run FindTypesAction<> on compiler instance!"
					)
				);
			}
			else
			{
				// TODO: Use findTypesAction.foundTypes
			}
		}

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		return result;
	}
}