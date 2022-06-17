#include "NetworkModule.h"


namespace NetworkModule
{
    runner BrowseRunner(
        [](const std::unique_ptr<Context>& ctx)
        {
            return ctx->proto.browse(ctx.get());
        });

    runner SearchRunner(
        [](const std::unique_ptr<Context>& ctx)
        {
            return ctx->proto.discover(ctx.get());
        }
    );

    extern "C" runner* GetBrowseRunner()
    {
        return &BrowseRunner;
    }
}