#include <string>
#include <vector>
#include <variant>
#include <format>

#define BOOST_PYTHON_STATIC_LIB  // required because we using boost.python as static library
#include <boost/python.hpp>
#include <boost/python/dict.hpp>

#include <RG3/Cpp/CppNamespace.h>
#include <RG3/Cpp/TypeClass.h>
#include <RG3/Cpp/TypeEnum.h>

#include <RG3/PyBind/PyCodeAnalyzerBuilder.h>
#include <RG3/PyBind/PyTypeBase.h>
#include <RG3/PyBind/PyTypeEnum.h>
#include <RG3/PyBind/PyTypeClass.h>
#include <RG3/PyBind/PyAnalyzerContext.h>
#include <RG3/PyBind/PyClangRuntime.h>



using namespace boost::python;


namespace rg3::pybind::wrappers
{
	static boost::python::str CppTypeReference_getTypeName(const rg3::cpp::TypeReference& typeRef)
	{
		return boost::python::str(typeRef.getRefName());
	}

	static boost::python::object CppTypeReference_getTypeInfo(const rg3::cpp::TypeReference& typeRef)
	{
		// TODO: Add support of work with internal type
		return boost::python::object();
	}

	static boost::python::list Tag_getArguments(const rg3::cpp::Tag& tag)
	{
		boost::python::list l;

		for (const auto& arg : tag.getArguments())
		{
			l.append(arg);
		}

		return l;
	}

	static boost::python::str TagArgument_getAsStringRepr(const rg3::cpp::TagArgument& arg)
	{
		switch (arg.getHoldedType())
		{
			case rg3::cpp::TagArgumentType::AT_TYPEREF:
			{
				static const rg3::cpp::TypeReference s_None {};
				rg3::cpp::TypeReference rType = arg.asTypeRef(s_None);
				return boost::python::str(std::format("TypeREF: {}", rType.getRefName()));
			}
			break;

			case rg3::cpp::TagArgumentType::AT_BOOL:
			{
				return boost::python::str(arg.asBool(false));
			}
			break;

			case rg3::cpp::TagArgumentType::AT_STRING:
			{
				static const std::string s_None {};
				return boost::python::str(arg.asString(s_None));
			}
			break;

			case rg3::cpp::TagArgumentType::AT_FLOAT:
			{
				return boost::python::str(arg.asFloat(.0f));
			}
			break;

			case rg3::cpp::TagArgumentType::AT_I64:
			{
				return boost::python::str(arg.asI64(0));
			}
			break;

			case rg3::cpp::TagArgumentType::AT_UNDEFINED:
			default:
				return boost::python::str("<UNDEFINED>");
		}

		return boost::python::str("<UNDEFINED>");
	}

	static boost::python::list Tags_getTagItemsList(const rg3::cpp::Tags& tags)
	{
		boost::python::list l;

		for (const auto& [name, tag] : tags.getTags())
		{
			l.append(tag);
		}

		return l;
	}
}


BOOST_PYTHON_MODULE(rg3py)
{
	/**
	 * Register base and trivial types here, other in impls
	 * Please, don't forget to add your public available types into PyBind/rg3py.pyi file!!!
	 */
	/// ----------- ENUMS -----------
	enum_<rg3::cpp::TypeKind>("CppTypeKind")
	    .value("TK_NONE", rg3::cpp::TypeKind::TK_NONE)
	    .value("TK_TRIVIAL", rg3::cpp::TypeKind::TK_TRIVIAL)
	    .value("TK_ENUM", rg3::cpp::TypeKind::TK_ENUM)
	    .value("TK_STRUCT_OR_CLASS", rg3::cpp::TypeKind::TK_STRUCT_OR_CLASS)
	    .value("TK_TEMPLATE_SPECIALIZATION", rg3::cpp::TypeKind::TK_TEMPLATE_SPECIALIZATION)
	;

	enum_<rg3::cpp::TagArgumentType>("TagArgumentType")
	    .value("AT_UNDEFINED", rg3::cpp::TagArgumentType::AT_UNDEFINED)
	    .value("AT_BOOL", rg3::cpp::TagArgumentType::AT_BOOL)
	    .value("AT_FLOAT", rg3::cpp::TagArgumentType::AT_FLOAT)
	    .value("AT_I64", rg3::cpp::TagArgumentType::AT_I64)
	    .value("AT_STRING", rg3::cpp::TagArgumentType::AT_STRING)
	    .value("AT_TYPEREF", rg3::cpp::TagArgumentType::AT_TYPEREF)
	;

	enum_<rg3::cpp::ClassEntryVisibility>("CppClassEntryVisibillity")
	    .value("CEV_PRIVATE", rg3::cpp::ClassEntryVisibility::CEV_PRIVATE)
	    .value("CEV_PROTECTED", rg3::cpp::ClassEntryVisibility::CEV_PROTECTED)
	    .value("CEV_PUBLIC", rg3::cpp::ClassEntryVisibility::CEV_PUBLIC)
	;

	/// ----------- CLASSES -----------
	class_<rg3::cpp::CppNamespace>("CppNamespace")
	    .def(init<std::string>(args("namespace")))
		.def("__eq__", &rg3::cpp::CppNamespace::operator==)
		.def("__ne__", &rg3::cpp::CppNamespace::operator!=)
		.def("__str__", make_function(&rg3::cpp::CppNamespace::asString, return_value_policy<copy_const_reference>()))
		.def("__repr__", make_function(&rg3::cpp::CppNamespace::asString, return_value_policy<copy_const_reference>()))
	;

	class_<rg3::cpp::DefinitionLocation>("Location")
	    .def(init<std::string, int, int>(args("path", "line", "offset")))
		.add_property("path", make_function(&rg3::cpp::DefinitionLocation::getPath, return_value_policy<return_by_value>()))
		.add_property("line", &rg3::cpp::DefinitionLocation::getLine)
		.add_property("column", &rg3::cpp::DefinitionLocation::getInLineOffset)
	;

	class_<rg3::cpp::TagArgument>("TagArgument")
	    .def(init<bool>(arg("barg")))
	    .def(init<float>(arg("farg")))
	    .def(init<std::int64_t>(arg("i64")))
	    .def(init<std::string>(arg("string")))
		.def("__str__", &rg3::pybind::wrappers::TagArgument_getAsStringRepr)
		.def("get_type", &rg3::cpp::TagArgument::getHoldedType, "Returns holded type")
		.def("as_bool", make_function(&rg3::cpp::TagArgument::asBool, return_value_policy<return_by_value>()))
		.def("as_float", make_function(&rg3::cpp::TagArgument::asFloat, return_value_policy<return_by_value>()))
		.def("as_i64", make_function(&rg3::cpp::TagArgument::asI64, return_value_policy<return_by_value>()))
		.def("as_string", make_function(&rg3::cpp::TagArgument::asString, return_value_policy<return_by_value>()))
	;

	class_<rg3::cpp::Tag>("Tag")
	    .add_property("name", make_function(&rg3::cpp::Tag::getName, return_value_policy<copy_const_reference>()))
		.add_property("arguments", make_function(&rg3::pybind::wrappers::Tag_getArguments, return_value_policy<return_by_value>()))
		.def("__eq__", make_function(&rg3::cpp::Tag::operator==, return_value_policy<return_by_value>()))
		.def("__ne__", make_function(&rg3::cpp::Tag::operator!=, return_value_policy<return_by_value>()))
	;

	class_<rg3::cpp::Tags>("Tags")
	    .add_property("items", &rg3::pybind::wrappers::Tags_getTagItemsList, "List of tags inside this registry")
		.def("__contains__", &rg3::cpp::Tags::hasTag)
		.def("has_tag", &rg3::cpp::Tags::hasTag)
		.def("get_tag", make_function(&rg3::cpp::Tags::getTag, return_value_policy<return_by_value>()))
	;

	class_<rg3::cpp::TypeReference>("CppTypeReference")
		.add_property("name", &rg3::pybind::wrappers::CppTypeReference_getTypeName)
		.add_property("info", &rg3::pybind::wrappers::CppTypeReference_getTypeInfo)
	;

	class_<rg3::cpp::EnumEntry>("CppEnumEntry")
		.add_property("name", make_getter(&rg3::cpp::EnumEntry::sName), "Name of entry")
		.add_property("value", make_getter(&rg3::cpp::EnumEntry::iValue), "Value of entry")
	;

	// TODO: Need create wrapper type PyClassProperty
	class_<rg3::cpp::ClassProperty>("CppClassProperty")
		.add_property("name", make_getter(&rg3::cpp::ClassProperty::sName))
		.add_property("alias", make_getter(&rg3::cpp::ClassProperty::sAlias))
		.add_property("visibility", make_getter(&rg3::cpp::ClassProperty::eVisibility))
		.add_property("tags", make_getter(&rg3::cpp::ClassProperty::vTags))
		.add_property("type_info", make_getter(&rg3::cpp::ClassProperty::sTypeName))
		.def("__eq__", make_function(&rg3::cpp::ClassProperty::operator==, return_value_policy<return_by_value>()))
		.def("__ne__", make_function(&rg3::cpp::ClassProperty::operator!=, return_value_policy<return_by_value>()))
	;

	// TODO: Need create wrapper type PyClassProperty
	class_<rg3::cpp::ClassFunction>("CppClassFunction")
		.add_property("name", make_getter(&rg3::cpp::ClassFunction::sName))
		.add_property("owner", make_getter(&rg3::cpp::ClassFunction::sOwnerClassName))
		.add_property("visibility", make_getter(&rg3::cpp::ClassFunction::eVisibility))
		.add_property("tags", make_getter(&rg3::cpp::ClassFunction::vTags))
		.add_property("is_static", make_getter(&rg3::cpp::ClassFunction::bIsStatic))
		.add_property("is_const", make_getter(&rg3::cpp::ClassFunction::bIsConst))
		.def("__eq__", make_function(&rg3::cpp::ClassFunction::operator==))
		.def("__ne__", make_function(&rg3::cpp::ClassFunction::operator!=))
	;

	enum_<rg3::llvm::CxxStandard>("CppStandard")
		.value("CXX_11", rg3::llvm::CxxStandard::CC_11)
		.value("CXX_14", rg3::llvm::CxxStandard::CC_14)
		.value("CXX_17", rg3::llvm::CxxStandard::CC_17)
		.value("CXX_20", rg3::llvm::CxxStandard::CC_20)
		.value("CXX_23", rg3::llvm::CxxStandard::CC_23)
		.value("CXX_DEFAULT", rg3::llvm::CxxStandard::CC_DEFAULT)
	;

	enum_<rg3::llvm::IncludeKind>("CppIncludeKind")
		.value("IK_PROJECT", rg3::llvm::IncludeKind::IK_PROJECT)
		.value("IK_SYSTEM", rg3::llvm::IncludeKind::IK_SYSTEM)
		.value("IK_SYSROOT", rg3::llvm::IncludeKind::IK_SYSROOT)
		.value("IK_THIRD_PARTY", rg3::llvm::IncludeKind::IK_THIRD_PARTY)
		.value("IK_DEFAULT", rg3::llvm::IncludeKind::IK_DEFAULT)
	;

	class_<rg3::llvm::IncludeInfo>("CppIncludeInfo")
		.def(init<std::string, rg3::llvm::IncludeKind>(args("path", "kind")))
		.add_property("path", make_getter(&rg3::llvm::IncludeInfo::sFsLocation))
		.add_property("kind", make_getter(&rg3::llvm::IncludeInfo::eKind))
	;

	enum_<rg3::llvm::AnalyzerResult::CompilerIssue::IssueKind>("CppCompilerIssueKind")
		.value("IK_NONE", rg3::llvm::AnalyzerResult::CompilerIssue::IssueKind::IK_NONE)
		.value("IK_WARNING", rg3::llvm::AnalyzerResult::CompilerIssue::IssueKind::IK_WARNING)
		.value("IK_INFO", rg3::llvm::AnalyzerResult::CompilerIssue::IssueKind::IK_INFO)
		.value("IK_ERROR", rg3::llvm::AnalyzerResult::CompilerIssue::IssueKind::IK_ERROR)
	;

	class_<rg3::llvm::AnalyzerResult::CompilerIssue>("CppCompilerIssue", no_init)
		.add_property("kind", make_getter(&rg3::llvm::AnalyzerResult::CompilerIssue::kind))
		.add_property("source_file", make_getter(&rg3::llvm::AnalyzerResult::CompilerIssue::sSourceFile))
		.add_property("message", make_getter(&rg3::llvm::AnalyzerResult::CompilerIssue::sMessage))
	;

	class_<rg3::pybind::PyTypeBase, boost::noncopyable, boost::shared_ptr<rg3::pybind::PyTypeBase>>("CppBaseType", no_init)
	    .add_property("name", make_function(&rg3::pybind::PyTypeBase::pyGetName, return_value_policy<return_by_value>()), "Name of C++ type (without namespace and qualifiers)")
		.add_property("hash", &rg3::pybind::PyTypeBase::__hash__, "Return unique hash of type")
		.add_property("kind", make_function(&rg3::pybind::PyTypeBase::pyGetTypeKind, return_value_policy<return_by_value>()), "Kind of type")
		.add_property("namespace", make_function(&rg3::pybind::PyTypeBase::pyGetNamespace, return_value_policy<copy_const_reference>()), "Namespace where declared type")
		.add_property("location", make_function(&rg3::pybind::PyTypeBase::pyGetLocation, return_value_policy<copy_const_reference>()), "Location where type declared")
		.add_property("pretty_name", &rg3::pybind::PyTypeBase::pyGetPrettyName, "Pretty name of type with namespace")
		.add_property("tags", make_function(&rg3::pybind::PyTypeBase::pyGetTags, return_value_policy<copy_const_reference>()), "Tags presented in type")

		.def("__str__", make_function(&rg3::pybind::PyTypeBase::__str__, return_value_policy<copy_const_reference>()))
		.def("__repr__", make_function(&rg3::pybind::PyTypeBase::__repr__, return_value_policy<copy_const_reference>()))
		.def("__hash__", &rg3::pybind::PyTypeBase::__hash__)
		.def("__eq__", &rg3::pybind::PyTypeBase::__eq__)
		.def("__ne__", &rg3::pybind::PyTypeBase::__ne__)
	;

	class_<rg3::pybind::PyTypeEnum, boost::noncopyable, boost::shared_ptr<rg3::pybind::PyTypeEnum>, boost::python::bases<rg3::pybind::PyTypeBase>>("CppEnum", no_init)
		.add_property("entries", make_function(&rg3::pybind::PyTypeEnum::pyGetEnumEntries, return_value_policy<copy_const_reference>()), "Entries of enum")
		.add_property("is_scoped", &rg3::pybind::PyTypeEnum::pyIsScoped, "Is enum scoped or not")
		.add_property("underlying_type", make_function(&rg3::pybind::PyTypeEnum::pyGetUnderlyingTypeStr, return_value_policy<copy_const_reference>()))
	;

	class_<rg3::pybind::PyTypeClass, boost::noncopyable, boost::shared_ptr<rg3::pybind::PyTypeClass>, boost::python::bases<rg3::pybind::PyTypeBase>>("CppClass", no_init)
		.add_property("properties", make_function(&rg3::pybind::PyTypeClass::pyGetClassProperties, return_value_policy<copy_const_reference>()), "Class properties")
		.add_property("functions", make_function(&rg3::pybind::PyTypeClass::pyGetClassFunctions, return_value_policy<copy_const_reference>()), "Class functions")
		.add_property("is_struct", &rg3::pybind::PyTypeClass::pyIsStruct)
		.add_property("is_trivial_constructible", &rg3::pybind::PyTypeClass::pyIsTriviallyConstructible)
		.add_property("parent_types", make_function(&rg3::pybind::PyTypeClass::pyGetClassParentsTypeNamesList, return_value_policy<copy_const_reference>()))
	;

	class_<rg3::pybind::PyCodeAnalyzerBuilder, boost::noncopyable, boost::shared_ptr<rg3::pybind::PyCodeAnalyzerBuilder>>("CodeAnalyzer", no_init)
		.def("make", &rg3::pybind::PyCodeAnalyzerBuilder::makeInstance)
		.staticmethod("make")

		.add_property("types", make_function(&rg3::pybind::PyCodeAnalyzerBuilder::getFoundTypes, return_value_policy<copy_const_reference>()))
		.add_property("issues", make_function(&rg3::pybind::PyCodeAnalyzerBuilder::getFoundIssues, return_value_policy<copy_const_reference>()))
		.add_property("ignore_runtime",
					  &rg3::pybind::PyCodeAnalyzerBuilder::isNonRuntimeTypesAllowedToBeCollected,
					  &rg3::pybind::PyCodeAnalyzerBuilder::setAllowToCollectNonRuntimeTypes,
					  "Allow to ignore @runtime tag near type decl")
		.add_property("definitions",
					  &rg3::pybind::PyCodeAnalyzerBuilder::getCompilerDefinitions,
					  &rg3::pybind::PyCodeAnalyzerBuilder::setCompilerDefinitions,
					  "")

		.def("set_ignore_non_runtime_types", &rg3::pybind::PyCodeAnalyzerBuilder::setAllowToCollectNonRuntimeTypes)
		.def("is_non_runtime_types_ignored", &rg3::pybind::PyCodeAnalyzerBuilder::isNonRuntimeTypesAllowedToBeCollected)

		.def("set_code", &rg3::pybind::PyCodeAnalyzerBuilder::setSourceCode)
		.def("set_file", &rg3::pybind::PyCodeAnalyzerBuilder::setSourceFile)
		.def("set_cpp_standard", &rg3::pybind::PyCodeAnalyzerBuilder::setCppStandard)
		.def("set_compiler_args", &rg3::pybind::PyCodeAnalyzerBuilder::setCompilerArgs)
		.def("set_compiler_include_dirs", &rg3::pybind::PyCodeAnalyzerBuilder::setCompilerIncludeDirs)
		.def("add_include_dir", &rg3::pybind::PyCodeAnalyzerBuilder::addIncludeDir)
		.def("add_project_include_dir", &rg3::pybind::PyCodeAnalyzerBuilder::addProjectIncludeDir)
		.def("get_definitions", &rg3::pybind::PyCodeAnalyzerBuilder::getCompilerDefinitions)
		.def("set_definitions", &rg3::pybind::PyCodeAnalyzerBuilder::setCompilerDefinitions)
		.def("analyze", &rg3::pybind::PyCodeAnalyzerBuilder::analyze)
	;

	class_<rg3::pybind::PyAnalyzerContext, boost::noncopyable , boost::shared_ptr<rg3::pybind::PyAnalyzerContext>>("AnalyzerContext", no_init)
		.def("make", &rg3::pybind::PyAnalyzerContext::makeInstance)
		.staticmethod("make")

		// Properties
		.add_property("workers_count", &rg3::pybind::PyAnalyzerContext::getWorkersCount, "Count of workers which will prepare incoming sources")
		.add_property("types", make_function(&rg3::pybind::PyAnalyzerContext::getFoundTypes, return_value_policy<copy_const_reference>()))
		.add_property("issues", make_function(&rg3::pybind::PyAnalyzerContext::getFoundIssues, return_value_policy<copy_const_reference>()))
		.add_property("headers", &rg3::pybind::PyAnalyzerContext::getHeaders, "List of headers")
		.add_property("include_directories", &rg3::pybind::PyAnalyzerContext::getCompilerIncludeDirs, "Include directories for compiler instance")
		.add_property("cpp_standard", &rg3::pybind::PyAnalyzerContext::getCppStandard, &rg3::pybind::PyAnalyzerContext::setCppStandard, "Set C++ standard")
		.add_property("compiler_args", &rg3::pybind::PyAnalyzerContext::getCompilerArgs, "Set clang compiler arguments list")
		.add_property("ignore_runtime_tag", &rg3::pybind::PyAnalyzerContext::isRuntimeTagIgnored, &rg3::pybind::PyAnalyzerContext::setIgnoreRuntimeTag, "Should context ignore @runtime tag on 'collect types' stage")
		.add_property("compiler_defs", &rg3::pybind::PyAnalyzerContext::getCompilerDefs, "Compiler definitions")

		// Functions
		.def("set_workers_count", &rg3::pybind::PyAnalyzerContext::setWorkersCount)
		.def("set_headers", &rg3::pybind::PyAnalyzerContext::setHeaders)
		.def("set_include_directories", &rg3::pybind::PyAnalyzerContext::setCompilerIncludeDirs)
		.def("set_compiler_args", &rg3::pybind::PyAnalyzerContext::setCompilerArgs)
		.def("set_compiler_defs", &rg3::pybind::PyAnalyzerContext::setCompilerDefs)
		.def("analyze", &rg3::pybind::PyAnalyzerContext::analyze)
	;

	class_<rg3::pybind::PyClangRuntime, boost::noncopyable>("ClangRuntime")
	    .def("get_version", &rg3::pybind::PyClangRuntime::getRuntimeInfo)
		.staticmethod("get_version")

		.def("detect_system_include_sources", &rg3::pybind::PyClangRuntime::detectSystemIncludeSources)
		.staticmethod("detect_system_include_sources")
	;
}