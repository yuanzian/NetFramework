#pragma once
#include <mutex>

#include "runner.h"

namespace NetworkModule {
    extern "C" runner* GetBrowseRunner();
}