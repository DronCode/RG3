#include <RG3/LLVM/CodeAnalyzer.h>
#include <RG3/LLVM/Actions/ExtractTypesFromTU.h>

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Comment.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Basic/TargetInfo.h>

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Support/MemoryBufferRef.h>
#include <llvm/Support/Host.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/TargetParser/Triple.h>

#include <RG3/LLVM/CompilerConfigDetector.h>

#include <algorithm>
#include <utility>


namespace rg3::llvm
{
	struct Visitor
	{
		clang::FrontendOptions& compilerOptions;

		void operator()(const std::filesystem::path& path)
		{
			std::string absolutePath = std::filesystem::absolute(path).string();

			compilerOptions.Inputs.push_back(
				clang::FrontendInputFile(
					absolutePath,
					clang::InputKind(
						clang::Language::CXX,
						clang::InputKind::Format::Source,
						false, // NOT preprocessed
						clang::InputKind::HeaderUnitKind::HeaderUnit_User,
						true // is Header = true
						),
					false // IsSystem = false
					)
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
					false // IsSystem = false
					)
			);
		}
	};

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
		m_source.emplace<std::string>(sourceCode);
	}

	void CodeAnalyzer::setSourceFile(const std::filesystem::path& sourceFile)
	{
		m_source.emplace<std::filesystem::path>(sourceFile);
	}

	CompilerConfig& CodeAnalyzer::getCompilerConfig()
	{
		return m_compilerConfig;
	}

	std::string sourceToString(const std::variant<std::filesystem::path, std::string>& src)
	{
		struct
		{
			std::string repr;

			void operator()(const std::filesystem::path& p)
			{
				repr = p.string();
			}

			void operator()(const std::string& s)
			{
				repr = s;
			}
		} v;

		std::visit(v, src);
		return v.repr;
	};

	AnalyzerResult CodeAnalyzer::analyze()
	{
		AnalyzerResult result;

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Run platform env detector
		const auto compilerEnvironment = CompilerConfigDetector::detectSystemCompilerEnvironment();
		if (auto pEnvFailure = std::get_if<CompilerEnvError>(&compilerEnvironment))
		{
			// Fatal error
			result.vIssues.emplace_back(AnalyzerResult::CompilerIssue::IssueKind::IK_ERROR, sourceToString(m_source), pEnvFailure->message);
			return result;
		}

		const auto* pCompilerEnv = std::get_if<CompilerEnvironment>(&compilerEnvironment);

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		clang::CompilerInstance compilerInstance;
		compilerInstance.createDiagnostics();

		// Set up FileManager and SourceManager
		compilerInstance.createFileManager();
		compilerInstance.createSourceManager(compilerInstance.getFileManager());

		std::vector<std::string> vProxyArgs = m_compilerConfig.vCompilerArgs;
#ifdef _WIN32
		vProxyArgs.emplace_back("-fms-extensions");
		vProxyArgs.emplace_back("-fdelayed-template-parsing");
		vProxyArgs.emplace_back("-fms-compatibility-version=19");
#endif

		std::vector<const char*> vCompilerArgs;
		vCompilerArgs.resize(vProxyArgs.size());

		for (size_t i = 0; i < vProxyArgs.size(); i++)
		{
			vCompilerArgs[i] = vProxyArgs[i].c_str();
		}

		// Set up CompilerInvocation
		auto invocation = std::make_shared<clang::CompilerInvocation>();
		clang::CompilerInvocation::CreateFromArgs(
			*invocation,
			::llvm::ArrayRef<const char*>(vCompilerArgs.data(), vCompilerArgs.size()),
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

		langOptions->LangStd = langKind;

#ifdef _WIN32
		compilerInstance.getPreprocessorOpts().addMacroDef("_MSC_VER=1932");
		compilerInstance.getPreprocessorOpts().addMacroDef("_MSC_FULL_VER=193231329");
		compilerInstance.getPreprocessorOpts().addMacroDef("_MSC_EXTENSIONS");
#endif

		std::shared_ptr<clang::TargetOptions> targetOpts = nullptr;

		// Setup triple
		if (!pCompilerEnv || pCompilerEnv->triple.empty())
		{
			// Use default triple
			targetOpts = std::make_shared<clang::TargetOptions>();
			targetOpts->Triple = ::llvm::sys::getDefaultTargetTriple();
			clang::TargetInfo* targetInfo = clang::TargetInfo::CreateTargetInfo(compilerInstance.getDiagnostics(), targetOpts);
			compilerInstance.setTarget(targetInfo);

			auto triple = targetInfo->getTriple();

			std::vector<std::string> vIncs;
			clang::LangOptions::setLangDefaults(*langOptions, clang::Language::CXX, triple, vIncs, langKind);
		}
		else
		{
			targetOpts = std::make_shared<clang::TargetOptions>();
			targetOpts->Triple = ::llvm::Triple::normalize(pCompilerEnv->triple);
			clang::TargetInfo* targetInfo = clang::TargetInfo::CreateTargetInfo(compilerInstance.getDiagnostics(), targetOpts);
			compilerInstance.setTarget(targetInfo);

			auto triple = targetInfo->getTriple();

			std::vector<std::string> vIncs;
			clang::LangOptions::setLangDefaults(*langOptions, clang::Language::CXX, triple, vIncs, langKind);
		}

		compilerInstance.setInvocation(invocation);

		// Set up FrontendOptions
		clang::FrontendOptions &opts = compilerInstance.getFrontendOpts();
		opts.ProgramAction = clang::frontend::ParseSyntaxOnly;

		opts.Inputs.clear();

		// Prepare compiler instance
		std::visit(Visitor(opts), m_source);

		// Set macros
		clang::PreprocessorOptions& preprocessorOptions = compilerInstance.getPreprocessorOpts();
		for (const auto& compilerDef : m_compilerConfig.vCompilerDefs)
		{
			preprocessorOptions.addMacroDef(compilerDef);
		}

		// Setup header dirs source
		clang::HeaderSearchOptions& headerSearchOptions = compilerInstance.getHeaderSearchOpts();
		{
			for (const auto& sysInc : (pCompilerEnv ? pCompilerEnv->config.vSystemIncludes : m_compilerConfig.vSystemIncludes))
			{
				const auto absolutePath = std::filesystem::absolute(sysInc.sFsLocation);
				headerSearchOptions.AddPath(absolutePath.string(), clang::frontend::IncludeDirGroup::CXXSystem, false, false);
			}

			for (const auto& incInfo : m_compilerConfig.vIncludes)
			{
				// Convert path to absolute
				const auto absolutePath = std::filesystem::absolute(incInfo.sFsLocation);
				headerSearchOptions.AddPath(absolutePath.string(), clang::frontend::IncludeDirGroup::Angled, false, false);
			}
		}

		// Run actions
		{
			rg3::llvm::actions::ExtractTypesFromTUAction findTypesAction { result.vFoundTypes, m_compilerConfig };
			if (!compilerInstance.ExecuteAction(findTypesAction))
			{
//				auto& diagEngine = compilerInstance.getDiagnostics();
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