#pragma once

#include <memory>
#include <RG3/Cpp/Tag.h>
#include <RG3/Cpp/TypeID.h>
#include <RG3/Cpp/TypeKind.h>
#include <RG3/Cpp/CppNamespace.h>
#include <RG3/Cpp/DefinitionLocation.h>


namespace rg3::cpp
{
    class TypeBase
    {
    public:
        TypeBase();
		virtual ~TypeBase() noexcept = default;

        TypeBase(TypeKind kind, const std::string& name, const std::string& prettyName, const CppNamespace& aNamespace, const DefinitionLocation& aLocation, const Tags& tags);

		[[nodiscard]] TypeID getID() const;
		[[nodiscard]] TypeKind getKind() const;
		[[nodiscard]] const std::string& getName() const;
		[[nodiscard]] const CppNamespace& getNamespace() const;
		[[nodiscard]] const std::string& getPrettyName() const;
		[[nodiscard]] const DefinitionLocation& getDefinition() const;
		void setDefinition(DefinitionLocation&& newLoc);

		[[nodiscard]] bool areSame(const TypeBase* pOther) const;

		[[nodiscard]] const Tags& getTags() const;
		[[nodiscard]] Tags& getTags();

		[[nodiscard]] bool operator==(const TypeBase& other) const { return  areSame(&other); }
		[[nodiscard]] bool operator!=(const TypeBase& other) const { return !areSame(&other); }

	 protected:
		virtual bool doAreSame(const TypeBase* pOther) const;

	 private:
		TypeKind m_kind { TypeKind::TK_NONE };
		std::string m_name;  ///< Name of type without namespaces and parent types
		std::string m_prettyName;  ///< "prettified" name contains full decl
		CppNamespace m_nameSpace;
		DefinitionLocation m_location;
		Tags m_tags;
    };

	using TypeBasePtr = std::unique_ptr<TypeBase>;
}