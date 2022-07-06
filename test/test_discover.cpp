#include "source/NetworkModule.h"
#include "source/protocol/protocol_smb.h"
#include "source/logger.h"

using namespace NetworkModule;

int main()
{
#if _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(1, 1), &wsaData);
    SetConsoleOutputCP(CP_UTF8);
#endif

    Context ctx
    {
        .proto = FindProtocol("smb"),
        .priv_data = std::make_shared<SMBContext>(),
        .priority = Context::Priority::S
    };

    GetSearchRunner()->AddToConsumer(&ctx);

#if _WIN32
    system("pause");
    WSACleanup();
#endif
}