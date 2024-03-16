#pragma once

#include <string_view>


namespace rg3::cpp
{
	struct BuiltinTags
	{
		/**
		* Runtime - means that type should be processed by us. This is not a single way to have this type processed.
		* When runtime type refs to non-runtime type that type may be 'runtimed' for RG3 tool.
		*/
		static constexpr std::string_view kRuntime { "runtime" };

		/**
		 * Brief - a "doxygen way" to know annotation about entity.
		 */
		static constexpr std::string_view kBrief { "brief" };

		/**
		 * Property - special tag to override property name. Usually, it has 1 argument - alias for property name
		 */
		static constexpr std::string_view kProperty { "property" };
	};
}