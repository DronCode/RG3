#include <RG3/LLVM/Annotations.h>
#include <RG3/Cpp/BuiltinTags.h>
#include <clang/AST/Attr.h>
#include <string_view>
#include <boost/algorithm/string.hpp>


namespace rg3::llvm
{
	static void eraseDeclaration(std::vector<std::string>& v, std::string_view sSelfDecl)
	{
		for (auto it = v.begin(); it != v.end(); )
		{
			if (it->empty() || (*it) == sSelfDecl)
			{
				it = v.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	void Annotations::collectFromDecl(clang::Decl* pDecl)
	{
		bool bHasAnnotateAttribute = pDecl->hasAttr<clang::AnnotateAttr>();
		if (!bHasAnnotateAttribute)
			return;

		for (clang::Attr* pAttr : pDecl->attrs())
		{
			if (clang::AnnotateAttr* pAnnotate = ::llvm::dyn_cast<clang::AnnotateAttr>(pAttr))
			{
				std::string_view sAnnotationItself { pAnnotate->getAnnotation() };

				if (sAnnotationItself == cpp::BuiltinAnnotations::kRegisterRuntime)
				{
					bIsRuntime = true;
				}
				if (sAnnotationItself.starts_with(cpp::BuiltinAnnotations::kRegisterField))
				{
					// Here we have annotation of RG3_RegisterField[X:Y] where X - original name, Y - new name.
					// We just need to split string by ':' and remove first char in first string and last char in last string
					std::vector<std::string> splitResult;
					boost::algorithm::split(splitResult, sAnnotationItself, boost::is_any_of("[]:"));

					// First of all we need to remove empty parts or ref itself
					eraseDeclaration(splitResult, cpp::BuiltinAnnotations::kRegisterField);

					if (!splitResult.empty())
					{
						// Nice, ready to save
						PropertyDescription& newProperty = knownProperties.emplace_back();
						newProperty.propertyRefName = splitResult[0];
						newProperty.propertyAliasName = (splitResult.size() == 2) ? splitResult[1] : splitResult[0];
					}
				}
				else if (sAnnotationItself.starts_with(cpp::BuiltinAnnotations::kRegisterFunction))
				{
					// Here we have annotation RG3_RegisterFunction[X] where X - function name to register.
					// NOTE: This part of code can't handle function overloads & other things. It means that code will have UB
					std::vector<std::string> splitResult;
					boost::algorithm::split(splitResult, sAnnotationItself, boost::is_any_of("[]"));

					if (splitResult.size() >= 2)
					{
						knownFunctions.emplace_back(splitResult[1]); // #0 - tag, #1 - func name
					}
				}
				else if (sAnnotationItself.starts_with(cpp::BuiltinAnnotations::kRegisterTag))
				{
					// Here we have annotation of RG3_RegisterTag[X] where X - tag declaration.
					std::vector<std::string> splitResult;
					boost::algorithm::split(splitResult, sAnnotationItself, boost::is_any_of("[]"));

					if (splitResult.size() >= 2)
					{
						// Tag contents stored at arg #1
						// splitResult[1]
						const auto tags = cpp::Tag::parseFromCommentString(splitResult[1]);
						for (const auto& [name, tag] : tags.getTags())
						{
							additionalTags.getTags()[name] = tag;
						}
					}
				}
				else if (sAnnotationItself.starts_with(cpp::BuiltinAnnotations::kOverrideLocation))
				{
					// Here we have annotation to override type location
					std::vector<std::string> splitResult;
					boost::algorithm::split(splitResult, sAnnotationItself, boost::is_any_of("[]"));

					if (splitResult.size() >= 2)
					{
						overrideLocation.emplace(splitResult[1]);
					}
				}
				else if (sAnnotationItself == cpp::BuiltinAnnotations::kInterpretTypeAsTrivial)
				{
					bInterpretAsTrivial = true;
				}
			}
		}
	}
}