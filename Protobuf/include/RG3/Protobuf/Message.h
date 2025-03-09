#pragma once

#include <RG3/Protobuf/Location.h>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>


namespace rg3::pb
{
	struct MessageField
	{
		std::string sName {};
		int iIndex { 0 };
		/// TYPE
	};

	using MessageFieldVector = std::vector<MessageField>;

	class Message : public boost::noncopyable
	{
	 public:
		using Ptr = boost::shared_ptr<Message>;

	 public:
		Message();
		Message(const std::string& sName, const Location& sLocation, const MessageFieldVector& vFields);

	 private:
		std::string m_sName {};
		Location m_sLocation {};
		MessageFieldVector m_aFields {};
	};
}