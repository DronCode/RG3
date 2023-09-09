#include <string>
#include <vector>
#include <variant>

#define BOOST_PYTHON_STATIC_LIB  // required because we using boost.python as static library
#include <boost/python.hpp>


#include <RG3/Cpp/CppNamespace.h>
#include <RG3/Cpp/TypeClass.h>
#include <RG3/Cpp/TypeEnum.h>

#include <RG3/PyBind/PyCodeAnalyzerBuilder.h>
#include <RG3/PyBind/PyTypeBase.h>
#include <RG3/PyBind/PyTypeEnum.h>
#include <RG3/PyBind/PyTypeClass.h>
#include <RG3/PyBind/PyTag.h>



using namespace boost::python;


BOOST_PYTHON_MODULE(rg3py_ext)
{
	/**
	 * Register base and trivial types here, other in impls
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
		.def("get_type", &rg3::cpp::TagArgument::getHoldedType, "Returns holded type")
		.def("as_bool", make_function(&rg3::cpp::TagArgument::asBool, return_value_policy<return_by_value>()))
		.def("as_float", make_function(&rg3::cpp::TagArgument::asFloat, return_value_policy<return_by_value>()))
		.def("as_i64", make_function(&rg3::cpp::TagArgument::asI64, return_value_policy<return_by_value>()))
		.def("as_string", make_function(&rg3::cpp::TagArgument::asString, return_value_policy<return_by_value>()))
	;

	class_<rg3::pybind::PyTag>("Tag")
		.add_property("name", make_function(&rg3::pybind::PyTag::pyGetName, return_value_policy<copy_const_reference>()))
		.add_property("arguments", make_function(&rg3::pybind::PyTag::pyGetArguments, return_value_policy<copy_const_reference>()))
		.def("__eq__", make_function(&rg3::pybind::PyTag::__eq__, return_value_policy<return_by_value>()))
		.def("__ne__", make_function(&rg3::pybind::PyTag::__ne__, return_value_policy<return_by_value>()))
	;

	class_<rg3::cpp::TypeReference>("CppTypeReference")
		.def(init<std::string>(args("typename")))
	;

	class_<rg3::cpp::EnumEntry>("CppEnumEntry")
		.add_property("name", make_getter(&rg3::cpp::EnumEntry::sName), "Name of entry")
		.add_property("value", make_getter(&rg3::cpp::EnumEntry::iValue), "Value of entry")
	;

	class_<rg3::cpp::ClassProperty>("CppClassProperty")
		.add_property("name", make_getter(&rg3::cpp::ClassProperty::sName))
		.add_property("alias", make_getter(&rg3::cpp::ClassProperty::sAlias))
		.add_property("visibility", make_getter(&rg3::cpp::ClassProperty::eVisibility))
		.add_property("tags", make_getter(&rg3::cpp::ClassProperty::vTags))
		.def("__eq__", make_function(&rg3::cpp::ClassProperty::operator==, return_value_policy<return_by_value>()))
		.def("__ne__", make_function(&rg3::cpp::ClassProperty::operator!=, return_value_policy<return_by_value>()))
	;

	class_<rg3::cpp::ClassFunction>("CppClassFunction")
		.add_property("name", make_getter(&rg3::cpp::ClassFunction::sName))
		.add_property("owner", make_getter(&rg3::cpp::ClassFunction::sOwnerClassName))
		.add_property("visibility", make_getter(&rg3::cpp::ClassFunction::eVisibility))
		.add_property("tags", make_getter(&rg3::cpp::ClassProperty::vTags))
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
		.def(init<std::string>(args("path")))
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
		.add_property("tags", make_function(&rg3::pybind::PyTypeBase::pyGetTags, return_value_policy<copy_const_reference>()), "Dict between tag and tag itself")

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

		.def("set_code", &rg3::pybind::PyCodeAnalyzerBuilder::setSourceCode)
		.def("set_file", &rg3::pybind::PyCodeAnalyzerBuilder::setSourceFile)
		.def("set_cpp_standard", &rg3::pybind::PyCodeAnalyzerBuilder::setCppStandard)
		.def("set_compiler_args", &rg3::pybind::PyCodeAnalyzerBuilder::setCompilerArgs)
		.def("set_compiler_include_dirs", &rg3::pybind::PyCodeAnalyzerBuilder::setCompilerIncludeDirs)
		.def("add_include_dir", &rg3::pybind::PyCodeAnalyzerBuilder::addIncludeDir)
		.def("add_project_include_dir", &rg3::pybind::PyCodeAnalyzerBuilder::addProjectIncludeDir)
		.def("analyze", &rg3::pybind::PyCodeAnalyzerBuilder::analyze)
	;
}