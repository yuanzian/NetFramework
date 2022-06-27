
#include "source/NetworkModule.h"

#include <string>

using namespace NetworkModule;

struct DLNAContext
{
    std::string uuid;
    std::string objid;
    //std::string friendlyName;
    std::string location;
};

int main()
{
    //Test();
    std::string uuid = "test", objid = "0", location = "1.com";
    Context ctx
    {
        .proto = FindProtocol("dlna"),
        .priv_data = std::make_shared<DLNAContext>(uuid, objid, location),
        .priority = Context::Priority::S
    };
    GetBrowseRunner()->CreateTask(&ctx, "");

    GetBrowseRunner()->AddToConsumer(&ctx, "");

    //runner<std::function<int(std::unique_ptr<int>, Context*)>, std::unique_ptr<int>, Context*> test{
    //    [](std::unique_ptr<int> a,Context* b) {return 0; }
    //};
    //test.CreateTask(std::make_unique<int>(2), &ctx);

}