#include "NetworkModule.h"


namespace NetworkModule
{
    runner BrowseRunner(
        [&](const std::unique_ptr<Context>& ctx)
        {
            return ctx->proto.browse(ctx.get());
        });

    extern "C" runner* GetBrowseRunner()
    {
        return &BrowseRunner;
    }
}