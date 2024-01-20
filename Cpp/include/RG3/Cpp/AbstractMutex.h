#pragma once

#include <type_traits>


namespace rg3::cpp
{
	struct NullMutex
	{
		void lock() {}
		void unlock() {}
		bool try_lock() { return true; } // NOLINT(*-convert-member-functions-to-static)
	};

	template <typename T>
	concept IsMutex = requires(T t)
	{
		t.lock();
		t.unlock();
		{ t.try_lock() } -> std::same_as<bool>;
	};
}