#include <RG3/Cpp/TypeBaseInfo.h>


namespace rg3::cpp
{
	TypeBaseInfo::TypeBaseInfo() = default;
	TypeBaseInfo::TypeBaseInfo(const TypeBaseInfo& copy)
		: eKind(copy.eKind)
	    , sName(copy.sName)
		, sPrettyName(copy.sPrettyName)
		, sNameSpace(copy.sNameSpace)
		, sDefLocation(copy.sDefLocation)
	{}

	TypeBaseInfo::TypeBaseInfo(TypeBaseInfo&& move) noexcept
		: eKind(move.eKind)
	    , sName(std::move(move.sName))
	    , sPrettyName(std::move(move.sPrettyName))
	    , sNameSpace(std::move(move.sNameSpace))
	    , sDefLocation(std::move(move.sDefLocation))
	{
		move.eKind = {};
		move.sName = {};
		move.sPrettyName = {};
		move.sNameSpace = {};
		move.sDefLocation = {};
	}

	TypeBaseInfo& TypeBaseInfo::operator=(const TypeBaseInfo& copy)
	{
		eKind = copy.eKind;
		sName = copy.sName;
		sPrettyName = copy.sPrettyName;
		sNameSpace = copy.sNameSpace;
		sDefLocation = copy.sDefLocation;
		return *this;
	}

	TypeBaseInfo& TypeBaseInfo::operator=(TypeBaseInfo&& move) noexcept
	{
		eKind = move.eKind;
		sName = std::move(move.sName);
		sPrettyName = std::move(move.sPrettyName);
		sNameSpace  = std::move(move.sNameSpace);
		sDefLocation = std::move(move.sDefLocation);

		move.eKind = {};
		move.sName = {};
		move.sPrettyName = {};
		move.sNameSpace = {};
		move.sDefLocation = {};

		return *this;
	}
}