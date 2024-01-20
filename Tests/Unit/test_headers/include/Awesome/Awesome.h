#pragma once

#ifdef ENABLE_STD_TEST_ARGS
#include <string>
#endif

namespace awesome
{
	/**
	 * @runtime
	 */
	struct SomethingAwesome
	{
#ifdef ENABLE_STD_TEST_ARGS
		std::string awesomeName {};  /// @property
		int awesomeAge { 42 };  /// @property
#endif
	};
}