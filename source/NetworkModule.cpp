#include "NetworkModule.h"
#include "runner.h"

namespace NetworkModule
{
    runner<std::function<int(Context*, std::string)>, Context*, std::string>
        BrowseRunner(
            [](Context* ctx, std::string path)
            {
                return ctx->proto->browse(ctx->priv_data);
            }
    );

    runner<std::function<int(Context*)>, Context*> SearchRunner(
        [](Context* ctx)
        {
            return ctx->proto->discover(ctx->priv_data);
        }
    );

    void Init()
    {
        //SetLogFile("./NetFramework.log");
        InitProtocols();
    }

    extern "C" runner<std::function<int(Context*)>, Context*> *GetSearchRunner()
    {
        return &SearchRunner;
    }

    extern "C" runner<std::function<int(Context*, std::string)>, Context*, std::string> *GetBrowseRunner()
    {
        return &BrowseRunner;
    }
}