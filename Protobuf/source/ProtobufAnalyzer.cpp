#include <RG3/Protobuf/ProtobufAnalyzer.h>

#include <google/protobuf/compiler/parser.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <fstream>
#include <sstream>
#include <fstream>
#include <memory>


namespace rg3::pb
{
	ProtobufAnalyzer::ProtobufAnalyzer() = default;

	void ProtobufAnalyzer::setCode(const std::string& sCode)
	{
		m_sSource = sCode;
	}

	void ProtobufAnalyzer::setFile(const std::filesystem::path& sPath)
	{
		m_sSource = sPath;
	}

	void ProtobufAnalyzer::setSource(const CodeSource& src)
	{
		m_sSource = src;
	}

	void ProtobufAnalyzer::setCompilerConfig(const CompilerConfig& sConfig)
	{
		m_sConfig = sConfig;
	}

	const CompilerConfig& ProtobufAnalyzer::getCompilerConfig() const
	{
		return m_sConfig;
	}

	CompilerConfig& ProtobufAnalyzer::getCompilerConfig()
	{
		return m_sConfig;
	}

	const CompilerIssuesVector& ProtobufAnalyzer::getIssues() const
	{
		return m_aIssues;
	}

	struct InMemoryErrorCollector final : google::protobuf::io::ErrorCollector
	{
		std::vector<CompilerIssue>* paIssues {};

		explicit InMemoryErrorCollector(std::vector<CompilerIssue>* pOut) : google::protobuf::io::ErrorCollector(), paIssues(pOut) {}

		void RecordError(int line, google::protobuf::io::ColumnNumber column, absl::string_view message) override
		{
			CompilerIssue& sIssue = paIssues->emplace_back();
			sIssue.eKind = IssueKind::IK_ERROR;
			sIssue.iLine = line;
			sIssue.iColumn = column;
			sIssue.sMessage = message.data();
		}

		void RecordWarning(int line, google::protobuf::io::ColumnNumber column, absl::string_view message) override
		{
			CompilerIssue& sIssue = paIssues->emplace_back();
			sIssue.eKind = IssueKind::IK_WARNING;
			sIssue.iLine = line;
			sIssue.iColumn = column;
			sIssue.sMessage = message.data();
		}
	};

	template<typename T> constexpr bool always_false_v = false;

	std::pair<std::unique_ptr<std::istream>, std::string> getStream(const ProtobufAnalyzer::CodeSource& source) {
		return std::visit([](const auto& value) -> std::pair<std::unique_ptr<std::istream>, std::string> {
			using T = std::decay_t<decltype(value)>;

			if constexpr (std::is_same_v<T, std::string>) {
				return { std::make_unique<std::istringstream>(value), "id0.proto" };
			} else if constexpr (std::is_same_v<T, std::filesystem::path>) {
				auto stream = std::make_unique<std::ifstream>(value);
				if (!stream->is_open())
				{
					return { nullptr, "" };
				}

				return { std::move(stream), value.string() };
			} else {
				static_assert(always_false_v<T>, "Unhandled variant type");
			}
		}, source);
	}

	bool ProtobufAnalyzer::analyze()
	{
		m_aIssues.clear();

		// 1. Parse
		auto [pStreamMem, sStreamId] = getStream(m_sSource);
		if (!pStreamMem)
		{
			CompilerIssue& sIssue = m_aIssues.emplace_back();
			sIssue.sMessage = "Failed to handle I/O (unable to open file IO)";
			sIssue.iColumn = sIssue.iLine = 0;
			return false;
		}

		InMemoryErrorCollector sErrorCollector { &m_aIssues };
		google::protobuf::compiler::Parser sProtobufParser {};
		google::protobuf::io::IstreamInputStream sStream { pStreamMem.get() };
		google::protobuf::io::Tokenizer sTokenizer { &sStream, &sErrorCollector };
		google::protobuf::FileDescriptorProto sDescriptor {};

		sProtobufParser.RecordErrorsTo(&sErrorCollector);
		sDescriptor.set_name(sStreamId);

		if (!sProtobufParser.Parse(&sTokenizer, &sDescriptor))
		{
			return false;
		}

		// Semantic analysis
		google::protobuf::DescriptorPool sDescriptorPool;

		google::protobuf::DescriptorPool sBuiltinPool(google::protobuf::DescriptorPool::generated_pool());
		const google::protobuf::FileDescriptor* pFileDescriptor = sBuiltinPool.BuildFile(sDescriptor);

		if (!pFileDescriptor)
		{
			CompilerIssue& sIssue = m_aIssues.emplace_back();
			sIssue.sMessage = "Semantic error: Failed to resolve types in the file.";
			sIssue.iColumn = sIssue.iLine = 0;
			return false;
		}

		// Done
		return std::count_if(m_aIssues.begin(), m_aIssues.end(), [](const CompilerIssue& sIssue) -> bool { return sIssue.eKind == IssueKind::IK_ERROR; }) == 0;
	}
}