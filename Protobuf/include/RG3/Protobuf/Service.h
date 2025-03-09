#pragma once

#include <boost/noncopyable.hpp>
#include <string>
#include <vector>


namespace rg3::pb
{
	struct ServiceMethod
	{
		std::string sName {};
		bool bClientStreaming { false };
		bool bServerStreaming { false };

		// INPUT TYPE
		// OUTPUT TYPE
	};

	using ServiceMethodsVector = std::vector<ServiceMethod>;

	class Service : public boost::noncopyable
	{
	 public:
		Service();
		Service(const std::string& sName, const std::string& sPackage, const ServiceMethodsVector& aMethods);
	};
}