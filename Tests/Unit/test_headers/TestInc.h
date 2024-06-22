#pragma once

// Remove this workaround after migrate to LLVM 17.x or later
#if __clang_major__ < 17
#	undef __clang_major__
#	define __clang_major__ 17
#endif

#include <Awesome/Awesome.h>


static void DoFoo(awesome::SomethingAwesome* pAwesome) {
	// We need to do awesome stuff here
}