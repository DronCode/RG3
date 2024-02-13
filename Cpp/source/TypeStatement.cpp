#include <RG3/Cpp/TypeStatement.h>


namespace rg3::cpp
{
	constexpr const char* kVoidNameDecl = "void";

	const TypeStatement TypeStatement::g_sVoid {
		TypeReference(kVoidNameDecl, nullptr),
		std::nullopt,
		false, false, false, false
	};

	bool TypeStatement::isVoid() const
	{
		return sTypeRef.getRefName() == kVoidNameDecl && !bIsPointer;
	}

	bool TypeStatement::operator==(const TypeStatement& other) const
	{
		return sTypeRef                  == other.sTypeRef &&
			   sDefinitionLocation       == other.sDefinitionLocation &&
			   bIsConst                  == other.bIsConst &&
			   bIsPtrConst               == other.bIsPtrConst &&
			   bIsPointer                == other.bIsPointer &&
			   bIsReference              == other.bIsReference &&
			   bIsTemplateSpecialization == other.bIsTemplateSpecialization;
	}

	bool TypeStatement::operator!=(const TypeStatement& other) const
	{
		return !operator==(other);
	}
}