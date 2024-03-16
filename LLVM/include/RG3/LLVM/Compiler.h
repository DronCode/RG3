#pragma once

#include <filesystem>
#include <vector>


namespace rg3::llvm
{
	enum class CxxStandard : int { CC_11 = 11, CC_14 = 14, CC_17 = 17, CC_20 = 20, CC_23 = 23, CC_26 = 26, CC_DEFAULT = CC_11 };
	enum class IncludeKind : int { IK_PROJECT = 0, IK_SYSTEM, IK_C_SYSTEM, IK_SYSROOT, IK_THIRD_PARTY, IK_DEFAULT = IK_PROJECT };


	struct IncludeInfo
	{
		std::filesystem::path sFsLocation;
		IncludeKind eKind { IncludeKind::IK_DEFAULT };
		bool bIsMacOSFramework { false };

		IncludeInfo() = default;
		explicit IncludeInfo(const std::string& path) : sFsLocation(path) {}
		IncludeInfo(const std::string& path, IncludeKind kind, bool bOSXFramework = false) : sFsLocation(path), eKind(kind), bIsMacOSFramework(bOSXFramework) {}
	};

	using IncludeVector = std::vector<IncludeInfo>;

	struct ClangRuntimeInfo
	{
		static std::string getRuntimeInfo();
	};
}
