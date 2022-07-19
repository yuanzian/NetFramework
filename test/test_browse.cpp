#include <iostream>
#include <string>
#include <thread>

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
    //std::cout << std::boolalpha << logger::SetLogFile(R"(./test.log)") << std::endl;
    //std::cout << std::boolalpha << logger::OpenFile() << std::endl;

    //std::string uuid = "test", objid = "0", location = "1.com";
    //Context ctx
    //{
    //    .proto = FindProtocol("dlna"),
    //    .priv_data = std::make_shared<DLNAContext>(uuid, objid, location),
    //    .priority = Context::Priority::S
    //};

    Context ctx
    {
        .proto = FindProtocol("smb"),
        .priv_data = std::make_shared<SMBContext>("127.0.0.1", "Share", "", "Guest","",nullptr,-1,nullptr,0,0,SMBContext::SambaVersion::SMB_UNDEFINED),
        .priority = Context::Priority::S
    };

    std::cout << std::boolalpha << GetBrowseRunner()->IsInitialized() << '\n';
    logger::Log(logger::logLevel::Trace, "{}", ctx.proto->name);

    GetBrowseRunner()->CreateTask(&ctx, "");

    GetBrowseRunner()->AddToConsumer(&ctx, "");

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(7s);

#if _WIN32
    system("pause");
    WSACleanup();
#endif
}