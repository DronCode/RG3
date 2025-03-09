#pragma once

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>
#include <cstdint>


namespace rg3::pb
{
	struct EnumEntry
	{
		std::string sName {};
		std::int64_t iValue { 0 };
	};

	using EnumEntriesVector = std::vector<EnumEntry>;

	class Enum : public boost::noncopyable
	{
	 public:
		using Ptr = boost::shared_ptr<Enum>;

	 public:
		Enum();
		Enum(const std::string& sName, const std::string& sPackage, const EnumEntriesVector& aEntries);

		[[nodiscard]] const std::string& getName() const;
		[[nodiscard]] const std::string& getPackage() const;
		[[nodiscard]] const EnumEntriesVector& getEntries() const;

	 private:
		std::string m_sName {};
		std::string m_sPackage {};
		EnumEntriesVector m_aEntries {};
	};
}