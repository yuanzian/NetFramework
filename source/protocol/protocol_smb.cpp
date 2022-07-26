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
#endif

#include "protocol.h"
#include "protocol_smb.h"
#include "rules.h"
#include "source/logger.h"

using namespace std::chrono_literals;
#define SMB1_CONNECTION_TIMEOUT 7s
#define SMB2_CONNECTION_TIMEOUT (3s).count()
#define DESTROY_CONTEXT_DELAY_TIME (30s).count()

netbios_ns* ns = nullptr;
SMBContext::~SMBContext()
{
    if (session)
    {
        if (tid != 0)
            smb_tree_disconnect(session, tid);
        smb_session_destroy(session);
        session = nullptr;
    }
    if (smb2)
    {
        smb2_disconnect_share(smb2);
        smb2_destroy_context(smb2);
        smb2 = nullptr;
    }
}

extern const Protocol smb_protocol =
{
    .name = "smb",
    .init = smb_init,
    .discover = smb_discover,
    .browse = smb_browse
};

static int smb_init()
{
    if (ns != nullptr)
        return -1;
    ns = netbios_ns_new();
    return 0;
}

static int smb_finish()
{
    int ret = 0;
    if (ns)
    {
        ret = netbios_ns_discover_stop(ns);
        netbios_ns_destroy(ns);
        ns = nullptr;
    }
    return ret;
}

static int smb_discover(std::shared_ptr<void> priv_data)
{
    SMBContext& smbc = *static_cast<SMBContext*>(priv_data.get());

    const netbios_ns_discover_callbacks callback
    {
        .p_opaque = (void*)0x42,
        .pf_on_entry_added = [](void* p_opaque, netbios_ns_entry* entry)
        {
            logger::Log(logger::logLevel::Info, "Add device {}:{}", netbios_ns_entry_name(entry), netbios_ns_entry_ip_char(entry));
            //std::scoped_lock<std::mutex> lock(deviceQueueMutex);
            //queueAddDeviceInfo.emplace(netbios_ns_entry_ip_char(entry), netbios_ns_entry_name(entry));
        },
        .pf_on_entry_removed = [](void* p_opaque, netbios_ns_entry* entry)
        {
            logger::Log(logger::logLevel::Info, "Remove device {}:{}", netbios_ns_entry_name(entry), netbios_ns_entry_ip_char(entry));
            //std::scoped_lock<std::mutex> lock(deviceQueueMutex);
            //queueRemoveDeviceInfo.emplace(netbios_ns_entry_ip_char(entry), netbios_ns_entry_name(entry));
        },
    };

    return (netbios_ns_discover_start(ns, 6, const_cast<netbios_ns_discover_callbacks*>(&callback)) == 0);
}

static int smb_browse(std::shared_ptr<void> priv_data)
{
    SMBContext& smbc = *static_cast<SMBContext*>(priv_data.get());

    std::vector<std::tuple<std::string, uint64_t, uint64_t>> vectorVideoProperty;
    std::vector<std::string> vectorSubtitleProperty;
    std::vector<std::string> vectorPictureProperty;
    std::multimap<std::string, std::string> mapSubtitleProperty;
    std::multimap<std::string, std::string> mapThumbnailProperty;
    std::ostringstream xt;

    void* files = nullptr;
    void* file = nullptr;
    int loginErrorCode;

    /* <folderxml>
    *      <item type=["directory" | "file"]>
    *          <name></name>
    *          <date></date>
    *          <size></size>
    *          [<subtitle></subtitle>]
    *          [<thumbnail></thumbnail>]
    *      </item>
    * </folderxml>
    */

    xt << "<folderxml>";
    do {
        loginErrorCode = smbc.smbc_connect_share();
        if (loginErrorCode != SMBC_LOGIN_SUCCESS)
            break;

        if (smbc.share == "IPC$")
        {
            smb_share_list shareList;
            if (smbc.smbc_get_shareList(shareList) || !shareList)
            {
                logger::Log(logger::logLevel::Error, "Get share list failed, cause: {}", smbc.smbc_get_errorInfo());
                break;
            }

            for (size_t i = 0; shareList[i] != nullptr; ++i)
            {
                xt << "<item>" << shareList[i] << "</item>";
            }
            smb_share_list_destroy(shareList);
        }
        else
        {
            files = smbc.smbc_opendir();
            if (files == nullptr)
            {
                const char* errorMsg = smbc.smbc_get_errorInfo();
                if (smbc.version == SMBContext::SambaVersion::SMB1 || strstr(errorMsg, "STATUS_ACCESS_DENIED"))
                    loginErrorCode = SMBC_LOGIN_AUTHORITYERROR;
                logger::Log(logger::logLevel::Error, "Open dir failed while browsing folder, cause: %s", errorMsg);
                break;
            }

            while ((file = smbc.smbc_readdir(files)) != nullptr)
            {
                std::string name;
                uint64_t cTime;
                uint64_t size;
                bool isDir;
                smbc.smbc_resolve_file_stat(file, name, cTime, size, isDir);

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
            smbc.smbc_closedir(files);

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

            if (!vectorVideoProperty.empty() && !vectorPictureProperty.empty())
            {
                for (const auto& [video, cTime, size] : vectorVideoProperty)
                {
                    std::string videoWithoutSuffix = video.substr(0, video.find_last_of('.'));
                    for (const std::string& picture : vectorPictureProperty)
                    {
                        std::string pictureWithoutSuffix = picture.substr(0, picture.find_last_of('.'));
                        if (videoWithoutSuffix == pictureWithoutSuffix)
                        {
                            mapThumbnailProperty.emplace(video, picture);
                            break;
                        }
                    }
                }
            }

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
        }
    } while (0);
    xt << "</folderxml>";
    smbc.smbc_disconnect_share();
    logger::Log(logger::logLevel::Trace, "{}", xt.str());
    return 0;
}

LoginError SMBContext::smbc_connect_share()
{
    LoginError loginErrorCode = SMBC_LOGIN_SUCCESS;
    logger::Log(logger::logLevel::Info, "Trying to connect {} with SMB2 protocol.", server);
    smb2 = smb2_init_context();
    if (smb2 == nullptr)
    {
        logger::Log(logger::logLevel::Error, "Init SMB2 session failed: smb://{}:{}@{}/{}",
            userID, password, server, share);
        return SMBC_LOGIN_INITSESSIONERROR;
    }

connect:
    smb2_set_security_mode(smb2, SMB2_NEGOTIATE_SIGNING_ENABLED);
    smb2_set_timeout(smb2, SMB2_CONNECTION_TIMEOUT);

    if (smb2_connect_share(smb2, server.c_str(), share.c_str(), userID.c_str(), password.c_str()) < 0)
    {
        const char* errorMsg = smb2_get_error(smb2);
        if (strstr(errorMsg, "STATUS_LOGON_FAILURE") ||
            strstr(errorMsg, "STATUS_PASSWORD_EXPIRED") ||
            strstr(errorMsg, "STATUS_PASSWORD_MUST_CHANGE") ||
            strstr(errorMsg, "STATUS_ACCOUNT_EXPIRED") ||
            strstr(errorMsg, "STATUS_ACCOUNT_DISABLED"))
            loginErrorCode = SMBC_LOGIN_PASSWORDERROR;
        else if (strstr(errorMsg, "STATUS_ACCESS_DENIED"))
        {
            if (smb2_get_negotiate_version(smb2) == SMB2_VERSION_ANY || smb2_get_negotiate_version(smb2) == SMB2_VERSION_0311)
            {
                logger::Log(logger::logLevel::Warning, "Cur negotiate version is SMB2_VERSION_0311, try to use ANY_VERSION_EXCEPT_0311.");
                smb2_disconnect_share(smb2);
                smb2_destroy_context(smb2);
                smb2 = smb2_init_context();
                smb2_set_negotiate_version(smb2, SMB2_VERSION_ANYEXCEPT0311);
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

        logger::Log(logger::logLevel::Error, "Connecting SMB2 share failed, cause: {}", errorMsg);
    }

    if (loginErrorCode == SMBC_LOGIN_SUCCESS || loginErrorCode == SMBC_LOGIN_AUTHORITYERROR)
    {
        version = static_cast<SMBContext::SambaVersion>(smb2_get_dialect_version(smb2));

        logger::Log(loginErrorCode == SMBC_LOGIN_SUCCESS ? logger::logLevel::Info : logger::logLevel::Error,
            "Create SMB2 session: smb-v{:x}://{}:{}@{}/{}, return {}",
            static_cast<int>(version),
            userID,
            password,
            server,
            share,
            static_cast<int>(loginErrorCode));
        return loginErrorCode;
    }
    else if (loginErrorCode == SMBC_LOGIN_PROTOCOLERROR || loginErrorCode == SMBC_LOGIN_CONNECTIONERROR || loginErrorCode == SMBC_LOGIN_UNKNOWNERROR)
    {
        logger::Log(logger::logLevel::Info, "Trying to connect {} with SMB1 protocol.", server);
        loginErrorCode = SMBC_LOGIN_SUCCESS;
        unsigned int addr = 0;
        struct addrinfo* p_info = nullptr;
        std::promise<int> connectPromise;
        std::future<int> connectFuture = connectPromise.get_future();
        if (getaddrinfo(server.c_str(), nullptr, nullptr, &p_info) == 0 && p_info->ai_family == AF_INET)
        {
            struct sockaddr_in* in = (struct sockaddr_in*)p_info->ai_addr;
            addr = in->sin_addr.s_addr;
        }
        else
        {
            loginErrorCode = SMBC_LOGIN_CONNECTIONERROR;
            goto error;
        }

        session = smb_session_new();
        std::thread(
            [&](std::promise<int> res) {
                res.set_value(smb_session_connect(session, server.c_str(), addr, SMB_TRANSPORT_TCP));
            }
        , std::move(connectPromise)).detach();

        switch (connectFuture.wait_for(SMB1_CONNECTION_TIMEOUT))
        {
        case std::future_status::timeout:
            delayDeconstruct = time(nullptr) + DESTROY_CONTEXT_DELAY_TIME;
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

        smb_session_set_creds(session, server.c_str(), userID.c_str(), password.c_str());
        if (smb_session_login(session) != DSM_SUCCESS)
        {
            loginErrorCode = SMBC_LOGIN_PASSWORDERROR;
            goto error;
        }
        loginErrorCode = (LoginError)smb_tree_connect(session, share.data(), &tid);
        if (loginErrorCode != DSM_SUCCESS)
            goto error;

        if (smb_session_is_guest(session) == 1)
        {
            logger::Log(logger::logLevel::Info, "Logged in with Guest");
        }

        logger::Log(logger::logLevel::Info,
            "Connecting SMB1 server SUCCESS: smb1://{}:{}@{}/{}",
            userID,
            password,
            server,
            share);
        version = SMBContext::SambaVersion::SMB1;
        return loginErrorCode;
    }

error:
    logger::Log(logger::logLevel::Error, "Connecting SMB server failed: smb://{}:{}@{}/{}, return {}",
        userID,
        password,
        server,
        share,
        static_cast<int>(loginErrorCode));
    return loginErrorCode;
}

void SMBContext::smbc_disconnect_share()
{
    using enum SMBContext::SambaVersion;

    switch (version)
    {
    case SMB1:
        if (session)
        {
            if (tid != 0)
                smb_tree_disconnect(session, tid);
            smb_session_destroy(session);
            session = nullptr;
        }
    case SMB2_02:
    case SMB2_10:
    case SMB3_00:
    case SMB3_02:
    case SMB3_11:
        if (smb2)
        {
            smb2_disconnect_share(smb2);
            smb2_destroy_context(smb2);
            smb2 = nullptr;
        }
        break;
    default:
        return;
    }
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
int SMBContext::smbc_get_shareList(smb_share_list& shareList)
{
    using enum SMBContext::SambaVersion;

    switch (version)
    {
    case SMB1:
        return smb_share_get_list(session, tid, &shareList, nullptr);
    case SMB2_02:
    case SMB2_10:
    case SMB3_00:
    case SMB3_02:
    case SMB3_11:
        return (shareList = smb2_share_enum(smb2)) ? 0 : -1;
    default:
        return 0;
    }
}

const char* SMBContext::smbc_get_errorInfo()
{
    using enum SMBContext::SambaVersion;

    switch (version)
    {
    case SMB1:
        return "NTERRNO:";
    case SMB2_02:
    case SMB2_10:
    case SMB3_00:
    case SMB3_02:
    case SMB3_11:
        return smb2_get_error(smb2);
    default:
        return nullptr;
    }
}

void* SMBContext::smbc_opendir()
{
    using enum SMBContext::SambaVersion;

    switch (version)
    {
    case SMB1:
        return smb_find(session, tid, (path + "\\*").data());
    case SMB2_02:
    case SMB2_10:
    case SMB3_00:
    case SMB3_02:
    case SMB3_11:
        return smb2_opendir(smb2, path.data());
    default:
        return nullptr;
    }
}

void* SMBContext::smbc_readdir(void*& dir)
{
    using enum SMBContext::SambaVersion;

    switch (version)
    {
    case SMB1:
        return smb_stat_list_read((smb_stat_list*)&dir);
    case SMB2_02:
    case SMB2_10:
    case SMB3_00:
    case SMB3_02:
    case SMB3_11:
        return smb2_readdir(smb2, (smb2dir*)dir);
    default:
        return nullptr;
    }
}

void SMBContext::smbc_resolve_file_stat(void* ent, std::string& name, uint64_t& cTime, uint64_t& size, bool& isDir)
{
    using enum SMBContext::SambaVersion;

    switch (version)
    {
    case SMB1:
    {
        smb_stat st = (smb_stat)ent;
        name = smb_stat_name(st);
        cTime = smb_stat_get(st, SMB_STAT_CTIME) / 10000000 - 11644473600;
        size = smb_stat_get(st, SMB_STAT_SIZE);
        isDir = smb_stat_get(st, SMB_STAT_ISDIR);
    }
    break;
    case SMB2_02:
    case SMB2_10:
    case SMB3_00:
    case SMB3_02:
    case SMB3_11:
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

void SMBContext::smbc_closedir(void* files)
{
    using enum SMBContext::SambaVersion;

    switch (version)
    {
    case SMB1:
        smb_stat_list_destroy((smb_stat_list)files);
        break;
    case SMB2_02:
    case SMB2_10:
    case SMB3_00:
    case SMB3_02:
    case SMB3_11:
        smb2_closedir(smb2, (smb2dir*)files);
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
