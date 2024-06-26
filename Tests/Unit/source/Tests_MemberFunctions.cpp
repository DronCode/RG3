#include <gtest/gtest.h>

#include <RG3/Cpp/Tag.h>
#include <RG3/Cpp/TypeBase.h>
#include <RG3/Cpp/TypeClass.h>
#include <RG3/LLVM/CodeAnalyzer.h>


class Tests_MemberFunctions : public ::testing::Test
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

TEST_F(Tests_MemberFunctions, CheckMemberFunctions)
{
	g_Analyzer->setSourceCode(R"(
namespace my_cool_space
{
	/// @runtime
	struct MyFooClass
	{
		static void DoSomething();
		int GetHealth() const;
		void AddHealth(int hp);
	protected:
		void SubHealth(unsigned int hp);
	};
}
)");

	g_Analyzer->getCompilerConfig().cppStandard = rg3::llvm::CxxStandard::CC_17;

	const auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(analyzeResult.vIssues.empty()) << "No issues should be here";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 1) << "Only 1 type should be here";

	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getPrettyName(), "my_cool_space::MyFooClass") << "Wrong pretty name";
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getKind(), rg3::cpp::TypeKind::TK_STRUCT_OR_CLASS) << "Wrong kind";

	auto asClass = static_cast<const rg3::cpp::TypeClass*>(analyzeResult.vFoundTypes[0].get()); // NOLINT(*-pro-type-static-cast-downcast)

	ASSERT_EQ(asClass->getProperties().size(), 0);
	ASSERT_EQ(asClass->getFunctions().size(), 4);

	ASSERT_EQ(asClass->getFunctions()[0].sName, "DoSomething");
	ASSERT_EQ(asClass->getFunctions()[0].eVisibility, rg3::cpp::ClassEntryVisibility::CEV_PUBLIC);
	ASSERT_EQ(asClass->getFunctions()[0].sOwnerClassName, "my_cool_space::MyFooClass");
	ASSERT_EQ(asClass->getFunctions()[0].bIsStatic, true);
	ASSERT_EQ(asClass->getFunctions()[0].bIsConst, false);

	ASSERT_EQ(asClass->getFunctions()[1].sName, "GetHealth");
	ASSERT_EQ(asClass->getFunctions()[1].eVisibility, rg3::cpp::ClassEntryVisibility::CEV_PUBLIC);
	ASSERT_EQ(asClass->getFunctions()[1].sOwnerClassName, "my_cool_space::MyFooClass");
	ASSERT_EQ(asClass->getFunctions()[1].bIsStatic, false);
	ASSERT_EQ(asClass->getFunctions()[1].bIsConst, true);

	ASSERT_EQ(asClass->getFunctions()[2].sName, "AddHealth");
	ASSERT_EQ(asClass->getFunctions()[2].eVisibility, rg3::cpp::ClassEntryVisibility::CEV_PUBLIC);
	ASSERT_EQ(asClass->getFunctions()[2].sOwnerClassName, "my_cool_space::MyFooClass");
	ASSERT_EQ(asClass->getFunctions()[2].bIsStatic, false);
	ASSERT_EQ(asClass->getFunctions()[2].bIsConst, false);

	ASSERT_EQ(asClass->getFunctions()[3].sName, "SubHealth");
	ASSERT_EQ(asClass->getFunctions()[3].eVisibility, rg3::cpp::ClassEntryVisibility::CEV_PROTECTED);
	ASSERT_EQ(asClass->getFunctions()[3].sOwnerClassName, "my_cool_space::MyFooClass");
	ASSERT_EQ(asClass->getFunctions()[3].bIsStatic, false);
	ASSERT_EQ(asClass->getFunctions()[3].bIsConst, false);
}

TEST_F(Tests_MemberFunctions, CheckFunctionArgumentPointerRefQualifiers)
{
	g_Analyzer->setSourceCode(R"(
/// @runtime
struct Vector3
{
	/// @property
	float x = 0.f;

	/// @property
	float y = 0.f;

	/// @property
	float z = 0.f;
};

/// @runtime
struct TransformComponent
{
	/// @property(vPos)
	const Vector3 position{};

	void MoveTo(const Vector3& vNewPos);
	const Vector3& GetPosition() const;

	static bool IsSomething();
};
)");

	g_Analyzer->getCompilerConfig().cppStandard = rg3::llvm::CxxStandard::CC_20;

	const auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(analyzeResult.vIssues.empty()) << "No issues should be here";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 2) << "Only 2 type should be here";

	// First type (check properties)
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getKind(), rg3::cpp::TypeKind::TK_STRUCT_OR_CLASS);

	auto asClass = static_cast<const rg3::cpp::TypeClass*>(analyzeResult.vFoundTypes[0].get()); // NOLINT(*-pro-type-static-cast-downcast)
	ASSERT_EQ(asClass->getFunctions().size(), 0);
	ASSERT_EQ(asClass->getProperties().size(), 3);

	ASSERT_EQ(asClass->getProperties()[0].sName, "x");
	ASSERT_EQ(asClass->getProperties()[0].sTypeInfo.sTypeRef.getRefName(), "float");
	ASSERT_EQ(asClass->getProperties()[1].sName, "y");
	ASSERT_EQ(asClass->getProperties()[1].sTypeInfo.sTypeRef.getRefName(), "float");
	ASSERT_EQ(asClass->getProperties()[2].sName, "z");
	ASSERT_EQ(asClass->getProperties()[2].sTypeInfo.sTypeRef.getRefName(), "float");

	// Second type (check properties & functions)
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getKind(), rg3::cpp::TypeKind::TK_STRUCT_OR_CLASS);

	asClass = static_cast<const rg3::cpp::TypeClass*>(analyzeResult.vFoundTypes[1].get()); // NOLINT(*-pro-type-static-cast-downcast)

	ASSERT_EQ(asClass->getFunctions().size(), 3);
	ASSERT_EQ(asClass->getProperties().size(), 1);

	ASSERT_EQ(asClass->getProperties()[0].sName, "position");
	ASSERT_EQ(asClass->getProperties()[0].sAlias, "vPos");
	ASSERT_EQ(asClass->getProperties()[0].sTypeInfo.bIsConst, true);
	ASSERT_EQ(asClass->getProperties()[0].sTypeInfo.sDefinitionLocation.has_value(), true);
	ASSERT_EQ(asClass->getProperties()[0].sTypeInfo.sDefinitionLocation.value().getPath(), "id0.hpp");

	ASSERT_EQ(asClass->getFunctions()[0].sName, "MoveTo");
	ASSERT_EQ(asClass->getFunctions()[0].sReturnType.isVoid(), true);
	ASSERT_EQ(asClass->getFunctions()[0].vArguments.size(), 1);
	ASSERT_EQ(asClass->getFunctions()[0].vArguments[0].bHasDefaultValue, false);
	ASSERT_EQ(asClass->getFunctions()[0].vArguments[0].sArgumentName, "vNewPos");
	ASSERT_EQ(asClass->getFunctions()[0].vArguments[0].sType.bIsPtrConst, true);
	ASSERT_EQ(asClass->getFunctions()[0].vArguments[0].sType.bIsPointer, false);
	ASSERT_EQ(asClass->getFunctions()[0].vArguments[0].sType.bIsReference, true);
	ASSERT_EQ(asClass->getFunctions()[0].vArguments[0].sType.sBaseInfo.sName, "Vector3");
	ASSERT_EQ(asClass->getFunctions()[0].vArguments[0].sType.sBaseInfo.sPrettyName, "Vector3");
	ASSERT_EQ(asClass->getFunctions()[0].vArguments[0].sType.sBaseInfo.sDefLocation.getPath(), "id0.hpp");
	ASSERT_EQ(asClass->getFunctions()[0].vArguments[0].sType.sBaseInfo.eKind, rg3::cpp::TypeKind::TK_STRUCT_OR_CLASS);
	ASSERT_EQ(asClass->getFunctions()[0].vArguments[0].sType.sTypeRef.getRefName(), "Vector3");
	ASSERT_EQ(asClass->getFunctions()[0].vArguments[0].sType.sDefinitionLocation.has_value(), true);
	ASSERT_EQ(asClass->getFunctions()[0].vArguments[0].sType.sDefinitionLocation.value().getPath(), "id0.hpp");

	ASSERT_EQ(asClass->getFunctions()[1].sName, "GetPosition");
	ASSERT_EQ(asClass->getFunctions()[1].vArguments.size(), 0);
	ASSERT_EQ(asClass->getFunctions()[1].sReturnType.isVoid(), false);
	ASSERT_EQ(asClass->getFunctions()[1].sReturnType.bIsPtrConst, true);
	ASSERT_EQ(asClass->getFunctions()[1].sReturnType.bIsReference, true);
	ASSERT_EQ(asClass->getFunctions()[1].sReturnType.bIsPointer, false);
	ASSERT_EQ(asClass->getFunctions()[1].sReturnType.sDefinitionLocation.has_value(), true);
	ASSERT_EQ(asClass->getFunctions()[1].sReturnType.sDefinitionLocation.value().getPath(), "id0.hpp");
	ASSERT_EQ(asClass->getFunctions()[1].sReturnType.sTypeRef.getRefName(), "Vector3");
	ASSERT_EQ(asClass->getFunctions()[1].sReturnType.sBaseInfo.sName, "Vector3");
	ASSERT_EQ(asClass->getFunctions()[1].sReturnType.sBaseInfo.sPrettyName, "Vector3");
	ASSERT_EQ(asClass->getFunctions()[1].sReturnType.sBaseInfo.sDefLocation.getPath(), "id0.hpp");
	ASSERT_EQ(asClass->getFunctions()[1].sReturnType.sBaseInfo.eKind, rg3::cpp::TypeKind::TK_STRUCT_OR_CLASS);

	ASSERT_EQ(asClass->getFunctions()[1].bIsConst, true);

	ASSERT_EQ(asClass->getFunctions()[2].sName, "IsSomething");
	ASSERT_EQ(asClass->getFunctions()[2].vArguments.size(), 0);
	ASSERT_EQ(asClass->getFunctions()[2].sReturnType.sTypeRef.getRefName(), "bool");
	ASSERT_EQ(asClass->getFunctions()[2].bIsStatic, true);
}


TEST_F(Tests_MemberFunctions, CheckMemberFuncOwnershipInTemplateSpec)
{
	g_Analyzer->setSourceCode(R"(
namespace engine::core {
	template <typename T>
	struct Vec3
	{
		T x;
		T y;
		T z;

		T GetDistanceTo(const T&) const;
	};
}

// Registrator
template <typename T> struct RegisterType {};

template <> struct
	__attribute__((annotate("RG3_RegisterRuntime")))
	__attribute__((annotate("RG3_RegisterField[x]")))
	__attribute__((annotate("RG3_RegisterField[y]")))
	__attribute__((annotate("RG3_RegisterField[z]")))
	__attribute__((annotate("RG3_RegisterFunction[GetDistanceTo]")))
RegisterType<engine::core::Vec3<float>> {
	using Type = engine::core::Vec3<float>;
};

template <> struct
	__attribute__((annotate("RG3_RegisterRuntime")))
	__attribute__((annotate("RG3_RegisterField[x]")))
	__attribute__((annotate("RG3_RegisterField[y]")))
	__attribute__((annotate("RG3_RegisterField[z]")))
	__attribute__((annotate("RG3_RegisterFunction[GetDistanceTo]")))
RegisterType<engine::core::Vec3<int>> {
	using Type = engine::core::Vec3<int>;
};
)");

	g_Analyzer->getCompilerConfig().cppStandard = rg3::llvm::CxxStandard::CC_20;

	const auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(analyzeResult.vIssues.empty()) << "No issues should be here";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 2) << "Only 2 type should be here";

	// First type
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getKind(), rg3::cpp::TypeKind::TK_STRUCT_OR_CLASS);
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getName(), "Vec3<float>");
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getPrettyName(), "engine::core::Vec3<float>");
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->isForwardDeclarable(), false) << "Non forward declarable (template spec)";
	auto* asClass0 = reinterpret_cast<rg3::cpp::TypeClass*>(analyzeResult.vFoundTypes[0].get());

	ASSERT_EQ(asClass0->getProperties().size(), 3);
	ASSERT_EQ(asClass0->getProperties()[0].sName, "x");
	ASSERT_EQ(asClass0->getProperties()[0].sAlias, "x");
	ASSERT_EQ(asClass0->getProperties()[0].sTypeInfo.sTypeRef.getRefName(), "float");
	ASSERT_EQ(asClass0->getProperties()[0].sTypeInfo.sBaseInfo.sName, "float");
	ASSERT_EQ(asClass0->getProperties()[0].sTypeInfo.sBaseInfo.sPrettyName, "float");
	ASSERT_EQ(asClass0->getProperties()[0].sTypeInfo.sBaseInfo.eKind, rg3::cpp::TypeKind::TK_TRIVIAL);
	ASSERT_EQ(asClass0->getProperties()[0].vTags.hasTag("property"), true);
	ASSERT_EQ(asClass0->getProperties()[1].sName, "y");
	ASSERT_EQ(asClass0->getProperties()[1].sAlias, "y");
	ASSERT_EQ(asClass0->getProperties()[1].sTypeInfo.sBaseInfo.sName, "float");
	ASSERT_EQ(asClass0->getProperties()[1].sTypeInfo.sBaseInfo.sPrettyName, "float");
	ASSERT_EQ(asClass0->getProperties()[1].sTypeInfo.sBaseInfo.eKind, rg3::cpp::TypeKind::TK_TRIVIAL);
	ASSERT_EQ(asClass0->getProperties()[1].sTypeInfo.sTypeRef.getRefName(), "float");
	ASSERT_EQ(asClass0->getProperties()[1].vTags.hasTag("property"), true);
	ASSERT_EQ(asClass0->getProperties()[2].sName, "z");
	ASSERT_EQ(asClass0->getProperties()[2].sAlias, "z");
	ASSERT_EQ(asClass0->getProperties()[2].sTypeInfo.sBaseInfo.sName, "float");
	ASSERT_EQ(asClass0->getProperties()[2].sTypeInfo.sBaseInfo.sPrettyName, "float");
	ASSERT_EQ(asClass0->getProperties()[2].sTypeInfo.sBaseInfo.eKind, rg3::cpp::TypeKind::TK_TRIVIAL);
	ASSERT_EQ(asClass0->getProperties()[2].sTypeInfo.sTypeRef.getRefName(), "float");
	ASSERT_EQ(asClass0->getProperties()[2].vTags.hasTag("property"), true);

	ASSERT_EQ(asClass0->getFunctions().size(), 1);
	ASSERT_EQ(asClass0->getFunctions()[0].sOwnerClassName, "engine::core::Vec3<float>");
	ASSERT_EQ(asClass0->getFunctions()[0].sReturnType.sTypeRef.getRefName(), "float");
	ASSERT_EQ(asClass0->getFunctions()[0].sReturnType.sBaseInfo.sName, "float");
	ASSERT_EQ(asClass0->getFunctions()[0].sReturnType.sBaseInfo.eKind, rg3::cpp::TypeKind::TK_TRIVIAL);
	ASSERT_EQ(asClass0->getFunctions()[0].sReturnType.sBaseInfo.sPrettyName, "float");
	ASSERT_EQ(asClass0->getFunctions()[0].bIsConst, true);
	ASSERT_EQ(asClass0->getFunctions()[0].vArguments.size(), 1);
	ASSERT_EQ(asClass0->getFunctions()[0].vArguments[0].bHasDefaultValue, false);
	ASSERT_EQ(asClass0->getFunctions()[0].vArguments[0].sArgumentName, "");
	ASSERT_EQ(asClass0->getFunctions()[0].vArguments[0].sType.sTypeRef.getRefName(), "float");
	ASSERT_EQ(asClass0->getFunctions()[0].vArguments[0].sType.bIsPtrConst, true);
	ASSERT_EQ(asClass0->getFunctions()[0].vArguments[0].sType.bIsReference, true);
	ASSERT_EQ(asClass0->getFunctions()[0].vArguments[0].sType.sBaseInfo.sName, "float");
	ASSERT_EQ(asClass0->getFunctions()[0].vArguments[0].sType.sBaseInfo.sPrettyName, "float");
	ASSERT_EQ(asClass0->getFunctions()[0].vArguments[0].sType.sBaseInfo.eKind, rg3::cpp::TypeKind::TK_TRIVIAL);

	// Second type
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getKind(), rg3::cpp::TypeKind::TK_STRUCT_OR_CLASS);
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getName(), "Vec3<int>");
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getPrettyName(), "engine::core::Vec3<int>");
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->isForwardDeclarable(), false) << "Non forward declarable (template spec)";
	auto* asClass1 = reinterpret_cast<rg3::cpp::TypeClass*>(analyzeResult.vFoundTypes[1].get());

	ASSERT_EQ(asClass1->getProperties().size(), 3);
	ASSERT_EQ(asClass1->getProperties()[0].sName, "x");
	ASSERT_EQ(asClass1->getProperties()[0].sAlias, "x");
	ASSERT_EQ(asClass1->getProperties()[0].sTypeInfo.sTypeRef.getRefName(), "int");
	ASSERT_EQ(asClass1->getProperties()[0].sTypeInfo.sBaseInfo.sName, "int");
	ASSERT_EQ(asClass1->getProperties()[0].sTypeInfo.sBaseInfo.sPrettyName, "int");
	ASSERT_EQ(asClass1->getProperties()[0].sTypeInfo.sBaseInfo.eKind, rg3::cpp::TypeKind::TK_TRIVIAL);
	ASSERT_EQ(asClass1->getProperties()[0].sTypeInfo.sTypeRef.getRefName(), "int");
	ASSERT_EQ(asClass0->getProperties()[0].vTags.hasTag("property"), true);
	ASSERT_EQ(asClass1->getProperties()[1].sName, "y");
	ASSERT_EQ(asClass1->getProperties()[1].sAlias, "y");
	ASSERT_EQ(asClass1->getProperties()[1].sTypeInfo.sTypeRef.getRefName(), "int");
	ASSERT_EQ(asClass1->getProperties()[1].sTypeInfo.sBaseInfo.sName, "int");
	ASSERT_EQ(asClass1->getProperties()[1].sTypeInfo.sBaseInfo.sPrettyName, "int");
	ASSERT_EQ(asClass1->getProperties()[1].sTypeInfo.sBaseInfo.eKind, rg3::cpp::TypeKind::TK_TRIVIAL);
	ASSERT_EQ(asClass1->getProperties()[1].sTypeInfo.sTypeRef.getRefName(), "int");
	ASSERT_EQ(asClass0->getProperties()[1].vTags.hasTag("property"), true);
	ASSERT_EQ(asClass1->getProperties()[2].sName, "z");
	ASSERT_EQ(asClass1->getProperties()[2].sAlias, "z");
	ASSERT_EQ(asClass1->getProperties()[2].sTypeInfo.sTypeRef.getRefName(), "int");
	ASSERT_EQ(asClass1->getProperties()[2].sTypeInfo.sBaseInfo.sName, "int");
	ASSERT_EQ(asClass1->getProperties()[2].sTypeInfo.sBaseInfo.sPrettyName, "int");
	ASSERT_EQ(asClass1->getProperties()[2].sTypeInfo.sBaseInfo.eKind, rg3::cpp::TypeKind::TK_TRIVIAL);
	ASSERT_EQ(asClass1->getProperties()[2].sTypeInfo.sTypeRef.getRefName(), "int");
	ASSERT_EQ(asClass0->getProperties()[2].vTags.hasTag("property"), true);

	ASSERT_EQ(asClass1->getFunctions().size(), 1);
	ASSERT_EQ(asClass1->getFunctions()[0].sOwnerClassName, "engine::core::Vec3<int>");
	ASSERT_EQ(asClass1->getFunctions()[0].sReturnType.sTypeRef.getRefName(), "int");
	ASSERT_EQ(asClass1->getFunctions()[0].bIsConst, true);
	ASSERT_EQ(asClass1->getFunctions()[0].vArguments.size(), 1);
	ASSERT_EQ(asClass1->getFunctions()[0].vArguments[0].bHasDefaultValue, false);
	ASSERT_EQ(asClass1->getFunctions()[0].vArguments[0].sArgumentName, "");
	ASSERT_EQ(asClass1->getFunctions()[0].vArguments[0].sType.sTypeRef.getRefName(), "int");
	ASSERT_EQ(asClass1->getFunctions()[0].vArguments[0].sType.bIsPtrConst, true);
	ASSERT_EQ(asClass1->getFunctions()[0].vArguments[0].sType.bIsReference, true);
	ASSERT_EQ(asClass1->getFunctions()[0].vArguments[0].sType.sBaseInfo.sName, "int");
	ASSERT_EQ(asClass1->getFunctions()[0].vArguments[0].sType.sBaseInfo.sPrettyName, "int");
	ASSERT_EQ(asClass1->getFunctions()[0].vArguments[0].sType.sBaseInfo.eKind, rg3::cpp::TypeKind::TK_TRIVIAL);
}

TEST_F(Tests_MemberFunctions, CheckMemberPropertyTypeReferenceForm)
{
	g_Analyzer->setSourceCode(R"(
namespace cool::name::space::at::all {
	/// @runtime
	struct Vector3
	{ // doesn't matter
	};

	template <typename T>
	struct TVector2
	{
		T x;
		T y;
	};

	namespace rg3 {
		template <typename T> struct RegisterType {};
		template <> struct
			__attribute__((annotate("RG3_RegisterRuntime")))
			__attribute__((annotate("RG3_RegisterField[x]")))
			__attribute__((annotate("RG3_RegisterField[y]")))
		RegisterType<cool::name::space::at::all::TVector2<float>> {
			using Type = cool::name::space::at::all::TVector2<float>;
		};
	}
}

using namespace cool::name::space::at::all; // to avoid of full form later

namespace engine::render
{
	using Vec2I = TVector2<int>;
}

using namespace engine::render;

namespace engine {
	/// @runtime
	struct TransformComponent
	{
		/// @property(vPos)
		const Vector3 position{};

		/// @property(vDir)
		Vector3 direction{};

		/// @property(vUP)
		TVector2<float> up {};

		/// @property(vTex0UV)
		Vec2I texUV {};
	};
}
)");

	g_Analyzer->getCompilerConfig().cppStandard = rg3::llvm::CxxStandard::CC_20;

	const auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(analyzeResult.vIssues.empty()) << "No issues should be here";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 3) << "Only 3 type should be here";

	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getPrettyName(), "cool::name::space::at::all::Vector3");
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getKind(), rg3::cpp::TypeKind::TK_STRUCT_OR_CLASS);

	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getPrettyName(), "cool::name::space::at::all::TVector2<float>");
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getKind(), rg3::cpp::TypeKind::TK_STRUCT_OR_CLASS);
	auto* asClass0 = reinterpret_cast<const rg3::cpp::TypeClass*>(analyzeResult.vFoundTypes[1].get());
	ASSERT_EQ(asClass0->getProperties().size(), 2);
	ASSERT_EQ(asClass0->getProperties()[0].sName, "x");
	ASSERT_EQ(asClass0->getProperties()[0].sAlias, "x");
	ASSERT_EQ(asClass0->getProperties()[0].sTypeInfo.sBaseInfo.sName, "float");
	ASSERT_EQ(asClass0->getProperties()[0].sTypeInfo.sBaseInfo.eKind, rg3::cpp::TypeKind::TK_TRIVIAL);
	ASSERT_EQ(asClass0->getProperties()[0].sTypeInfo.sBaseInfo.sDefLocation.isAngledPath(), false);
	ASSERT_EQ(asClass0->getProperties()[0].sTypeInfo.sBaseInfo.sDefLocation.getPath().empty(), true);
	ASSERT_EQ(asClass0->getProperties()[1].sName, "y");
	ASSERT_EQ(asClass0->getProperties()[1].sAlias, "y");
	ASSERT_EQ(asClass0->getProperties()[1].sTypeInfo.sBaseInfo.sName, "float");
	ASSERT_EQ(asClass0->getProperties()[1].sTypeInfo.sBaseInfo.eKind, rg3::cpp::TypeKind::TK_TRIVIAL);
	ASSERT_EQ(asClass0->getProperties()[1].sTypeInfo.sBaseInfo.sDefLocation.isAngledPath(), false);
	ASSERT_EQ(asClass0->getProperties()[1].sTypeInfo.sBaseInfo.sDefLocation.getPath().empty(), true);

	ASSERT_EQ(analyzeResult.vFoundTypes[2]->getPrettyName(), "engine::TransformComponent");
	ASSERT_EQ(analyzeResult.vFoundTypes[2]->getKind(), rg3::cpp::TypeKind::TK_STRUCT_OR_CLASS);

	auto* asClass = reinterpret_cast<const rg3::cpp::TypeClass*>(analyzeResult.vFoundTypes[2].get());
	ASSERT_EQ(asClass->getProperties().size(), 4);

	ASSERT_EQ(asClass->getProperties()[0].sName, "position");
	ASSERT_EQ(asClass->getProperties()[0].sAlias, "vPos");
	ASSERT_EQ(asClass->getProperties()[0].sTypeInfo.sTypeRef.getRefName(), "cool::name::space::at::all::Vector3");
	ASSERT_EQ(asClass->getProperties()[0].sTypeInfo.sBaseInfo.sName, "Vector3");
	ASSERT_EQ(asClass->getProperties()[0].sTypeInfo.sBaseInfo.sPrettyName, "cool::name::space::at::all::Vector3");
	ASSERT_EQ(asClass->getProperties()[0].sTypeInfo.sBaseInfo.eKind, rg3::cpp::TypeKind::TK_STRUCT_OR_CLASS);
	ASSERT_EQ(asClass->getProperties()[0].sTypeInfo.sBaseInfo.sDefLocation.getPath(), "id0.hpp");

	ASSERT_EQ(asClass->getProperties()[1].sName, "direction");
	ASSERT_EQ(asClass->getProperties()[1].sAlias, "vDir");
	ASSERT_EQ(asClass->getProperties()[1].sTypeInfo.sTypeRef.getRefName(), "cool::name::space::at::all::Vector3");
	ASSERT_EQ(asClass->getProperties()[1].sTypeInfo.sBaseInfo.sName, "Vector3");
	ASSERT_EQ(asClass->getProperties()[1].sTypeInfo.sBaseInfo.sPrettyName, "cool::name::space::at::all::Vector3");
	ASSERT_EQ(asClass->getProperties()[1].sTypeInfo.sBaseInfo.eKind, rg3::cpp::TypeKind::TK_STRUCT_OR_CLASS);
	ASSERT_EQ(asClass->getProperties()[1].sTypeInfo.sBaseInfo.sDefLocation.getPath(), "id0.hpp");

	ASSERT_EQ(asClass->getProperties()[2].sName, "up");
	ASSERT_EQ(asClass->getProperties()[2].sAlias, "vUP");
	ASSERT_EQ(asClass->getProperties()[2].sTypeInfo.sTypeRef.getRefName(), "cool::name::space::at::all::TVector2<float>");
	ASSERT_EQ(asClass->getProperties()[2].sTypeInfo.sBaseInfo.sName, "TVector2<float>");
	ASSERT_EQ(asClass->getProperties()[2].sTypeInfo.sBaseInfo.sPrettyName, "cool::name::space::at::all::TVector2<float>");
	ASSERT_EQ(asClass->getProperties()[2].sTypeInfo.sBaseInfo.eKind, rg3::cpp::TypeKind::TK_STRUCT_OR_CLASS);
	ASSERT_EQ(asClass->getProperties()[2].sTypeInfo.sBaseInfo.sDefLocation.getPath(), "id0.hpp");

	ASSERT_EQ(asClass->getProperties()[3].sName, "texUV");
	ASSERT_EQ(asClass->getProperties()[3].sAlias, "vTex0UV");
	ASSERT_EQ(asClass->getProperties()[3].sTypeInfo.sTypeRef.getRefName(), "engine::render::Vec2I");
	ASSERT_EQ(asClass->getProperties()[3].sTypeInfo.sBaseInfo.sName, "Vec2I");
	ASSERT_EQ(asClass->getProperties()[3].sTypeInfo.sBaseInfo.sPrettyName, "engine::render::Vec2I");
	ASSERT_EQ(asClass->getProperties()[3].sTypeInfo.sBaseInfo.eKind, rg3::cpp::TypeKind::TK_STRUCT_OR_CLASS);
	ASSERT_EQ(asClass->getProperties()[3].sTypeInfo.sBaseInfo.sDefLocation.getPath(), "id0.hpp");
}
