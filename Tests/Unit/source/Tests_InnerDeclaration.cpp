#include <gtest/gtest.h>

#include <RG3/Cpp/Tag.h>
#include <RG3/Cpp/TypeBase.h>
#include <RG3/Cpp/TypeClass.h>
#include <RG3/Cpp/TypeEnum.h>
#include <RG3/LLVM/CodeAnalyzer.h>


class Tests_InnerDeclaration : public ::testing::Test
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

TEST_F(Tests_InnerDeclaration, CheckInnerDecl)
{
	g_Analyzer->setSourceCode(R"(
namespace base {
	/// @runtime
	struct MyStruct
	{
		/// @runtime
		enum class MyEnum { ENT_0 = 0, ENT_1 = 1, ENT_2 = 2 };
	};
}
)");

	g_Analyzer->getCompilerConfig().cppStandard = rg3::llvm::CxxStandard::CC_17;

	const auto analyzeResult = g_Analyzer->analyze();

	// expected to have 8 aliases
	ASSERT_TRUE(analyzeResult.vIssues.empty()) << "No issues should be here";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 2) << "Only 2 type should be here";

	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getName(), "MyStruct");
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getKind(), rg3::cpp::TypeKind::TK_STRUCT_OR_CLASS);
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getPrettyName(), "base::MyStruct");

	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getName(), "MyStruct::MyEnum");
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getKind(), rg3::cpp::TypeKind::TK_ENUM);
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getPrettyName(), "base::MyStruct::MyEnum");
	ASSERT_EQ(static_cast<const rg3::cpp::TypeEnum*>(analyzeResult.vFoundTypes[1].get())->isScoped(), true);
}

TEST_F(Tests_InnerDeclaration, CheckDeepPathAndInnerDeclarationWithAnnotationScheme)
{
	g_Analyzer->setSourceCode(R"(
namespace awesome::location {
	struct MyCoolEnt
	{
		struct AnotherThing
		{};
	};
}

using namespace awesome::location; // make it harder

// Registrator
template <typename T> struct RegisterType {};

// Alias registration
template <> struct
	__attribute__((annotate("RG3_RegisterRuntime")))
	__attribute__((annotate("RG3_MakeTypeTrivial")))
	__attribute__((annotate("RG3_OverrideLocation[some/deep/location.hpp]")))
RegisterType<MyCoolEnt::AnotherThing> {
	using Type = MyCoolEnt::AnotherThing;
};
)");

	auto& compilerConfig = g_Analyzer->getCompilerConfig();
	compilerConfig.cppStandard = rg3::llvm::CxxStandard::CC_17;

	const auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(analyzeResult.vIssues.empty()) << "Got errors!";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 1) << "Expected to have 1 types here";

	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getName(), "MyCoolEnt::AnotherThing");
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getPrettyName(), "awesome::location::MyCoolEnt::AnotherThing");
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getKind(), rg3::cpp::TypeKind::TK_TRIVIAL);
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getDefinition().getPath(), "some/deep/location.hpp");
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getDefinition().isAngledPath(), true);
}

TEST_F(Tests_InnerDeclaration, CheckInnerRegisterInsideTemplate)
{
	g_Analyzer->setSourceCode(R"(
namespace some::cool::space {
	template <typename T>
	struct TContainer
	{
		enum class EKind {
			EK_1 = 1,
			EK_3 = 3,
			EK_37 = 37
		};

		enum EScope : short {
			SC_0 = 20,
			SC_8 = 72
		};

		T someVal1;
		EKind kind;
	};
}

namespace easy::space {
	using FloatContainer = some::cool::space::TContainer<float>;
}

using namespace easy::space;

// Registrator
template <typename T> struct RegisterType {};

// Alias registration
template <> struct
	__attribute__((annotate("RG3_RegisterRuntime")))
RegisterType<FloatContainer::EKind> {
	using Type = FloatContainer::EKind;
};

template <> struct
	__attribute__((annotate("RG3_RegisterRuntime")))
RegisterType<some::cool::space::TContainer<bool>::EKind> {
	using Type = some::cool::space::TContainer<bool>::EKind;
};

template <> struct
	__attribute__((annotate("RG3_RegisterRuntime")))
RegisterType<some::cool::space::TContainer<bool>::EScope> {
	using Type = some::cool::space::TContainer<bool>::EScope;
};
)");

	auto& compilerConfig = g_Analyzer->getCompilerConfig();
	compilerConfig.cppStandard = rg3::llvm::CxxStandard::CC_17;

	const auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(analyzeResult.vIssues.empty()) << "Got errors!";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 3) << "Expected to have 3 types here";

	// NOTE: Type FloatContainer::EKind will be unrolled to some::cool::space::TContainer<float> because of LLVM. Maybe will be fixed later.
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getName(), "TContainer<float>::EKind");
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getPrettyName(), "some::cool::space::TContainer<float>::EKind");
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getNamespace().asString(), "some::cool::space");
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getKind(), rg3::cpp::TypeKind::TK_ENUM);
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getDefinition().isAngledPath(), false);
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getDefinition().getPath(), "id0.hpp");
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->isForwardDeclarable(), false);
	auto* asEnum0 = reinterpret_cast<const rg3::cpp::TypeEnum*>(analyzeResult.vFoundTypes[0].get());
	ASSERT_EQ(asEnum0->getEntries().size(), 3);
	ASSERT_EQ(asEnum0->getEntries()[0].sName, "EK_1");
	ASSERT_EQ(asEnum0->getEntries()[0].iValue, 1);
	ASSERT_EQ(asEnum0->getEntries()[1].sName, "EK_3");
	ASSERT_EQ(asEnum0->getEntries()[1].iValue, 3);
	ASSERT_EQ(asEnum0->getEntries()[2].sName, "EK_37");
	ASSERT_EQ(asEnum0->getEntries()[2].iValue, 37);
	ASSERT_EQ(asEnum0->isScoped(), true);

	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getName(), "TContainer<bool>::EKind");
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getPrettyName(), "some::cool::space::TContainer<bool>::EKind");
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getNamespace().asString(), "some::cool::space");
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getKind(), rg3::cpp::TypeKind::TK_ENUM);
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getDefinition().isAngledPath(), false);
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getDefinition().getPath(), "id0.hpp");
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->isForwardDeclarable(), false);
	auto* asEnum1 = reinterpret_cast<const rg3::cpp::TypeEnum*>(analyzeResult.vFoundTypes[1].get());
	ASSERT_EQ(asEnum1->getEntries().size(), 3);
	ASSERT_EQ(asEnum1->getEntries()[0].sName, "EK_1");
	ASSERT_EQ(asEnum1->getEntries()[0].iValue, 1);
	ASSERT_EQ(asEnum1->getEntries()[1].sName, "EK_3");
	ASSERT_EQ(asEnum1->getEntries()[1].iValue, 3);
	ASSERT_EQ(asEnum1->getEntries()[2].sName, "EK_37");
	ASSERT_EQ(asEnum1->getEntries()[2].iValue, 37);
	ASSERT_EQ(asEnum1->isScoped(), true);

	ASSERT_EQ(analyzeResult.vFoundTypes[2]->getName(), "TContainer<bool>::EScope");
	ASSERT_EQ(analyzeResult.vFoundTypes[2]->getPrettyName(), "some::cool::space::TContainer<bool>::EScope");
	ASSERT_EQ(analyzeResult.vFoundTypes[2]->getNamespace().asString(), "some::cool::space");
	ASSERT_EQ(analyzeResult.vFoundTypes[2]->getKind(), rg3::cpp::TypeKind::TK_ENUM);
	ASSERT_EQ(analyzeResult.vFoundTypes[2]->getDefinition().isAngledPath(), false);
	ASSERT_EQ(analyzeResult.vFoundTypes[2]->getDefinition().getPath(), "id0.hpp");
	ASSERT_EQ(analyzeResult.vFoundTypes[2]->isForwardDeclarable(), false);
	auto* asEnum2 = reinterpret_cast<const rg3::cpp::TypeEnum*>(analyzeResult.vFoundTypes[2].get());
	ASSERT_EQ(asEnum2->getEntries().size(), 2);
	ASSERT_EQ(asEnum2->getEntries()[0].sName, "SC_0");
	ASSERT_EQ(asEnum2->getEntries()[0].iValue, 20);
	ASSERT_EQ(asEnum2->getEntries()[1].sName, "SC_8");
	ASSERT_EQ(asEnum2->getEntries()[1].iValue, 72);
	ASSERT_EQ(asEnum2->isScoped(), false);
}