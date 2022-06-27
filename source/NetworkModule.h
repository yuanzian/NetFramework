#pragma once
#include <mutex>

#include "runner.h"

namespace NetworkModule {
    extern "C" runner<std::function<int( Context*, std::string)>,  Context*, std::string> *GetBrowseRunner();
}