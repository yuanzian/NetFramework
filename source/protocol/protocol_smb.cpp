#include <cstring>
#include <string>
#include <fstream>
#include <future>

#if __ANDROID__ || __linux__
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <poll.h>
#endif

#if _WIN64
#include "sys/socket.h"
#define gettid() GetCurrentThreadId()
#endif

#include "protocol.h"
#include "protocol_smb.h"
#include "rules.h"

extern const Protocol smb_protocol =
{
    .name = "smb",
    .discover = smb_discover,
    .browse = smb_browse
};

static int smb_discover(Context* ctx)
{
    return 0;
}

static int smb_browse(Context* ctx)
{
    std::vector<std::tuple<std::string, uint64_t, uint64_t>> vectorVideoProperty;
    std::vector<std::string> vectorSubtitleProperty;
    std::vector<std::string> vectorPictureProperty;
    std::multimap<std::string, std::string> mapSubtitleProperty;
    std::multimap<std::string, std::string> mapThumbnailProperty;
    std::ostringstream xt;

    void* files = nullptr;
    void* file = nullptr;

    int ret = smbc_connect_share(ctx);
    files = smbc_opendir(ctx);
    while ((file = smbc_readdir(ctx, files)) != nullptr)
    {
        std::string name;
        uint64_t cTime;
        uint64_t size;
        bool isDir;
        smbc_resolve_file_stat(ctx, file, name, cTime, size, isDir);

        if (isDir)
        {
            xt << R"(<item type="directory"><name>)" << name << R"(</name>)" << R"(<date>)" << cTime << R"(</date></item>)";
        }
        else if (IsVideo(name))
        {
            vectorVideoProperty.emplace_back(name, cTime, size);
        }
        else if (IsSubtitle(name) && size > 0 && (size >> 20) < 5)
        {
            vectorSubtitleProperty.emplace_back(name);
        }
        else if (IsPicture(name))
        {
            vectorPictureProperty.emplace_back(name);
        }
    }
    smbc_closedir(ctx, files);

    if (!vectorVideoProperty.empty() && !vectorSubtitleProperty.empty())
    {
        for (const auto& [video, cTime, size] : vectorVideoProperty)
        {
            std::string videoWithoutSuffix = video.substr(0, video.find_last_of('.'));
            for (auto it = vectorSubtitleProperty.begin(); it != vectorSubtitleProperty.end();)
            {
                std::string subtitle = *it;
                std::string subtitleWithoutSuffix = subtitle.substr(0, subtitle.find_last_of('.'));
                if (is_video_name_match(subtitleWithoutSuffix, videoWithoutSuffix)
                    || subtitleWithoutSuffix.find(videoWithoutSuffix) != std::string::npos)
                {
                    mapSubtitleProperty.emplace(video, subtitle);
                    it = vectorSubtitleProperty.erase(it);
                }
                else ++it;
            }
        }
    }

    //if (!vectorVideoProperty.empty() && !vectorPictureProperty.empty())
    //{
    //    for (const auto& [video, cTime, size] : vectorVideoProperty)
    //    {
    //        std::string videoWithoutSuffix = video.substr(0, video.find_last_of('.'));
    //        for (const std::string& picture : vectorPictureProperty)
    //        {
    //            std::string pictureWithoutSuffix = picture.substr(0, picture.find_last_of('.'));
    //            if (videoWithoutSuffix == pictureWithoutSuffix)
    //            {
    //                mapThumbnailProperty.emplace(video, picture);
    //                break;
    //            }
    //        }
    //    }
    //}

    for (const auto& [video, cTime, size] : vectorVideoProperty)
    {
        xt << R"(<item type="file"><name>)" << video <<
            "</name><date>" << cTime <<
            "</date><size>" << size << "</size>";

        const auto& [subtitleEntryBegin, subtitleEntryEnd] = mapSubtitleProperty.equal_range(video);
        for_each(subtitleEntryBegin, subtitleEntryEnd,
            [&](std::pair<const std::string, std::string>& subtitleEntry) {
                xt << "<subtitle>" << subtitleEntry.second << "</subtitle>";
            });

        const auto& [thumbnailEntryBegin, thumbnailEntryEnd] = mapThumbnailProperty.equal_range(video);
        for_each(thumbnailEntryBegin, thumbnailEntryEnd,
            [&](std::pair<const std::string, std::string>& thumbnailEntry) {
                xt << "<thumbnail>" << thumbnailEntry.second << "</thumbnail>";
            });
        xt << "</item>";
    }
    xt << "</folderxml>";
    return 0;
}


using namespace std::chrono_literals;
#define SMB1_CONNECTION_TIMEOUT 7s
#define SMB2_CONNECTION_TIMEOUT (3s).count()
#define DESTROY_CONTEXT_DELAY_TIME (30s).count()
//
//SMBContext()
//    : session(nullptr)
//    , tid(0)
//    , smb2(nullptr)
//    , tmSessionConLastUse(0)
//    , delayDeconstruct(0)
//    , version(SMB_UNDEFINED)
//{}
//
//~SMBContext()
//{
//    if (session)
//    {
//        if (tid != 0)
//            smb_tree_disconnect(session, tid);
//        smb_session_destroy(session);
//        session = nullptr;
//    }
//    if (smb2)
//    {
//        smb2_disconnect_share(smb2);
//        smb2_destroy_context(smb2);
//        smb2 = nullptr;
//    }
//}


LoginError smbc_connect_share(Context* ctx)
{
    std::shared_ptr<SMBContext> smbc = std::static_pointer_cast<SMBContext>(ctx->priv_data);

    LoginError loginErrorCode = SMBC_LOGIN_SUCCESS;
    //Log(LEVEL_INFO, "Trying to connect %s with SMB2 protocol.", server.c_str());
    smbc->smb2 = smb2_init_context();
    if (smbc->smb2 == nullptr)
    {
        //Log(LEVEL_ERROR, "Init SMB2 session failed: smb://%s:%s@%s/%s", 
            //Encrypt(userID.c_str()).c_str(), Encrypt(password.c_str()).c_str(), server.c_str(), share.c_str());
        return SMBC_LOGIN_INITSESSIONERROR;
    }

connect:
    smb2_set_security_mode(smbc->smb2, SMB2_NEGOTIATE_SIGNING_ENABLED);
    smb2_set_timeout(smbc->smb2, SMB2_CONNECTION_TIMEOUT);

    if (smb2_connect_share(smbc->smb2, smbc->server.c_str(), smbc->share.c_str(), smbc->userID.c_str(), smbc->password.c_str()) < 0)
    {
        const char* errorMsg = smb2_get_error(smbc->smb2);
        if (strstr(errorMsg, "STATUS_LOGON_FAILURE") ||
            strstr(errorMsg, "STATUS_PASSWORD_EXPIRED") ||
            strstr(errorMsg, "STATUS_PASSWORD_MUST_CHANGE") ||
            strstr(errorMsg, "STATUS_ACCOUNT_EXPIRED") ||
            strstr(errorMsg, "STATUS_ACCOUNT_DISABLED"))
            loginErrorCode = SMBC_LOGIN_PASSWORDERROR;
        else if (strstr(errorMsg, "STATUS_ACCESS_DENIED"))
        {
            if (smb2_get_negotiate_version(smbc->smb2) == SMB2_VERSION_ANY || smb2_get_negotiate_version(smbc->smb2) == SMB2_VERSION_0311)
            {
                //Log(LEVEL_WARNING, "Cur negotiate version is SMB2_VERSION_0311, try to use ANY_VERSION_EXCEPT_0311.");
                smb2_disconnect_share(smbc->smb2);
                smb2_destroy_context(smbc->smb2);
                smbc->smb2 = smb2_init_context();
                smb2_set_negotiate_version(smbc->smb2, SMB2_VERSION_ANYEXCEPT0311);
                goto connect;
            }
            loginErrorCode = SMBC_LOGIN_AUTHORITYERROR;
        }
        else if (strstr(errorMsg, "Timeout") || strstr(errorMsg, "Socket connect failed with 10051"))
        {
            loginErrorCode = SMBC_LOGIN_CONNECTIONERROR;
        }
        else if (strstr(errorMsg, "104") || strstr(errorMsg, "SMB2_STATUS_LOGON_TYPE_NOT_GRANTED"))
        {
            loginErrorCode = SMBC_LOGIN_PROTOCOLERROR;
        }
        else loginErrorCode = SMBC_LOGIN_UNKNOWNERROR;

        //Log(LEVEL_ERROR, "Connecting SMB2 share failed, cause: %s", errorMsg);
    }

    if (loginErrorCode == SMBC_LOGIN_SUCCESS || loginErrorCode == SMBC_LOGIN_AUTHORITYERROR)
    {
        smbc->version = static_cast<SMBContext::SambaVersion>(smb2_get_dialect_version(smbc->smb2));

        //Log(loginErrorCode == SMBC_LOGIN_SUCCESS ? LEVEL_INFO : LEVEL_ERROR,
        //    "Create SMB2 session: smb%x://%s:%s@%s/%s, return %s",
        //    version,
        //    Encrypt(userID.c_str()).c_str(),
        //    Encrypt(password.c_str()).c_str(),
        //    server.c_str(), share.c_str(),
        //    LoginErrorString[-(int)loginErrorCode]);
        return loginErrorCode;
    }
    else if (loginErrorCode == SMBC_LOGIN_PROTOCOLERROR || loginErrorCode == SMBC_LOGIN_CONNECTIONERROR || loginErrorCode == SMBC_LOGIN_UNKNOWNERROR)
    {
        //Log(LEVEL_INFO, "Trying to connect %s with SMB1 protocol.", server.c_str());
        loginErrorCode = SMBC_LOGIN_SUCCESS;
        unsigned int addr = 0;
        struct addrinfo* p_info = nullptr;
        std::promise<int> connectPromise;
        std::future<int> connectFuture = connectPromise.get_future();
        if (getaddrinfo(smbc->server.c_str(), nullptr, nullptr, &p_info) == 0 && p_info->ai_family == AF_INET)
        {
            struct sockaddr_in* in = (struct sockaddr_in*)p_info->ai_addr;
            addr = in->sin_addr.s_addr;
        }
        else
        {
            loginErrorCode = SMBC_LOGIN_CONNECTIONERROR;
            goto error;
        }

        smbc->session = smb_session_new();
        std::thread(
            [&](std::promise<int> res) {
                res.set_value(smb_session_connect(smbc->session, smbc->server.c_str(), addr, SMB_TRANSPORT_TCP));
            }
        , std::move(connectPromise)).detach();

        switch (connectFuture.wait_for(SMB1_CONNECTION_TIMEOUT))
        {
        case std::future_status::timeout:
            smbc->delayDeconstruct = time(nullptr) + DESTROY_CONTEXT_DELAY_TIME;
            loginErrorCode = SMBC_LOGIN_CONNECTIONERROR;
            goto error;
            break;
        case std::future_status::ready:
            if (connectFuture.get() != DSM_SUCCESS)
            {
                loginErrorCode = SMBC_LOGIN_CONNECTIONERROR;
                goto error;
            }
            break;
        default:
            break;
        }

        smb_session_set_creds(smbc->session, smbc->server.c_str(), smbc->userID.c_str(), smbc->password.c_str());
        if (smb_session_login(smbc->session) != DSM_SUCCESS)
        {
            loginErrorCode = SMBC_LOGIN_PASSWORDERROR;
            goto error;
        }
        loginErrorCode = (LoginError)smb_tree_connect(smbc->session, smbc->share.data(), &smbc->tid);
        if (loginErrorCode != DSM_SUCCESS)
            goto error;

        if (smb_session_is_guest(smbc->session) == 1)
        {
            //Log(LEVEL_INFO, "Logged in with Guest");
        }

        //Log(LEVEL_INFO,
        //"Connecting SMB1 server SUCCESS: smb1://%s:%s@%s/%s",
        //    Encrypt(userID.c_str()).c_str(),
        //    Encrypt(password.c_str()).c_str(),
        //    server.c_str(), share.c_str());
        //    version = SMB1;
        //    return loginErrorCode;
    }

error:
    //Log(LEVEL_ERROR, "Connecting SMB server failed: smb://%s:%s@%s/%s, return %s",
    //Encrypt(userID.c_str()).c_str(),
    //    Encrypt(password.c_str()).c_str(),
    //    server.c_str(), share.c_str(),
    //    LoginErrorString[-(int)loginErrorCode]);
    return loginErrorCode;
}
//
//bool IsValid()
//{
//    if (version == SMB1)
//    {
//        return session && smb_session_get_nt_status(session) == DSM_SUCCESS;
//    }
//    else
//    {
//        return smb2 && strlen(smb2_get_error(smb2)) == 0;
//    }
//}
//
//int smbc_get_shareList(smb_share_list& shareList)
//{
//    if (version == SMB1)
//        return smb_share_get_list(session, tid, &shareList, nullptr);
//    else
//    {
//        return (shareList = smb2_share_enum(smb2)) ? 0 : -1;
//    }
//}
//
//const char* smbc_get_errorInfo()
//{
//    if (version == SMB1)
//        return "NTERRNO:";
//    else
//    {
//        return smb2_get_error(smb2);
//    }
//}
//
void* smbc_opendir(Context* ctx)
{
    std::shared_ptr<SMBContext> smbc = std::static_pointer_cast<SMBContext>(ctx->priv_data);

    switch (smbc->version)
    {
    case SMBContext::SMB1:
        return smb_find(smbc->session, smbc->tid, (smbc->path + "\\*").data());
    case SMBContext::SMB2_02:
    case SMBContext::SMB2_10:
    case SMBContext::SMB3_00:
    case SMBContext::SMB3_02:
    case SMBContext::SMB3_11:
        return smb2_opendir(smbc->smb2, smbc->path.data());
    default:
        return nullptr;
    }
}

void* smbc_readdir(Context* ctx, void*& dir)
{
    std::shared_ptr<SMBContext> smbc = std::static_pointer_cast<SMBContext>(ctx->priv_data);

    switch (smbc->version)
    {
    case SMBContext::SMB1:
        return smb_stat_list_read((smb_stat_list*)&dir);
    case SMBContext::SMB2_02:
    case SMBContext::SMB2_10:
    case SMBContext::SMB3_00:
    case SMBContext::SMB3_02:
    case SMBContext::SMB3_11:
        return smb2_readdir(smbc->smb2, (smb2dir*)dir);
    default:
        return nullptr;
    }
}

void smbc_resolve_file_stat(Context* ctx, void* ent, std::string& name, uint64_t& cTime, uint64_t& size, bool& isDir)
{
    std::shared_ptr<SMBContext> smbc = std::static_pointer_cast<SMBContext>(ctx->priv_data);

    switch (smbc->version)
    {
    case SMBContext::SMB1:
    {
        smb_stat st = (smb_stat)ent;
        name = smb_stat_name(st);
        cTime = smb_stat_get(st, SMB_STAT_CTIME) / 10000000 - 11644473600;
        size = smb_stat_get(st, SMB_STAT_SIZE);
        isDir = smb_stat_get(st, SMB_STAT_ISDIR);
    }
    break;
    case SMBContext::SMB2_02:
    case SMBContext::SMB2_10:
    case SMBContext::SMB3_00:
    case SMBContext::SMB3_02:
    case SMBContext::SMB3_11:
    {
        smb2_stat_64 st = ((smb2dirent*)ent)->st;
        name = ((smb2dirent*)ent)->name;
        cTime = st.smb2_ctime;
        size = st.smb2_size;
        isDir = st.smb2_type & SMB2_FILE_ATTRIBUTE_DIRECTORY;
    }
    break;
    default:
        break;
    }
}

void smbc_closedir(Context* ctx, void* files)
{
    std::shared_ptr<SMBContext> smbc = std::static_pointer_cast<SMBContext>(ctx->priv_data);

    switch (smbc->version)
    {
    case SMBContext::SMB1:
        smb_stat_list_destroy((smb_stat_list)files);
        break;
    case SMBContext::SMB2_02:
    case SMBContext::SMB2_10:
    case SMBContext::SMB3_00:
    case SMBContext::SMB3_02:
    case SMBContext::SMB3_11:
        smb2_closedir(smbc->smb2, (smb2dir*)files);
        break;
    default:
        break;
    }
}

//void* smbc_open(const char* file, int flag)
//{
//    if (version == SMB1)
//    {
//        smb_fd fd = 0;
//        smb_fopen(session, tid, file, flag, &fd);
//        return smb_stat_fd(session, fd);
//    }
//    else
//    {
//        return smb2_open(smb2, file, flag);
//    }
//}
//
//int64_t smbc_seek(void* fh, int64_t offset, int whence)
//{
//    if (version == SMB1)
//    {
//        return smb_fseek(session, SMB_FD(tid, ((smb_file*)fh)->fid), offset, whence);
//    }
//    else
//    {
//        return smb2_lseek(smb2, (smb2fh*)fh, offset, whence, nullptr);
//    }
//}
//
//int64_t smbc_get_file_size(void* fh)
//{
//    if (version == SMB1)
//    {
//        return smb_stat_get((smb_stat)fh, SMB_STAT_SIZE);
//    }
//    else
//    {
//        return smb2_get_file_size((smb2fh*)fh);
//    }
//}
//
//int smbc_get_max_read_size()
//{
//    if (version == SMB1)
//    {
//        return 0xffff;
//    }
//    else
//    {
//        return smb2_get_max_read_size(smb2);
//    }
//}
//
//int smbc_read(void* fh, uint8_t* buf, uint32_t len)
//{
//    if (version == SMB1)
//    {
//        return smb_fread(session, SMB_FD(tid, ((smb_file*)fh)->fid), buf, len);
//    }
//    else
//    {
//        return smb2_read(smb2, (smb2fh*)fh, buf, len);
//    }
//}
//
//void smbc_close(void* fh)
//{
//    if (version == SMB1)
//    {
//        smb_fclose(session, SMB_FD(tid, ((smb_file*)fh)->fid));
//    }
//    else
//    {
//        smb2_close(smb2, (smb2fh*)fh);
//    }
//}
