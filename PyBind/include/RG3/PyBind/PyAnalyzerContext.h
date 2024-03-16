#pragma once

#include <RG3/PyBind/PyTypeBase.h>
#include <RG3/LLVM/CodeAnalyzer.h>
#include <RG3/LLVM/Compiler.h>

#define BOOST_PYTHON_STATIC_LIB
#include <boost/python.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

#include <atomic>
#include <vector>
#include <filesystem>
#include <shared_mutex>


namespace rg3::pybind
{
	/**
	 * @brief Handle request to analyze multiple header files at once. Single instance could use multiple workers and share types data base between analyzers.
	 * @note This class is not copyable.
	 * @note When analyze process running main context thread will be locked (ie analyzer context will wait until all workers will finish their jobs)
	 * @note All produced types will be alive until object is in use
	 * @python Mapped to type AnalyzerContext
	 * @example
	 * {code}
	 * with AnalyzerContext.make() as analyzer_context:
	 * 		analyzer_context.set_workers_count(16)
	 * 		analyzer_context.set_cpp_standard(CppStandard.CXX_17)
	 * 		analyzer_context.add_project_include_directory("include")
	 * 		analyzer_context.add_project_include_directory("ThirdPart/glm/include")
	 *
	 * 		found_headers: List[str] = glob("include\*.h", recursive=True)
	 * 		analyzer_context.set_files(found_headers)
	 *
	 * 		analyzer_context.analyze()  # Run analyze
	 *
	 * 		# Check result
	 * {code}
	 */
	class PyAnalyzerContext : public boost::noncopyable
	{
	 public:
		PyAnalyzerContext();
		~PyAnalyzerContext();

		static boost::shared_ptr<PyAnalyzerContext> makeInstance()
		{
			return boost::shared_ptr<PyAnalyzerContext>(new PyAnalyzerContext());
		}

	 public:
		void setWorkersCount(int workersCount);
		[[nodiscard]] int getWorkersCount() const;

		void setHeaders(const boost::python::list& headers);
		[[nodiscard]] boost::python::list getHeaders() const;

		void setCompilerIncludeDirs(const boost::python::list& includeDirs);
		[[nodiscard]] boost::python::list getCompilerIncludeDirs() const;

		void setCppStandard(rg3::llvm::CxxStandard standard);
		rg3::llvm::CxxStandard getCppStandard() const;

		void setCompilerArgs(const boost::python::list& compilerArgs);
		[[nodiscard]] boost::python::list getCompilerArgs() const;

		void setCompilerDefs(const boost::python::list& compilerDefs);
		[[nodiscard]] boost::python::list getCompilerDefs() const;

		void setIgnoreRuntimeTag(bool bIgnoreRT);
		bool isRuntimeTagIgnored() const;

		boost::python::object pyGetTypeOfTypeReference(const rg3::cpp::TypeReference& typeReference);

		[[nodiscard]] const boost::python::list& getFoundIssues() const;
		[[nodiscard]] const boost::python::list& getFoundTypes() const;

	 public:
		/**
		 * @fn analyze
		 * @brief Run analyze process.
		 * @return true if everything is ok
		 * @note When bShouldWait is true, function will return true when all analyzer thread will be finished
		 */
		bool analyze();

		/**
		 * @fn isFinished
		 * @return true if analyzer finished or not started
		 */
		bool isFinished() const;

	 private:
		bool runAnalyze();

		struct ResolverContext
		{
			enum class ContextSpace {
				CS_UNDEFINED,
				CS_TYPE,
				CS_PROPERTY,
				CS_FUNCTION,
			};

			ContextSpace eSpace { ContextSpace::CS_UNDEFINED };
			boost::shared_ptr<rg3::cpp::TypeBase> pOwner { nullptr };
		};

		void pushResolverIssue(const ResolverContext& context, std::string&& errorMessage);

		bool resolveTypeReferences();
		bool resolveTags(const ResolverContext& context, rg3::cpp::Tags& tagsToResolve);

	 private:
		struct RuntimeContext;
		std::unique_ptr<RuntimeContext> m_pContext { nullptr };

		std::atomic<bool> m_bInProgress { false }; /// When true - analyze in progres
		std::vector<std::filesystem::path> m_headersToPrepare {}; /// List of headers which will be prepared
		rg3::llvm::CompilerConfig m_compilerConfig {}; /// Shared config for all analyzer instances (they 'll use it in read only mode)

		// Found subjects
		struct PyFoundSubjects
		{
			std::shared_mutex   lockMutex;

			// Found & mapped original types. Key - typename (prettified, value - type instance with ownership)
			std::unordered_map<std::string, boost::shared_ptr<rg3::pybind::PyTypeBase>> vFoundTypeInstances;

			// Mapped types to python side
			boost::python::list pyFoundTypes;
			boost::python::list pyFoundIssues;
		};

		PyFoundSubjects m_pySubjects {};

		int m_iWorkersAmount { 2 }; /// How much workers allowed to be used. Note: value must be in range [1, N) where N - count of cpu cores * 2
		bool m_bIgnoreRuntimeTag { false }; /// Should code gen use all possible types or not
	};
}
//PyAnalyzerContext