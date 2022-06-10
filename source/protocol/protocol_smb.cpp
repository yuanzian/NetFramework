
#include <string>
#include "protocol.h"

static int smb_discover(Context* ctx)
{
    return 0;
}

static int smb_browse(Context* ctx)
{
    return 0;
}

//struct SMBContext
//{
//    std::string server;
//    std::string share;
//    std::string userID;
//    std::string password;
//    smb_session* session;
//    smb_tid tid;
//    smb2_context* smb2;
//    time_t tmSessionConLastUse;
//    time_t delayDeconstruct;
//    SambaVersion version;
//};

extern const Protocol smb_protocol =
{
    .name = "smb",
    .discover = smb_discover,
    .browse = smb_browse
};