#pragma once

#include <string>
#include <memory>

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
    enum class SambaVersion :int
    {
        SMB_UNDEFINED = -1,
        SMB1 = 1,
        SMB2_02 = 0x0202,
        SMB2_10 = 0x0210,
        SMB3_00 = 0x0300,
        SMB3_02 = 0x0302,
        SMB3_11 = 0x0311
    }version;

    ~SMBContext();
};

static int smb_discover(std::shared_ptr<void> ctx);
static int smb_browse(std::shared_ptr<void> ctx);

LoginError smbc_connect_share(std::shared_ptr<SMBContext> smbc);
bool IsValid();
int smbc_get_shareList(smb_share_list& shareList);
const char* smbc_get_errorInfo();
void* smbc_opendir(std::shared_ptr<SMBContext> smbc);
void* smbc_readdir(std::shared_ptr<SMBContext> smbc, void*& dir);
void smbc_resolve_file_stat(std::shared_ptr<SMBContext> smbc, void* ent, std::string& name, uint64_t& cTime, uint64_t& size, bool& isDir);
void smbc_closedir(std::shared_ptr<SMBContext> smbc, void* files);
void* smbc_open(const char* file, int flag);
int64_t smbc_seek(void* fh, int64_t offset, int whence);
int64_t smbc_get_file_size(void* fh);
int smbc_get_max_read_size();
int smbc_read(void* fh, uint8_t* buf, uint32_t len);
void smbc_close(void* fh);