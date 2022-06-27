#include "NetworkModule.h"
#include "runner.h"

namespace NetworkModule
{
    runner<std::function<int(Context*, std::string)>, Context*, std::string>
        BrowseRunner(
            []( Context* ctx, std::string path)
            {
                return 0;
                //return ctx.proto.browse(ctx);
            }
    );

    runner<std::function<int(Context*)>, Context*> SearchRunner(
        [](Context* ctx)
        {
            return 0;
            //return ctx->proto.discover(ctx);
        }
    );

    extern "C" runner<std::function<int( Context*, std::string)>,  Context*, std::string> *GetBrowseRunner()
    {
        return &BrowseRunner;
    }
}