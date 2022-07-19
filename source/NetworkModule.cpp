#include "NetworkModule.h"
#include "runner.h"
#include "logger.h"

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
        if (logger::SetLogFile("./NetFramework.log"))
            logger::OpenFile();

        InitProtocols();
    }

    void Finish()
    {
        FinishProtocols();

        logger::CloseFile();
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