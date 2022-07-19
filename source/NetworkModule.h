#pragma once
#include <mutex>

#include "runner.h"

namespace NetworkModule {

    void Init();
    extern "C" runner<std::function<int(Context*)>, Context*> *GetSearchRunner();
    extern "C" runner<std::function<int( Context*, std::string)>,  Context*, std::string> *GetBrowseRunner();
}