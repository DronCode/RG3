#pragma once

// __RG3__ is defined for every target. It make possible to determine which file should be ignored or not
// __RG3_COMMIT__ contains hash of git commit. Usually, it's same to your version commit.
// __RG3__BUILD_DATE contains build date. Could be different between platforms (because CI)

#ifndef __RG3__
#error RG3 not defined
#endif

#ifndef __RG3_COMMIT__
#error commit hash not defined
#endif

#ifndef __RG3_BUILD_DATE__
#error build date not defined
#endif