#include <RG3/Protobuf/ProtobufAnalyzer.h>

#include <gtest/gtest.h>
#include <memory>


class Tests_Protobuf : public ::testing::Test
{
 protected:
	void SetUp() override
	{
		g_Analyzer = std::make_unique<rg3::pb::ProtobufAnalyzer>();
	}

	void TearDown() override
	{
		g_Analyzer = nullptr;
	}

 protected:
	std::unique_ptr<rg3::pb::ProtobufAnalyzer> g_Analyzer { nullptr };
};

TEST_F(Tests_Protobuf, CheckErrorHandler)
{
	g_Analyzer->setCode(R"(
syntax = "proto3";

message WhatTheFuck {
	string name = 1;
	int32 id = 1;
	mdma_pzdc field = 40;
}
)");

	g_Analyzer->getCompilerConfig().eSyntax = rg3::pb::ProtobufSyntax::PB_SYNTAX_3;
	g_Analyzer->getCompilerConfig().vIncludeDirs = {};
	ASSERT_FALSE(g_Analyzer->analyze());
}

TEST_F(Tests_Protobuf, SimpleUsage)
{
	g_Analyzer->setCode(R"(
syntax = "proto3";

import "google/protobuf/descriptor.proto";

// Custom entries (should be declared somehow)
extend google.protobuf.MessageOptions {
  bool runtime = 707228;
}

message User {
	option (runtime) = true;

	string id = 1;
	string first_name = 2;
	string last_name = 3;
	string avatar = 4;
}
)");

	g_Analyzer->getCompilerConfig().eSyntax = rg3::pb::ProtobufSyntax::PB_SYNTAX_3;
	g_Analyzer->getCompilerConfig().vIncludeDirs = {};
	ASSERT_TRUE(g_Analyzer->analyze());
}