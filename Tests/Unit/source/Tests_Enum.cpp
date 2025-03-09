#include <gtest/gtest.h>

#include <RG3/Cpp/TypeBase.h>
#include <RG3/Cpp/TypeEnum.h>
#include <RG3/LLVM/CodeAnalyzer.h>


class Tests_Enum : public ::testing::Test
{
 protected:
	void SetUp() override
	{
		g_Analyzer = std::make_unique<rg3::llvm::CodeAnalyzer>();
	}

	void TearDown() override
	{
		g_Analyzer = nullptr;
	}

 protected:
	std::unique_ptr<rg3::llvm::CodeAnalyzer> g_Analyzer { nullptr };
};


TEST_F(Tests_Enum, ProcessSimpleEnum)
{
	g_Analyzer->setSourceCode(R"(
namespace my::cool::ns {
	/**
	 * @runtime
	 **/
	enum class MyCoolEnum : unsigned short
	{
		MCE_FIRST_ENTRY = 0,
		MCE_ANOTHER_ENTRY = 0xFFF0u,
		MCE_REVERTED = 0x200u,
	};
}
)");

	auto& compilerConfig = g_Analyzer->getCompilerConfig();
	compilerConfig.cppStandard = rg3::llvm::CxxStandard::CC_17;

	const auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(static_cast<bool>(analyzeResult)) << "Looks like we have an issues with analyzer";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 1) << "Expected to have 1 type";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getKind(), rg3::cpp::TypeKind::TK_ENUM) << "Expected to have enum";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getName(), "MyCoolEnum") << "Bad name";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getNamespace().asString(), "my::cool::ns") << "Invalid namespace detected";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->isForwardDeclarable(), true) << "This enum must be forward declarable!";

	auto asEnum = static_cast<rg3::cpp::TypeEnum*>(analyzeResult.vFoundTypes[0].get()); // NOLINT(*-pro-type-static-cast-downcast)

	ASSERT_TRUE(asEnum->isScoped()) << "Must be scoped";
	ASSERT_EQ(asEnum->getUnderlyingType().getRefName(), "unsigned short") << "Expected 'unsigned short'";
	ASSERT_EQ(asEnum->getEntries().size(), 3) << "Expected to have 3 elements";
	ASSERT_TRUE(asEnum->containsValue(0)) << "";
	ASSERT_TRUE(asEnum->containsValue(0xFFF0u)) << "";
	ASSERT_TRUE(asEnum->containsValue(0x200u)) << "";
	ASSERT_FALSE(asEnum->containsValue(333)) << "";
	ASSERT_EQ(asEnum->getEntries()[0].sName, "MCE_FIRST_ENTRY");
	ASSERT_EQ(asEnum->getEntries()[0].iValue, 0);
	ASSERT_EQ(asEnum->getEntries()[1].sName, "MCE_ANOTHER_ENTRY");
	ASSERT_EQ(asEnum->getEntries()[1].iValue, 0xFFF0u);
	ASSERT_EQ(asEnum->getEntries()[2].sName, "MCE_REVERTED");
	ASSERT_EQ(asEnum->getEntries()[2].iValue, 0x200u);
}

TEST_F(Tests_Enum, CheckEnumThroughTypedef)
{
	g_Analyzer->setSourceCode(R"(
namespace my::cool::ns {
	enum class MyCoolEnum : unsigned short
	{
		MCE_FIRST_ENTRY = 0,
		MCE_ANOTHER_ENTRY = 0xFFF0u,
		MCE_REVERTED = 0x200u,
	};
}

namespace world
{
	/// @runtime
	using PerfectEnum = my::cool::ns::MyCoolEnum;
}
)");

	auto& compilerConfig = g_Analyzer->getCompilerConfig();
	compilerConfig.cppStandard = rg3::llvm::CxxStandard::CC_17;

	const auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(static_cast<bool>(analyzeResult)) << "Looks like we have an issues with analyzer";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 1) << "Expected to have 1 type";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getKind(), rg3::cpp::TypeKind::TK_ENUM) << "Expected to have enum";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getName(), "PerfectEnum") << "Bad name";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getPrettyName(), "world::PerfectEnum") << "Bad pretty name";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getNamespace().asString(), "world") << "Invalid namespace detected";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->isForwardDeclarable(), false) << "This enum produced from alias and must be non forward declarable!";

	auto asEnum = static_cast<rg3::cpp::TypeEnum*>(analyzeResult.vFoundTypes[0].get()); // NOLINT(*-pro-type-static-cast-downcast)

	ASSERT_TRUE(asEnum->isScoped()) << "Must be scoped";
	ASSERT_EQ(asEnum->getUnderlyingType().getRefName(), "unsigned short") << "Expected 'unsigned short'";
	ASSERT_EQ(asEnum->getEntries().size(), 3) << "Expected to have 3 elements";
	ASSERT_TRUE(asEnum->containsValue(0)) << "";
	ASSERT_TRUE(asEnum->containsValue(0xFFF0u)) << "";
	ASSERT_TRUE(asEnum->containsValue(0x200u)) << "";
	ASSERT_FALSE(asEnum->containsValue(333)) << "";
	ASSERT_EQ(asEnum->getEntries()[0].sName, "MCE_FIRST_ENTRY");
	ASSERT_EQ(asEnum->getEntries()[0].iValue, 0);
	ASSERT_EQ(asEnum->getEntries()[1].sName, "MCE_ANOTHER_ENTRY");
	ASSERT_EQ(asEnum->getEntries()[1].iValue, 0xFFF0u);
	ASSERT_EQ(asEnum->getEntries()[2].sName, "MCE_REVERTED");
	ASSERT_EQ(asEnum->getEntries()[2].iValue, 0x200u);
}

TEST_F(Tests_Enum, CheckMultipleEnumsAtOnce)
{
	g_Analyzer->setSourceCode(R"(
namespace engine::render
{
	/**
	 * @runtime
	 * @serializer
	 */
	enum class EBlendOperand
	{
		NO_OPERAND = 0,
		BLEND_OP_ZERO,
		BLEND_OP_ONE,
		BLEND_OP_SRC_COLOR,
		BLEND_OP_ONE_MINUS_SRC_COLOR,
		BLEND_OP_DEST_COLOR,
		BLEND_OP_ONE_MINUS_DEST_COLOR,
		BLEND_OP_SRC_ALPHA,
		BLEND_OP_ONE_MINUS_SRC_ALPHA,
		BLEND_OP_CONST_COLOR,
		BLEND_OP_ONE_MINUS_CONST_COLOR,
		BLEND_OP_CONST_ALPHA,
		BLEND_OP_ONE_MINUS_CONST_ALPHA
	};

	/**
	 * @runtime
	 * @serializer
	 */
	enum class EBlendFunction
	{
		NO_FUNCTION = 0,
		BLEND_ADD,
		BLEND_SUBTRACT,
		BLEND_REVERSE_SUBTRACT,
		BLEND_MIN,
		BLEND_MAX
	};
}
)");

	auto& compilerConfig = g_Analyzer->getCompilerConfig();
	compilerConfig.cppStandard = rg3::llvm::CxxStandard::CC_20;

	const auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(static_cast<bool>(analyzeResult)) << "Looks like we have an issues with analyzer";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 2) << "Expected to have 2 type2";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getKind(), rg3::cpp::TypeKind::TK_ENUM) << "Expected to have enum";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getName(), "EBlendOperand") << "Bad name";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getPrettyName(), "engine::render::EBlendOperand") << "Bad pretty name";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getNamespace().asString(), "engine::render") << "Invalid namespace detected";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->isForwardDeclarable(), true) << "This enum must be forward declarable!";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getTags().hasTag("runtime"), true);
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getTags().hasTag("serializer"), true);

	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getKind(), rg3::cpp::TypeKind::TK_ENUM) << "Expected to have enum";
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getName(), "EBlendFunction") << "Bad name";
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getPrettyName(), "engine::render::EBlendFunction") << "Bad pretty name";
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getNamespace().asString(), "engine::render") << "Invalid namespace detected";
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->isForwardDeclarable(), true) << "This enum must be forward declarable!";
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getTags().hasTag("runtime"), true);
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getTags().hasTag("serializer"), true);

	auto asEnum0 = static_cast<rg3::cpp::TypeEnum*>(analyzeResult.vFoundTypes[0].get()); // NOLINT(*-pro-type-static-cast-downcast)
	auto asEnum1 = static_cast<rg3::cpp::TypeEnum*>(analyzeResult.vFoundTypes[1].get()); // NOLINT(*-pro-type-static-cast-downcast)

	// enum #0 - EBlendOperand
	ASSERT_EQ(asEnum0->getEntries().size(), 13);
	ASSERT_EQ(asEnum0->getEntries()[ 0].sName, "NO_OPERAND");
	ASSERT_EQ(asEnum0->getEntries()[ 1].sName, "BLEND_OP_ZERO");
	ASSERT_EQ(asEnum0->getEntries()[ 2].sName, "BLEND_OP_ONE");
	ASSERT_EQ(asEnum0->getEntries()[ 3].sName, "BLEND_OP_SRC_COLOR");
	ASSERT_EQ(asEnum0->getEntries()[ 4].sName, "BLEND_OP_ONE_MINUS_SRC_COLOR");
	ASSERT_EQ(asEnum0->getEntries()[ 5].sName, "BLEND_OP_DEST_COLOR");
	ASSERT_EQ(asEnum0->getEntries()[ 6].sName, "BLEND_OP_ONE_MINUS_DEST_COLOR");
	ASSERT_EQ(asEnum0->getEntries()[ 7].sName, "BLEND_OP_SRC_ALPHA");
	ASSERT_EQ(asEnum0->getEntries()[ 8].sName, "BLEND_OP_ONE_MINUS_SRC_ALPHA");
	ASSERT_EQ(asEnum0->getEntries()[ 9].sName, "BLEND_OP_CONST_COLOR");
	ASSERT_EQ(asEnum0->getEntries()[10].sName, "BLEND_OP_ONE_MINUS_CONST_COLOR");
	ASSERT_EQ(asEnum0->getEntries()[11].sName, "BLEND_OP_CONST_ALPHA");
	ASSERT_EQ(asEnum0->getEntries()[12].sName, "BLEND_OP_ONE_MINUS_CONST_ALPHA");

	// enum #1 - EBlendFunction
	ASSERT_EQ(asEnum1->getEntries().size(), 6);
	ASSERT_EQ(asEnum1->getEntries()[ 0].sName, "NO_FUNCTION");
	ASSERT_EQ(asEnum1->getEntries()[ 1].sName, "BLEND_ADD");
	ASSERT_EQ(asEnum1->getEntries()[ 2].sName, "BLEND_SUBTRACT");
	ASSERT_EQ(asEnum1->getEntries()[ 3].sName, "BLEND_REVERSE_SUBTRACT");
	ASSERT_EQ(asEnum1->getEntries()[ 4].sName, "BLEND_MIN");
	ASSERT_EQ(asEnum1->getEntries()[ 5].sName, "BLEND_MAX");
}

TEST_F(Tests_Enum, CheckEnumValueListAffectFromOtherIncludes)
{
	g_Analyzer->setSourceCode(R"(
namespace engine::render
{
	/**
	 * @runtime
	 */
	enum class ESample
	{
		ES_FIRST = 1,
		ES_SECOND = 2
	};
}

enum ESomeLegacy
{
	__LDO_NO_INIT_OR_SMTH_I_DONT_KNOW__ = 0x1337,
	_SOME_WEIRD_ENTRY = 0x201,
	LOL_IT_MUST_BE_SOMEWHERE_ELSE = 0x500
};

namespace core
{
	/**
	 * @runtime
	 **/
	enum class EProfileType
	{
		PT_ADMIN,
		PT_USER
	};

	enum EProfilePhotoType { PPT_JPEG, PPT_PNG, PPT_NO_PHOTO };
	enum class ESomeElse { SE_1, SE_2 };
}
)");

	auto& compilerConfig = g_Analyzer->getCompilerConfig();
	compilerConfig.cppStandard = rg3::llvm::CxxStandard::CC_20;

	const auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(static_cast<bool>(analyzeResult)) << "Looks like we have an issues with analyzer";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 2) << "Expected to have 2 types";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getKind(), rg3::cpp::TypeKind::TK_ENUM) << "Expected to have enum";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getName(), "ESample") << "Bad name";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getPrettyName(), "engine::render::ESample") << "Bad pretty name";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getNamespace().asString(), "engine::render") << "Invalid namespace detected";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->isForwardDeclarable(), true) << "This enum must be forward declarable!";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getTags().hasTag("runtime"), true);

	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getKind(), rg3::cpp::TypeKind::TK_ENUM) << "Expected to have enum";
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getName(), "EProfileType") << "Bad name";
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getPrettyName(), "core::EProfileType") << "Bad pretty name";
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getNamespace().asString(), "core") << "Invalid namespace detected";
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->isForwardDeclarable(), true) << "This enum must be forward declarable!";
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getTags().hasTag("runtime"), true);

	auto asEnum0 = static_cast<rg3::cpp::TypeEnum*>(analyzeResult.vFoundTypes[0].get()); // NOLINT(*-pro-type-static-cast-downcast)
	auto asEnum1 = static_cast<rg3::cpp::TypeEnum*>(analyzeResult.vFoundTypes[1].get()); // NOLINT(*-pro-type-static-cast-downcast)

	// enum #0 - ESample
	ASSERT_EQ(asEnum0->getEntries().size(), 2);
	ASSERT_EQ(asEnum0->getEntries()[0].sName, "ES_FIRST");
	ASSERT_EQ(asEnum0->getEntries()[1].sName, "ES_SECOND");

	// enum #1 - EProfileType
	ASSERT_EQ(asEnum1->getEntries().size(), 2);
	ASSERT_EQ(asEnum1->getEntries()[0].sName, "PT_ADMIN");
	ASSERT_EQ(asEnum1->getEntries()[1].sName, "PT_USER");
}

TEST_F(Tests_Enum, CheckCustomProtoInclude)
{
	g_Analyzer->setSourceCode(R"(
#proto_include <Protocol/RPC/Engine.proto>
)");

	auto& compilerConfig = g_Analyzer->getCompilerConfig();
	compilerConfig.cppStandard = rg3::llvm::CxxStandard::CC_20;

	const auto analyzeResult = g_Analyzer->analyze();

	//ASSERT_EQ(analyzeResult.vIssues.size(), 0);
}