#pragma once

#include <RG3/Cpp/AbstractMutex.h>
#include <boost/noncopyable.hpp>


namespace rg3::cpp
{
	template <typename TMutex> requires (IsMutex<TMutex>)
	struct TransactionGuard : boost::noncopyable
	{
		TMutex& owned;

		explicit TransactionGuard(TMutex& toBeLocked) : owned(toBeLocked)
		{
			owned.lock();
		}

		~TransactionGuard()
		{
			owned.unlock();
		}
	};
}