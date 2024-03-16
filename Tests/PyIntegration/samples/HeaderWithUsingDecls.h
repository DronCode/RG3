#pragma once

namespace not_std {
	template <typename T> struct shared_ptr {}; // do stuff
}

namespace engine {
	/// @runtime
	struct MyComponent
	{
		/// @runtime
		using Ptr = not_std::shared_ptr<MyComponent>;
	};
}

namespace xzibit_stl
{
    /// @runtime
	using i32 = int;

	/// @runtime
	using u32 = unsigned int;

	/// @runtime
	using bytes = unsigned char*;
}