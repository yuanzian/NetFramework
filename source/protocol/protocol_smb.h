#pragma once

extern "C" {
#include "bdsm.h"
}
#include "libsmb2.h"
#include "smb2.h"

enum LoginError
{
    SMBC_LOGIN_SUCCESS = 0,
    SMBC_LOGIN_UNKNOWNERROR = -1,
    SMBC_LOGIN_PASSWORDERROR = -2,
    SMBC_LOGIN_CONNECTIONERROR = -3,
    SMBC_LOGIN_AUTHORITYERROR = -4,
    SMBC_LOGIN_INITSESSIONERROR = -5,
    SMBC_LOGIN_PROTOCOLERROR = -6
};

struct SMBContext
{
    std::string server;
    std::string share;
    std::string path;
    std::string userID;
    std::string password;
    smb_session* session;
    smb_tid tid;
    smb2_context* smb2;
    time_t tmSessionConLastUse;
    time_t delayDeconstruct;
    enum SambaVersion
    {
        SMB_UNDEFINED = 0,
        SMB1 = 1,
        SMB2_02 = 0x0202,
        SMB2_10 = 0x0210,
        SMB3_00 = 0x0300,
        SMB3_02 = 0x0302,
        SMB3_11 = 0x0311
    }version;
};


static int smb_discover(Context* ctx);
static int smb_browse(Context* ctx);

LoginError smbc_connect_share(Context* ctx);
bool IsValid();
int smbc_get_shareList(smb_share_list& shareList);
const char* smbc_get_errorInfo();
void* smbc_opendir(Context* ctx);
void* smbc_readdir(Context* ctx, void*& dir);
void smbc_resolve_file_stat(Context* ctx, void* ent, std::string& name, uint64_t& cTime, uint64_t& size, bool& isDir);
void smbc_closedir(Context* ctx, void* files);
void* smbc_open(const char* file, int flag);
int64_t smbc_seek(void* fh, int64_t offset, int whence);
int64_t smbc_get_file_size(void* fh);
int smbc_get_max_read_size();
int smbc_read(void* fh, uint8_t* buf, uint32_t len);
void smbc_close(void* fh);