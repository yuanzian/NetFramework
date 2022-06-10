
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

    GetBrowseRunner()->AddToConsumer(ctx);
}