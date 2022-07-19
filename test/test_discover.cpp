#include "source/NetworkModule.h"
#include "source/protocol/protocol_smb.h"
#include "source/logger.h"

using namespace NetworkModule;

struct DLNAContext
{
    std::string uuid;
    std::string objid;
    std::string friendlyName;
    std::string manufacturer;
    std::string location;
};

int main()
{
#if _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(1, 1), &wsaData);
    SetConsoleOutputCP(CP_UTF8);
#endif

    Init();

    Context ctx
    {
        .proto = FindProtocol("smb"),
        .priv_data = std::make_shared<SMBContext>(),
        .priority = Context::Priority::S
    };

    Context ctx1
    {
        .proto = FindProtocol("dlna"),
        .priv_data = std::make_shared<DLNAContext>(),
        .priority = Context::Priority::S
    };

    GetSearchRunner()->AddToConsumer(&ctx);
    GetSearchRunner()->AddToConsumer(&ctx1);

    //Finish();
#if _WIN32
    system("pause");
    WSACleanup();
#endif
}