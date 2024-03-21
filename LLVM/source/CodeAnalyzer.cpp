#include <RG3/LLVM/CodeAnalyzer.h>
#include <RG3/LLVM/Actions/ExtractTypesFromTU.h>
#include <RG3/LLVM/Consumers/CompilerDiagnosticsConsumer.h>

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
#include <llvm/TargetParser/Host.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/TargetParser/Triple.h>

#include <RG3/LLVM/CompilerConfigDetector.h>

#include <RG3_Config.h> /// Auto-generated by CMake

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

	void CodeAnalyzer::setCompilerEnvironment(const CompilerEnvironment& env)
	{
		m_env = env;
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
		const CompilerEnvironment* pCompilerEnv = nullptr;
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Run platform env detector
		if (!m_env.has_value())
		{
			const auto compilerEnvironment = CompilerConfigDetector::detectSystemCompilerEnvironment();
			if (auto pEnvFailure = std::get_if<CompilerEnvError>(&compilerEnvironment))
			{
				// Fatal error
				result.vIssues.emplace_back(AnalyzerResult::CompilerIssue { AnalyzerResult::CompilerIssue::IssueKind::IK_ERROR, sourceToString(m_source), 0, 0, pEnvFailure->message });
				return result;
			}

			// Override env
			m_env = *std::get_if<CompilerEnvironment>(&compilerEnvironment);
		}

		pCompilerEnv = &m_env.value();

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		clang::CompilerInstance compilerInstance;
		compilerInstance.createDiagnostics();

		// Create custom diagnostics consumer and pass it into CompilerInstance
		{
			auto errorCollector = std::make_unique<consumers::CompilerDiagnosticsConsumer>(result);
			compilerInstance.getDiagnostics().setClient(errorCollector.release(), false);
		}

		// Set up FileManager and SourceManager
		compilerInstance.createFileManager();
		compilerInstance.createSourceManager(compilerInstance.getFileManager());

		std::vector<std::string> vProxyArgs = m_compilerConfig.vCompilerArgs;
#ifdef _WIN32
		vProxyArgs.emplace_back("-fms-extensions");
		vProxyArgs.emplace_back("-fdelayed-template-parsing");
		vProxyArgs.emplace_back("-fms-compatibility-version=19");
#endif

		if (auto it = std::find(vProxyArgs.begin(), vProxyArgs.end(), "-x"); it != vProxyArgs.end())
		{
			// need to remove this iter and next
			auto next = std::next(it);

			if (next != std::end(vProxyArgs))
			{
				// remove next
				vProxyArgs.erase(next);
			}

			// and remove this
			vProxyArgs.erase(it);
		}

		// We will append "-x c++-header" option always
		vProxyArgs.emplace_back("-x");
		vProxyArgs.emplace_back("c++-header");

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
				langKind = clang::LangStandard::Kind::lang_cxx20;
				break;
#if LLVM_VERSION_MAJOR >= 17
			case CxxStandard::CC_23:
				langOptions->CPlusPlus23 = 1;
				langKind = clang::LangStandard::Kind::lang_cxx23;
				break;
			case CxxStandard::CC_26:
				langOptions->CPlusPlus26 = 1;
				langKind = clang::LangStandard::Kind::lang_cxx26;
				break;
#else
			case CxxStandard::CC_23:
			case CxxStandard::CC_26:
				assert(false && "Unsupported Cxx standard! Used C++20 instead!");
				langOptions->CPlusPlus20 = 1;
				langKind = clang::LangStandard::Kind::lang_cxx20;
				break;
#endif
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

#ifdef __APPLE__
		// This should be enough?
		compilerInstance.getPreprocessorOpts().addMacroDef("__GCC_HAVE_DWARF2_CFI_ASM=1");
#endif

		std::shared_ptr<clang::TargetOptions> targetOpts = nullptr;

		// Setup triple
#if !defined(__APPLE__)
		if (pCompilerEnv->triple.empty())
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
#else
		// On Apple we should use default triple instead of detect it at runtime
		::llvm::Triple triple(::llvm::sys::getDefaultTargetTriple());
		compilerInstance.getTargetOpts().Triple = triple.str();
		compilerInstance.setTarget(clang::TargetInfo::CreateTargetInfo(compilerInstance.getDiagnostics(), std::make_shared<clang::TargetOptions>(compilerInstance.getTargetOpts())));
#endif

		compilerInstance.setInvocation(invocation);

		// Set up FrontendOptions
		clang::FrontendOptions &opts = compilerInstance.getFrontendOpts();
		opts.ProgramAction = clang::frontend::ParseSyntaxOnly;

		opts.Inputs.clear();

		// Prepare compiler instance
		{
			Visitor v { opts };
			std::visit(v, m_source);
		}

		// Set macros
		clang::PreprocessorOptions& preprocessorOptions = compilerInstance.getPreprocessorOpts();
		for (const auto& compilerDef : m_compilerConfig.vCompilerDefs)
		{
			preprocessorOptions.addMacroDef(compilerDef);
		}

		// Add builtins
		preprocessorOptions.addMacroDef("__RG3__=1");
		preprocessorOptions.addMacroDef("__RG3_COMMIT__=\"" RG3_BUILD_HASH "\"");
		preprocessorOptions.addMacroDef("__RG3_BUILD_DATE__=\"" __DATE__ "\"");

#ifdef __APPLE__
		// For apple only. They cares about GNUC? Idk & I don't care
		preprocessorOptions.addMacroDef("__GNUC__=4");
#endif

		// Setup header dirs source
		clang::HeaderSearchOptions& headerSearchOptions = compilerInstance.getHeaderSearchOpts();
		{
			for (const auto& sysInc : pCompilerEnv->config.vSystemIncludes)
			{
				const auto absolutePath = std::filesystem::absolute(sysInc.sFsLocation);

				clang::frontend::IncludeDirGroup group = clang::frontend::IncludeDirGroup::Angled;

				if (sysInc.eKind == IncludeKind::IK_SYSROOT)
				{
					// ignore sysroot here
					continue;
				}

				if (sysInc.eKind == IncludeKind::IK_SYSTEM)
				{
					group = clang::frontend::IncludeDirGroup::System;
				}

				if (sysInc.eKind == IncludeKind::IK_C_SYSTEM)
				{
					group = clang::frontend::IncludeDirGroup::ExternCSystem;
				}

				headerSearchOptions.AddPath(absolutePath.string(), group, false, true);
			}

			for (const auto& incInfo : m_compilerConfig.vIncludes)
			{
				// Convert path to absolute
				const auto absolutePath = std::filesystem::absolute(incInfo.sFsLocation);
				headerSearchOptions.AddPath(absolutePath.string(), clang::frontend::IncludeDirGroup::Angled, false, true);
			}
		}

		// Run actions
		{
			rg3::llvm::actions::ExtractTypesFromTUAction findTypesAction { result.vFoundTypes, m_compilerConfig };
			compilerInstance.ExecuteAction(findTypesAction);
		}

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		return result;
	}
}