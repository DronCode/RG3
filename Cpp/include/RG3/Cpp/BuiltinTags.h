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

	struct BuiltinAnnotations
	{
		/**
		 * @brief Annotation for type which could not be annotated via runtime tag (or any other tag)
		 */
		static constexpr std::string_view kRegisterRuntime { "RG3_RegisterRuntime" };

		/**
		 * @brief Annotation for type fields. Semantics: RG3_RegisterField[{original_name}:{alias_name}]
		 * 	Original name: name of field without class name
		 * 	Alias name: [Optional] new name of field. Must be unique for type
		 */
		static constexpr std::string_view kRegisterField { "RG3_RegisterField" };

		/**
		 * @brief Annotation for type function registration. Semantics: RG3_RegisterFunction[funcName]
		 * @note Aliasing not supported
		 */
		static constexpr std::string_view kRegisterFunction { "RG3_RegisterFunction" };

		/**
		 * @brief Annotation for extra tag for type. Semantics: RG3_RegisterTag[@tagName(args)]
		 * @note Use full semantic because will be used default parser!
		 * @note Use string escape symbol \" to use strings inside tags inside annotation
		 */
		static constexpr std::string_view kRegisterTag { "RG3_RegisterTag" };

		/**
		 * @brief Annotation for override final type location place. Semantics: RG3_OverrideLication[new/path]
		 * @note Path will be marked as 'angled' ie to use as #include <my_path>
		 * @note Line & column will be always zeroed
		 * @note Only last annotation will affect final path!
		 */
		static constexpr std::string_view kOverrideLocation { "RG3_OverrideLocation" };

		/**
		 * @brief Override type original kind and mark it as trivial. Loss all extras. Just save only base info about the type
		 * @note  Type will loss all extras.
		 * @note  RG3 can ignore some checks when this annotation declared
		 */
		static constexpr std::string_view kInterpretTypeAsTrivial { "RG3_MakeTypeTrivial" };
	};
}