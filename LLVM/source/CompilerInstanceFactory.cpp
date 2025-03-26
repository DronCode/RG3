#include <RG3/LLVM/CompilerInstanceFactory.h>
#include <RG3_Config.h> /// Auto-generated by CMake

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Comment.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Preprocessor.h>
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


namespace rg3::llvm
{
	struct Visitor
	{
		clang::FrontendOptions& compilerOptions;
		clang::CompilerInstance* pCompilerInstance { nullptr };

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
			std::string sanitizedBuffer;
			std::remove_copy_if(
				buffer.begin(), buffer.end(),
				std::back_inserter(sanitizedBuffer),
				[](char c) { return c == '\0'; }
			);

			auto pMemBuffer = ::llvm::MemoryBuffer::getMemBufferCopy(sanitizedBuffer, "id0.hpp");
			pCompilerInstance->getPreprocessorOpts().addRemappedFile("id0.hpp", pMemBuffer.release());

			compilerOptions.Inputs.push_back(clang::FrontendInputFile("id0.hpp", clang::Language::CXX));
		}
	};

	void CompilerInstanceFactory::makeInstance(clang::CompilerInstance* pOutInstance,
											   const std::variant<std::filesystem::path, std::string>& sInput,
											   const rg3::llvm::CompilerConfig& sCompilerConfig,
											   const rg3::llvm::CompilerEnvironment* pCompilerEnv)
	{
		pOutInstance->createDiagnostics();

		// Set up FileManager and SourceManager
		pOutInstance->createFileManager();
		pOutInstance->createSourceManager(pOutInstance->getFileManager());

		std::vector<std::string> vProxyArgs = sCompilerConfig.vCompilerArgs;
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
			pOutInstance->getDiagnostics()
		);

		// Use C++20
		clang::LangStandard::Kind langKind;
		auto& langOptions = invocation->getLangOpts();

		langOptions.CPlusPlus = 1;

		switch (sCompilerConfig.cppStandard)
		{
		case CxxStandard::CC_11:
			langOptions.CPlusPlus11 = 1;
			langKind = clang::LangStandard::Kind::lang_cxx11;
			break;
		case CxxStandard::CC_14:
			langOptions.CPlusPlus14 = 1;
			langKind = clang::LangStandard::Kind::lang_cxx14;
			break;
		case CxxStandard::CC_17:
			langOptions.CPlusPlus17 = 1;
			langKind = clang::LangStandard::Kind::lang_cxx17;
			break;
		case CxxStandard::CC_20:
			langOptions.CPlusPlus20 = 1;
			langKind = clang::LangStandard::Kind::lang_cxx20;
			break;
		case CxxStandard::CC_23:
			langOptions.CPlusPlus23 = 1;
			langKind = clang::LangStandard::Kind::lang_cxx23;
			break;
		case CxxStandard::CC_26:
			langOptions.CPlusPlus26 = 1;
			langKind = clang::LangStandard::Kind::lang_cxx26;
			break;
		default:
			langOptions.CPlusPlus11 = 1;
			langKind = clang::LangStandard::Kind::lang_cxx11;
			break;
		}

		langOptions.LangStd = langKind;
		langOptions.IsHeaderFile = true; // NOTE: Maybe we should use flag here?

#ifdef _WIN32
		pOutInstance->getPreprocessorOpts().addMacroDef("_MSC_VER=1932");
		pOutInstance->getPreprocessorOpts().addMacroDef("_MSC_FULL_VER=193231329");
		pOutInstance->getPreprocessorOpts().addMacroDef("_MSC_EXTENSIONS");

		/**
		 * Workaround: it's workaround for MSVC 2022 with yvals_core.h which send static assert when Clang version less than 17.x.x
		 * Example : static assertion failed: error STL1000: Unexpected compiler version, expected Clang 17.0.0 or newer. at C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Tools\MSVC\14.40.33807\include\yvals_core.h:898
		 *
		 * This macro block static assertion and should help us, but this part of code MUST be removed after RG3 migrates to latest LLVM & Clang
		 */
		pOutInstance->getPreprocessorOpts().addMacroDef("_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH=1");
#endif

#ifdef __APPLE__
		// This should be enough?
		pOutInstance->getPreprocessorOpts().addMacroDef("__GCC_HAVE_DWARF2_CFI_ASM=1");
#endif

		std::shared_ptr<clang::TargetOptions> targetOpts = nullptr;

		// Setup triple
#if !defined(__APPLE__)
		if (pCompilerEnv->triple.empty())
		{
			// Use default triple
			targetOpts = std::make_shared<clang::TargetOptions>();
			targetOpts->Triple = ::llvm::sys::getDefaultTargetTriple();
			clang::TargetInfo* targetInfo = clang::TargetInfo::CreateTargetInfo(pOutInstance->getDiagnostics(), targetOpts);
			pOutInstance->setTarget(targetInfo);

			auto triple = targetInfo->getTriple();

			std::vector<std::string> vIncs;
			clang::LangOptions::setLangDefaults(langOptions, clang::Language::CXX, triple, vIncs, langKind);
		}
		else
		{
			targetOpts = std::make_shared<clang::TargetOptions>();
			targetOpts->Triple = ::llvm::Triple::normalize(pCompilerEnv->triple);
			clang::TargetInfo* targetInfo = clang::TargetInfo::CreateTargetInfo(pOutInstance->getDiagnostics(), targetOpts);
			pOutInstance->setTarget(targetInfo);

			auto triple = targetInfo->getTriple();

			std::vector<std::string> vIncs;
			clang::LangOptions::setLangDefaults(langOptions, clang::Language::CXX, triple, vIncs, langKind);
		}
#else
		// On Apple we should use default triple instead of detect it at runtime
		::llvm::Triple triple(::llvm::sys::getDefaultTargetTriple());
		pOutInstance->getTargetOpts().Triple = triple.str();
		pOutInstance->setTarget(clang::TargetInfo::CreateTargetInfo(pOutInstance->getDiagnostics(), std::make_shared<clang::TargetOptions>(pOutInstance->getTargetOpts())));
#endif

		pOutInstance->setInvocation(invocation);
		pOutInstance->createPreprocessor(clang::TranslationUnitKind::TU_Complete);

		// Set up FrontendOptions
		clang::FrontendOptions &opts = pOutInstance->getFrontendOpts();
		opts.ProgramAction = clang::frontend::ParseSyntaxOnly;
		opts.SkipFunctionBodies = static_cast<unsigned int>(sCompilerConfig.bSkipFunctionBodies);

		opts.Inputs.clear();

		// Prepare compiler instance
		{
			Visitor v { opts, pOutInstance };
			std::visit(v, sInput);
		}

		// Set macros
		clang::PreprocessorOptions& preprocessorOptions = pOutInstance->getPreprocessorOpts();
		for (const auto& compilerDef : sCompilerConfig.vCompilerDefs)
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
		clang::HeaderSearchOptions& headerSearchOptions = pOutInstance->getHeaderSearchOpts();
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

			for (const auto& incInfo : sCompilerConfig.vIncludes)
			{
				// Convert path to absolute
				const auto absolutePath = std::filesystem::absolute(incInfo.sFsLocation);
				headerSearchOptions.AddPath(absolutePath.string(), clang::frontend::IncludeDirGroup::Angled, false, true);
			}
		}

		// small self check
		assert(pOutInstance->hasDiagnostics() && "Diagnostics not set up!");
		assert(pOutInstance->hasTarget() && "Target not set up!");
		assert(pOutInstance->hasFileManager() && "FileManager not set up!");
		assert(pOutInstance->hasPreprocessor() && "Preprocessor not set up!");
	}
}