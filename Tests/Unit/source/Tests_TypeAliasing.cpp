#include <gtest/gtest.h>

#include <RG3/Cpp/Tag.h>
#include <RG3/Cpp/TypeBase.h>
#include <RG3/Cpp/TypeClass.h>
#include <RG3/Cpp/TypeAlias.h>
#include <RG3/LLVM/CodeAnalyzer.h>


class Tests_TypeAliasing : public ::testing::Test
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

TEST_F(Tests_TypeAliasing, CheckTrivialTypeAliasing)
{
	g_Analyzer->setSourceCode(R"(
template <typename T> struct FakeT;

namespace gg {
	/// @runtime
	using f32 = float;

	/// @runtime
	using f64 = double;

	/// @runtime
	using i8 = char;

	/// @runtime
	using u8 = unsigned char;

	/// @runtime
	using i16 = short;

	/// @runtime
	using u16 = unsigned short;

	/// @runtime
	using i32 = int;

	/// @runtime
	using u32 = unsigned int;

	/// @runtime
	using bytes = u8*;

	/// @runtime
	using c_bytes = const bytes;

	/// @runtime
	using cc_bytes = const u8* const;

	/// @runtime
	using i32_fake = FakeT<int>;
}
)");

	g_Analyzer->getCompilerConfig().cppStandard = rg3::llvm::CxxStandard::CC_17;

	const auto analyzeResult = g_Analyzer->analyze();

	// expected to have 8 aliases
	ASSERT_TRUE(analyzeResult.vIssues.empty()) << "No issues should be here";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 12) << "Only 12 type should be here";

	{
		ASSERT_EQ(analyzeResult.vFoundTypes[0]->getKind(), rg3::cpp::TypeKind::TK_ALIAS);
		ASSERT_EQ(analyzeResult.vFoundTypes[0]->getName(), "f32");
		ASSERT_EQ(analyzeResult.vFoundTypes[0]->getPrettyName(), "gg::f32");

		auto asAlias = static_cast<const rg3::cpp::TypeAlias*>(analyzeResult.vFoundTypes[0].get()); // NOLINT(*-pro-type-static-cast-downcast)
		ASSERT_EQ(asAlias->getTargetType().getRefName(), "float");
	}

	{
		ASSERT_EQ(analyzeResult.vFoundTypes[1]->getKind(), rg3::cpp::TypeKind::TK_ALIAS);
		ASSERT_EQ(analyzeResult.vFoundTypes[1]->getName(), "f64");
		ASSERT_EQ(analyzeResult.vFoundTypes[1]->getPrettyName(), "gg::f64");

		auto asAlias = static_cast<const rg3::cpp::TypeAlias*>(analyzeResult.vFoundTypes[1].get()); // NOLINT(*-pro-type-static-cast-downcast)
		ASSERT_EQ(asAlias->getTargetType().getRefName(), "double");
	}

	{
		ASSERT_EQ(analyzeResult.vFoundTypes[2]->getKind(), rg3::cpp::TypeKind::TK_ALIAS);
		ASSERT_EQ(analyzeResult.vFoundTypes[2]->getName(), "i8");
		ASSERT_EQ(analyzeResult.vFoundTypes[2]->getPrettyName(), "gg::i8");

		auto asAlias = static_cast<const rg3::cpp::TypeAlias*>(analyzeResult.vFoundTypes[2].get()); // NOLINT(*-pro-type-static-cast-downcast)
		ASSERT_EQ(asAlias->getTargetType().getRefName(), "char");
	}

	{
		ASSERT_EQ(analyzeResult.vFoundTypes[3]->getKind(), rg3::cpp::TypeKind::TK_ALIAS);
		ASSERT_EQ(analyzeResult.vFoundTypes[3]->getName(), "u8");
		ASSERT_EQ(analyzeResult.vFoundTypes[3]->getPrettyName(), "gg::u8");

		auto asAlias = static_cast<const rg3::cpp::TypeAlias*>(analyzeResult.vFoundTypes[3].get()); // NOLINT(*-pro-type-static-cast-downcast)
		ASSERT_EQ(asAlias->getTargetType().getRefName(), "unsigned char");
	}

	{
		ASSERT_EQ(analyzeResult.vFoundTypes[4]->getKind(), rg3::cpp::TypeKind::TK_ALIAS);
		ASSERT_EQ(analyzeResult.vFoundTypes[4]->getName(), "i16");
		ASSERT_EQ(analyzeResult.vFoundTypes[4]->getPrettyName(), "gg::i16");

		auto asAlias = static_cast<const rg3::cpp::TypeAlias*>(analyzeResult.vFoundTypes[4].get()); // NOLINT(*-pro-type-static-cast-downcast)
		ASSERT_EQ(asAlias->getTargetType().getRefName(), "short");
	}

	{
		ASSERT_EQ(analyzeResult.vFoundTypes[5]->getKind(), rg3::cpp::TypeKind::TK_ALIAS);
		ASSERT_EQ(analyzeResult.vFoundTypes[5]->getName(), "u16");
		ASSERT_EQ(analyzeResult.vFoundTypes[5]->getPrettyName(), "gg::u16");

		auto asAlias = static_cast<const rg3::cpp::TypeAlias*>(analyzeResult.vFoundTypes[5].get()); // NOLINT(*-pro-type-static-cast-downcast)
		ASSERT_EQ(asAlias->getTargetType().getRefName(), "unsigned short");
	}

	{
		ASSERT_EQ(analyzeResult.vFoundTypes[6]->getKind(), rg3::cpp::TypeKind::TK_ALIAS);
		ASSERT_EQ(analyzeResult.vFoundTypes[6]->getName(), "i32");
		ASSERT_EQ(analyzeResult.vFoundTypes[6]->getPrettyName(), "gg::i32");

		auto asAlias = static_cast<const rg3::cpp::TypeAlias*>(analyzeResult.vFoundTypes[6].get()); // NOLINT(*-pro-type-static-cast-downcast)
		ASSERT_EQ(asAlias->getTargetType().getRefName(), "int");
	}

	{
		ASSERT_EQ(analyzeResult.vFoundTypes[7]->getKind(), rg3::cpp::TypeKind::TK_ALIAS);
		ASSERT_EQ(analyzeResult.vFoundTypes[7]->getName(), "u32");
		ASSERT_EQ(analyzeResult.vFoundTypes[7]->getPrettyName(), "gg::u32");

		auto asAlias = static_cast<const rg3::cpp::TypeAlias*>(analyzeResult.vFoundTypes[7].get()); // NOLINT(*-pro-type-static-cast-downcast)
		ASSERT_EQ(asAlias->getTargetType().getRefName(), "unsigned int");
	}

	{
		ASSERT_EQ(analyzeResult.vFoundTypes[8]->getKind(), rg3::cpp::TypeKind::TK_ALIAS);
		ASSERT_EQ(analyzeResult.vFoundTypes[8]->getName(), "bytes");
		ASSERT_EQ(analyzeResult.vFoundTypes[8]->getPrettyName(), "gg::bytes");

		auto asAlias = static_cast<const rg3::cpp::TypeAlias*>(analyzeResult.vFoundTypes[8].get()); // NOLINT(*-pro-type-static-cast-downcast)
		ASSERT_EQ(asAlias->getTargetType().getRefName(), "unsigned char");
		ASSERT_EQ(asAlias->getTargetTypeDescription().bIsConst, false);
		ASSERT_EQ(asAlias->getTargetTypeDescription().bIsPointer, true);
		ASSERT_EQ(asAlias->getTargetTypeDescription().bIsReference, false);
		ASSERT_EQ(asAlias->getTargetTypeDescription().bIsPtrConst, false);
		ASSERT_EQ(asAlias->getTargetTypeDescription().bIsTemplateSpecialization, false);
	}

	{
		ASSERT_EQ(analyzeResult.vFoundTypes[9]->getKind(), rg3::cpp::TypeKind::TK_ALIAS);
		ASSERT_EQ(analyzeResult.vFoundTypes[9]->getName(), "c_bytes");
		ASSERT_EQ(analyzeResult.vFoundTypes[9]->getPrettyName(), "gg::c_bytes");

		auto asAlias = static_cast<const rg3::cpp::TypeAlias*>(analyzeResult.vFoundTypes[9].get()); // NOLINT(*-pro-type-static-cast-downcast)
		ASSERT_EQ(asAlias->getTargetType().getRefName(), "unsigned char");
		ASSERT_EQ(asAlias->getTargetTypeDescription().bIsConst, true); // Actually, it's because in alias const applied to parent type
		ASSERT_EQ(asAlias->getTargetTypeDescription().bIsPointer, true);
		ASSERT_EQ(asAlias->getTargetTypeDescription().bIsReference, false);
		ASSERT_EQ(asAlias->getTargetTypeDescription().bIsPtrConst, false);
		ASSERT_EQ(asAlias->getTargetTypeDescription().bIsTemplateSpecialization, false);
	}

	{
		ASSERT_EQ(analyzeResult.vFoundTypes[10]->getKind(), rg3::cpp::TypeKind::TK_ALIAS);
		ASSERT_EQ(analyzeResult.vFoundTypes[10]->getName(), "cc_bytes");
		ASSERT_EQ(analyzeResult.vFoundTypes[10]->getPrettyName(), "gg::cc_bytes");

		auto asAlias = static_cast<const rg3::cpp::TypeAlias*>(analyzeResult.vFoundTypes[10].get()); // NOLINT(*-pro-type-static-cast-downcast)
		ASSERT_EQ(asAlias->getTargetType().getRefName(), "unsigned char");
		ASSERT_EQ(asAlias->getTargetTypeDescription().bIsConst, true);
		ASSERT_EQ(asAlias->getTargetTypeDescription().bIsPointer, true);
		ASSERT_EQ(asAlias->getTargetTypeDescription().bIsReference, false);
		ASSERT_EQ(asAlias->getTargetTypeDescription().bIsPtrConst, true);
		ASSERT_EQ(asAlias->getTargetTypeDescription().bIsTemplateSpecialization, false);
	}

	{
		ASSERT_EQ(analyzeResult.vFoundTypes[11]->getKind(), rg3::cpp::TypeKind::TK_ALIAS);
		ASSERT_EQ(analyzeResult.vFoundTypes[11]->getName(), "i32_fake");
		ASSERT_EQ(analyzeResult.vFoundTypes[11]->getPrettyName(), "gg::i32_fake");

		auto asAlias = static_cast<const rg3::cpp::TypeAlias*>(analyzeResult.vFoundTypes[11].get()); // NOLINT(*-pro-type-static-cast-downcast)
		ASSERT_EQ(asAlias->getTargetType().getRefName(), "FakeT<int>");
		ASSERT_EQ(asAlias->getTargetTypeDescription().bIsTemplateSpecialization, true);
		ASSERT_EQ(asAlias->getTargetTypeDescription().bIsConst, false);
		ASSERT_EQ(asAlias->getTargetTypeDescription().bIsPointer, false);
		ASSERT_EQ(asAlias->getTargetTypeDescription().bIsReference, false);
		ASSERT_EQ(asAlias->getTargetTypeDefinedAt().getPath(), "id0.hpp");
	}
}

TEST_F(Tests_TypeAliasing, CheckGlobalAliasesToTrivialTypes)
{
	g_Analyzer->setSourceCode(R"(
namespace c_ai {
	/// @runtime
	typedef unsigned int MyGreatInt;
}

/// @runtime
using MyFancyType = c_ai::MyGreatInt*;
)");

	g_Analyzer->getCompilerConfig().cppStandard = rg3::llvm::CxxStandard::CC_17;

	const auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(analyzeResult.vIssues.empty()) << "No issues should be here";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 2) << "Only 2 type should be here";

	// Expected to have here 2 aliases: first to unsigned int, and second to unsigned int. It's because type alias extracted before save.
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getKind(), rg3::cpp::TypeKind::TK_ALIAS);
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getName(), "MyGreatInt");
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getPrettyName(), "c_ai::MyGreatInt");
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getDefinition().getPath(), "id0.hpp"); // but alias always known
	ASSERT_NE(analyzeResult.vFoundTypes[0]->getDefinition().getLine(), 0);

	auto alias0 = static_cast<const rg3::cpp::TypeAlias*>(analyzeResult.vFoundTypes[0].get()); // NOLINT(*-pro-type-static-cast-downcast)
	ASSERT_EQ(alias0->getTargetType().getRefName(), "unsigned int");
	ASSERT_EQ(alias0->getTargetTypeDefinedAt().getPath(), ""); // because trivial types has no definition
	ASSERT_EQ(alias0->getTargetTypeDescription().bIsTemplateSpecialization, false);
	ASSERT_EQ(alias0->getTargetTypeDescription().bIsPtrConst, false);
	ASSERT_EQ(alias0->getTargetTypeDescription().bIsPointer, false);
	ASSERT_EQ(alias0->getTargetTypeDescription().bIsConst, false);
	ASSERT_EQ(alias0->getTargetTypeDescription().bIsReference, false);

	auto alias1 = static_cast<const rg3::cpp::TypeAlias*>(analyzeResult.vFoundTypes[1].get()); // NOLINT(*-pro-type-static-cast-downcast)
	ASSERT_EQ(alias1->getTargetType().getRefName(), "unsigned int");
	ASSERT_EQ(alias1->getTargetTypeDefinedAt().getPath(), ""); // because trivial types has no definition
	ASSERT_EQ(alias1->getTargetTypeDescription().bIsTemplateSpecialization, false);
	ASSERT_EQ(alias1->getTargetTypeDescription().bIsPtrConst, false);
	ASSERT_EQ(alias1->getTargetTypeDescription().bIsPointer, true);
	ASSERT_EQ(alias1->getTargetTypeDescription().bIsConst, false);
	ASSERT_EQ(alias1->getTargetTypeDescription().bIsReference, false);
}

TEST_F(Tests_TypeAliasing, CheckGlobalAliasToUserType)
{
	g_Analyzer->setSourceCode(R"(
namespace engine {
	/// @runtime
	struct CustomStruct
	{};
}

namespace user {
	/// @runtime
	using CustomAlias = const engine::CustomStruct* const;
}
)");

	g_Analyzer->getCompilerConfig().cppStandard = rg3::llvm::CxxStandard::CC_17;

	const auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(analyzeResult.vIssues.empty()) << "No issues should be here";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 2) << "Only 2 type should be here";

	// Check first type - struct in namespace 'engine' with name 'CustomStruct' (is_class = false, is_struct = true)
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getName(), "CustomStruct");
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getPrettyName(), "engine::CustomStruct");
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getKind(), rg3::cpp::TypeKind::TK_STRUCT_OR_CLASS);

	auto class0 = static_cast<const rg3::cpp::TypeClass*>(analyzeResult.vFoundTypes[0].get()); // NOLINT(*-pro-type-static-cast-downcast)
	ASSERT_EQ(class0->getFunctions().size(), 0);
	ASSERT_EQ(class0->getProperties().size(), 0);
	ASSERT_TRUE(class0->isStruct());
	ASSERT_EQ(class0->getDefinition().getPath(), "id0.hpp");
	ASSERT_NE(class0->getDefinition().getLine(), 0); // idk which line is correct, so it's more than 0!

	// Check second type - alias to 'engine::CustomStruct' with fully known name, location, type stmt and etc
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getName(), "CustomAlias");
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getPrettyName(), "user::CustomAlias");
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getDefinition().getPath(), "id0.hpp");
	ASSERT_NE(analyzeResult.vFoundTypes[1]->getDefinition().getLine(), 0);

	auto alias0 = static_cast<const rg3::cpp::TypeAlias*>(analyzeResult.vFoundTypes[1].get()); // NOLINT(*-pro-type-static-cast-downcast)
	ASSERT_NE(alias0->getTargetTypeDefinedAt().getLine(), 0);
	ASSERT_EQ(alias0->getTargetTypeDefinedAt().getPath(), "id0.hpp");
	ASSERT_EQ(alias0->getTargetType().getRefName(), "engine::CustomStruct");
	ASSERT_EQ(alias0->getTargetTypeDescription().bIsConst, true);
	ASSERT_EQ(alias0->getTargetTypeDescription().bIsPointer, true);
	ASSERT_EQ(alias0->getTargetTypeDescription().bIsPtrConst, true);
	ASSERT_EQ(alias0->getTargetTypeDescription().bIsReference, false);
	ASSERT_EQ(alias0->getTargetTypeDescription().bIsTemplateSpecialization, false);
}

TEST_F(Tests_TypeAliasing, CheckInClassTypeAlias)
{
	g_Analyzer->setSourceCode(R"(
namespace not_std {
	template <typename T> struct shared_ptr {}; // do stuff
}

namespace engine {
	/// @runtime
	struct MyComponent
	{
		/// @runtime
		using Ptr = not_std::shared_ptr<MyComponent>;
	};
}
)");

	g_Analyzer->getCompilerConfig().cppStandard = rg3::llvm::CxxStandard::CC_17;

	const auto analyzeResult = g_Analyzer->analyze();

	ASSERT_TRUE(analyzeResult.vIssues.empty()) << "No issues should be here";
	ASSERT_EQ(analyzeResult.vFoundTypes.size(), 2) << "Only 2 type should be here";

	// First type - trivial struct
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getName(), "MyComponent");
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getKind(), rg3::cpp::TypeKind::TK_STRUCT_OR_CLASS);
	ASSERT_EQ(analyzeResult.vFoundTypes[0]->getPrettyName(), "engine::MyComponent");

	// Second type - type alias inside another type
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getName(), "Ptr");
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getKind(), rg3::cpp::TypeKind::TK_ALIAS);
	ASSERT_EQ(analyzeResult.vFoundTypes[1]->getPrettyName(), "engine::MyComponent::Ptr");

	auto asAlias = static_cast<const rg3::cpp::TypeAlias*>(analyzeResult.vFoundTypes[1].get()); // NOLINT(*-pro-type-static-cast-downcast)
	ASSERT_EQ(asAlias->getTargetType().getRefName(), "not_std::shared_ptr<engine::MyComponent>");
	ASSERT_EQ(asAlias->getTargetTypeDefinedAt().getPath(), "id0.hpp");
}