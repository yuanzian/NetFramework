#include "NetworkModule.h"
#include "runner.h"

namespace NetworkModule
{
    std::unique_ptr<int> a;
    runner<std::function<int(Context*, std::string)>, Context*, std::string>
        BrowseRunner(
            [](Context* ctx, std::string path)
            {
                return ctx->proto.browse(ctx);
            }
    );

    runner<std::function<int(Context*)>, Context*> SearchRunner(
        [](Context* ctx)
        {
            return ctx->proto.discover(ctx);
        }
    );

    extern "C" runner<std::function<int(Context*, std::string)>, Context*, std::string> *GetBrowseRunner()
    {
        return &BrowseRunner;
    }
}