#include <gtest/gtest.h>

#include <RG3/Cpp/TypeBase.h>
#include <RG3/Cpp/TypeEnum.h>
#include <RG3/Cpp/TypeClass.h>
#include <RG3/LLVM/CodeAnalyzer.h>



class Tests_AnonymousRegistration : public ::testing::Test
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

TEST_F(Tests_AnonymousRegistration, TrivialAndSTLTypesRegistration)
{
	g_Analyzer->setSourceCode(R"(
#include <string>
#include <cstdint>

// Registrator
template <typename T> struct RegisterType {};

// Types
template <> struct
	__attribute__((annotate("RG3_RegisterRuntime")))
	__attribute__((annotate("RG3_OverrideLocation[cstdint]")))
RegisterType<uint8_t> {
	using Type = uint8_t;
};

template <> struct
	__attribute__((annotate("RG3_RegisterRuntime")))
RegisterType<int> {
	using Type = int;
};

template <> struct
	__attribute__((annotate("RG3_RegisterRuntime")))
RegisterType<float> {
	using Type = float;
};

template <> struct
	__attribute__((annotate("RG3_RegisterRuntime")))
RegisterType<double> {
	using Type = double;
};

template <> struct
	__attribute__((annotate("RG3_RegisterRuntime")))
	__attribute__((annotate("RG3_OverrideLocation[string]")))
RegisterType<std::string> {
	using Type = std::string;
};
)");

	auto& compilerConfig = g_Analyzer->getCompilerConfig();
	compilerConfig.cppStandard = rg3::llvm::CxxStandard::CC_17;

	const auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(analyzeResult.vIssues.empty()) << "Got errors!";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 5) << "Expected to have 5 type here, got " << analyzeResult.vFoundTypes.size();

	// Check types
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getName(), "uint8_t"); // weird case: all std::(u)int(8|16|32|64) types are located in global namespace, std:: is just a proxy. Idk, let it be here like this
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getPrettyName(), "uint8_t");
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getNamespace().asString(), "");
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getDefinition().isAngledPath(), true);
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getDefinition().getPath(), "cstdint");
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getDefinition().getLine(), 0);
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getDefinition().getInLineOffset(), 0);
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getKind(), rg3::cpp::TypeKind::TK_TRIVIAL);

	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getName(), "int");
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getPrettyName(), "int");
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getNamespace().asString(), "");
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getDefinition().isAngledPath(), false);
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getDefinition().getPath(), "");
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getDefinition().getLine(), 0);
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getDefinition().getInLineOffset(), 0);
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getKind(), rg3::cpp::TypeKind::TK_TRIVIAL);

	ASSERT_EQ(analyzeResult.vFoundTypes[2]->getName(), "float");
	ASSERT_EQ(analyzeResult.vFoundTypes[2]->getPrettyName(), "float");
	ASSERT_EQ(analyzeResult.vFoundTypes[2]->getNamespace().asString(), "");
	ASSERT_EQ(analyzeResult.vFoundTypes[2]->getDefinition().isAngledPath(), false);
	ASSERT_EQ(analyzeResult.vFoundTypes[2]->getDefinition().getPath(), "");
	ASSERT_EQ(analyzeResult.vFoundTypes[2]->getDefinition().getLine(), 0);
	ASSERT_EQ(analyzeResult.vFoundTypes[2]->getDefinition().getInLineOffset(), 0);
	ASSERT_EQ(analyzeResult.vFoundTypes[2]->getKind(), rg3::cpp::TypeKind::TK_TRIVIAL);

	ASSERT_EQ(analyzeResult.vFoundTypes[3]->getName(), "double");
	ASSERT_EQ(analyzeResult.vFoundTypes[3]->getPrettyName(), "double");
	ASSERT_EQ(analyzeResult.vFoundTypes[3]->getNamespace().asString(), "");
	ASSERT_EQ(analyzeResult.vFoundTypes[3]->getDefinition().isAngledPath(), false);
	ASSERT_EQ(analyzeResult.vFoundTypes[3]->getDefinition().getPath(), "");
	ASSERT_EQ(analyzeResult.vFoundTypes[3]->getDefinition().getLine(), 0);
	ASSERT_EQ(analyzeResult.vFoundTypes[3]->getDefinition().getInLineOffset(), 0);
	ASSERT_EQ(analyzeResult.vFoundTypes[3]->getKind(), rg3::cpp::TypeKind::TK_TRIVIAL);

	ASSERT_EQ(analyzeResult.vFoundTypes[4]->getName(), "string");
	ASSERT_EQ(analyzeResult.vFoundTypes[4]->getPrettyName(), "std::string");
	ASSERT_EQ(analyzeResult.vFoundTypes[4]->getNamespace().asString(), "std");
	ASSERT_EQ(analyzeResult.vFoundTypes[4]->getDefinition().isAngledPath(), true);
	ASSERT_EQ(analyzeResult.vFoundTypes[4]->getDefinition().getPath(), "string");
	ASSERT_EQ(analyzeResult.vFoundTypes[4]->getDefinition().getLine(), 0);
	ASSERT_EQ(analyzeResult.vFoundTypes[4]->getDefinition().getInLineOffset(), 0);
	ASSERT_EQ(analyzeResult.vFoundTypes[4]->getKind(), rg3::cpp::TypeKind::TK_STRUCT_OR_CLASS);

	ASSERT_TRUE(reinterpret_cast<rg3::cpp::TypeClass*>(analyzeResult.vFoundTypes[4].get())->getProperties().empty());
	ASSERT_TRUE(reinterpret_cast<rg3::cpp::TypeClass*>(analyzeResult.vFoundTypes[4].get())->getFunctions().empty());
}

TEST_F(Tests_AnonymousRegistration, SimpleUsage)
{
	g_Analyzer->setSourceCode(R"(
namespace engine::math {
	struct V2
	{
		float x { .0f };
		float y { .0f };
		bool bUseHacks { false };

	private:
		bool IsNormalized() const;

	public:
		void Reset();
	};
}

namespace game {
	using Vec2 = engine::math::V2;
}

// Registrator
template <typename T> struct RegisterType {};
template <> struct
	__attribute__((annotate("RG3_RegisterRuntime")))
	__attribute__((annotate("RG3_RegisterField[x:MyPropX]")))
	__attribute__((annotate("RG3_RegisterField[y:MyPropY]")))
	__attribute__((annotate("RG3_RegisterFunction[IsNormalized]")))
	__attribute__((annotate("RG3_RegisterTag[@serialize(\"MyCoolSerializer\")]")))
	__attribute__((annotate("RG3_OverrideLocation[cgame]")))
RegisterType<game::Vec2> {
	using Type = game::Vec2;
};
)");

	auto& compilerConfig = g_Analyzer->getCompilerConfig();
	compilerConfig.cppStandard = rg3::llvm::CxxStandard::CC_17;

	const auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(analyzeResult.vIssues.empty()) << "Got errors!";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 1) << "Expected to have 1 type here, got " << analyzeResult.vFoundTypes.size();

	// Check type
	auto* pType = analyzeResult.vFoundTypes[0].get();
	ASSERT_EQ(pType->getName(), "Vec2");
	ASSERT_EQ(pType->getPrettyName(), "game::Vec2");
	ASSERT_EQ(pType->getNamespace().asString(), "game");
	ASSERT_EQ(pType->getDefinition().getPath(), "cgame") << "Annotation RG3_OverrideLocation not working (wrong path)!";
	ASSERT_EQ(pType->getDefinition().getLine(), 0) << "Annotation RG3_OverrideLocation failed: line expected to be zero!";
	ASSERT_EQ(pType->getDefinition().getInLineOffset(), 0) << "Annotation RG3_OverrideLocation failed: column expected to be zero!";
	ASSERT_EQ(pType->getDefinition().isAngledPath(), true) << "Annotation RG3_OverrideLocation failed: path must be angled";
	ASSERT_EQ(pType->getKind(), rg3::cpp::TypeKind::TK_STRUCT_OR_CLASS) << "Expected to have class or struct";
	ASSERT_EQ(pType->getTags().getTags().size(), 2) << "Expected to have at least 2 tags";
	ASSERT_EQ(pType->getTags().hasTag("runtime"), true);
	ASSERT_EQ(pType->getTags().hasTag("serialize"), true);
	ASSERT_EQ(pType->getTags().getTag("runtime").getArgumentsCount(), 0);
	ASSERT_EQ(pType->getTags().getTag("serialize").getArgumentsCount(), 1);
	ASSERT_EQ(pType->getTags().getTag("serialize").getArguments()[0].getHoldedType(), rg3::cpp::TagArgumentType::AT_STRING);
	ASSERT_EQ(pType->getTags().getTag("serialize").getArguments()[0].asString(""), "MyCoolSerializer");

	// Check class
	auto* pClass = reinterpret_cast<rg3::cpp::TypeClass*>(pType);
	ASSERT_EQ(pClass->isStruct(), true);
	ASSERT_EQ(pClass->getFunctions().size(), 1) << "Expected to have only 1 function";
	ASSERT_EQ(pClass->getProperties().size(), 2) << "Expected to have 2 properties";

	ASSERT_EQ(pClass->getFunctions()[0].sName, "IsNormalized");
	ASSERT_EQ(pClass->getFunctions()[0].bIsConst, true);
	ASSERT_EQ(pClass->getFunctions()[0].vTags.getTags().size(), 0);
	ASSERT_EQ(pClass->getFunctions()[0].sOwnerClassName, "Vec2");
	ASSERT_EQ(pClass->getFunctions()[0].vArguments.size(), 0);
	ASSERT_EQ(pClass->getFunctions()[0].eVisibility, rg3::cpp::ClassEntryVisibility::CEV_PRIVATE);
	ASSERT_EQ(pClass->getFunctions()[0].sReturnType.sTypeRef.getRefName(), "bool");

	ASSERT_EQ(pClass->getProperties()[0].eVisibility, rg3::cpp::ClassEntryVisibility::CEV_PUBLIC);
	ASSERT_EQ(pClass->getProperties()[0].sName, "x");
	ASSERT_EQ(pClass->getProperties()[0].sAlias, "MyPropX");
	ASSERT_EQ(pClass->getProperties()[0].sTypeInfo.sTypeRef.getRefName(), "float");
	ASSERT_EQ(pClass->getProperties()[0].vTags.getTags().empty(), true);

	ASSERT_EQ(pClass->getProperties()[1].eVisibility, rg3::cpp::ClassEntryVisibility::CEV_PUBLIC);
	ASSERT_EQ(pClass->getProperties()[1].sName, "y");
	ASSERT_EQ(pClass->getProperties()[1].sAlias, "MyPropY");
	ASSERT_EQ(pClass->getProperties()[1].sTypeInfo.sTypeRef.getRefName(), "float");
	ASSERT_EQ(pClass->getProperties()[1].vTags.getTags().empty(), true);
}

TEST_F(Tests_AnonymousRegistration, SimpleEnum)
{
	g_Analyzer->setSourceCode(R"(
enum EBasicEnum { BE_ONE = 1, BE_THREE = 3, BE_THREE_HUNDRED_BUCKS = 300 };
enum class EScopedEnum { SE_ONE = 1, SE_FF = 0xFF };

// Registrator
template <typename T> struct RegisterType {};

template <> struct
	__attribute__((annotate("RG3_RegisterRuntime")))
RegisterType<EBasicEnum> {
	using Type = EBasicEnum;
};

template <> struct
	__attribute__((annotate("RG3_RegisterRuntime")))
RegisterType<EScopedEnum> {
	using Type = EScopedEnum;
};
)");

	auto& compilerConfig = g_Analyzer->getCompilerConfig();
	compilerConfig.cppStandard = rg3::llvm::CxxStandard::CC_17;

	const auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(analyzeResult.vIssues.empty()) << "Got errors!";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 2) << "Expected to have 2 type here, got " << analyzeResult.vFoundTypes.size();

	// Check type
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getName(), "EBasicEnum");
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getKind(), rg3::cpp::TypeKind::TK_ENUM);
	ASSERT_EQ(reinterpret_cast<rg3::cpp::TypeEnum*>(analyzeResult.vFoundTypes[0].get())->isScoped(), false);
	ASSERT_EQ(reinterpret_cast<rg3::cpp::TypeEnum*>(analyzeResult.vFoundTypes[0].get())->getEntries().size(), 3);
	ASSERT_EQ(reinterpret_cast<rg3::cpp::TypeEnum*>(analyzeResult.vFoundTypes[0].get())->getEntries()[0].sName, "BE_ONE");
	ASSERT_EQ(reinterpret_cast<rg3::cpp::TypeEnum*>(analyzeResult.vFoundTypes[0].get())->getEntries()[0].iValue, 1);
	ASSERT_EQ(reinterpret_cast<rg3::cpp::TypeEnum*>(analyzeResult.vFoundTypes[0].get())->getEntries()[1].sName, "BE_THREE");
	ASSERT_EQ(reinterpret_cast<rg3::cpp::TypeEnum*>(analyzeResult.vFoundTypes[0].get())->getEntries()[1].iValue, 3);
	ASSERT_EQ(reinterpret_cast<rg3::cpp::TypeEnum*>(analyzeResult.vFoundTypes[0].get())->getEntries()[2].sName, "BE_THREE_HUNDRED_BUCKS");
	ASSERT_EQ(reinterpret_cast<rg3::cpp::TypeEnum*>(analyzeResult.vFoundTypes[0].get())->getEntries()[2].iValue, 300);

	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getName(), "EScopedEnum");
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getKind(), rg3::cpp::TypeKind::TK_ENUM);
	ASSERT_EQ(reinterpret_cast<rg3::cpp::TypeEnum*>(analyzeResult.vFoundTypes[1].get())->isScoped(), true);
	ASSERT_EQ(reinterpret_cast<rg3::cpp::TypeEnum*>(analyzeResult.vFoundTypes[1].get())->getEntries().size(), 2);
	ASSERT_EQ(reinterpret_cast<rg3::cpp::TypeEnum*>(analyzeResult.vFoundTypes[1].get())->getEntries()[0].sName, "SE_ONE");
	ASSERT_EQ(reinterpret_cast<rg3::cpp::TypeEnum*>(analyzeResult.vFoundTypes[1].get())->getEntries()[0].iValue, 1);
	ASSERT_EQ(reinterpret_cast<rg3::cpp::TypeEnum*>(analyzeResult.vFoundTypes[1].get())->getEntries()[1].sName, "SE_FF");
	ASSERT_EQ(reinterpret_cast<rg3::cpp::TypeEnum*>(analyzeResult.vFoundTypes[1].get())->getEntries()[1].iValue, 255);
}
// Commented for v0.0.3, for v0.0.4 will support this
//TEST_F(Tests_AnonymousRegistration, CheckTemplateSpecilizationBehaviour)
//{
//	g_Analyzer->setSourceCode(R"(
//namespace engine::math {
//	template <typename T>
//	struct Vector2D
//	{
//		T x;
//		T y;
//	};
//}
//
//namespace game {
//	using V2F = engine::math::Vector2D<float>;
//	using V2I = engine::math::Vector2D<int>;
//}
//
//
//// Registrator
//template <typename T> struct RegisterType {};
//
//template <> struct
//	__attribute__((annotate("RG3_RegisterRuntime")))
//	__attribute__((annotate("RG3_RegisterField[x]")))
//	__attribute__((annotate("RG3_RegisterField[y]")))
//RegisterType<game::V2F> {
//	using Type = game::V2F;
//};
//
//template <> struct
//	__attribute__((annotate("RG3_RegisterRuntime")))
//	__attribute__((annotate("RG3_RegisterField[x]")))
//	__attribute__((annotate("RG3_RegisterField[y]")))
//RegisterType<game::V2I> {
//	using Type = game::V2I;
//};
//)");
//
//	auto& compilerConfig = g_Analyzer->getCompilerConfig();
//	compilerConfig.cppStandard = rg3::llvm::CxxStandard::CC_17;
//
//	const auto analyzeResult = g_Analyzer->analyze();
//
//	ASSERT_TRUE(analyzeResult.vIssues.empty()) << "Got errors!";
//	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 2) << "Expected to have 1 type here, got " << analyzeResult.vFoundTypes.size();
//}