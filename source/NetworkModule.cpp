#include "NetworkModule.h"


namespace NetworkModule
{
    runner BrowseRunner(
        [](const std::unique_ptr<Context>& ctx)
        {
            return ctx->proto.browse(ctx);
        });

    runner SearchRunner(
        [](const std::unique_ptr<Context>& ctx)
        {
            return ctx->proto.discover(ctx);
        }
    );

    extern "C" runner* GetBrowseRunner()
    {
        return &BrowseRunner;
    }
}