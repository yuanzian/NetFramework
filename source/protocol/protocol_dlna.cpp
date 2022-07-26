﻿#include <fstream>
#include <memory>
#include <mutex>
#include <regex>
#include <cstdarg>

#include "upnpapi.h"
#include "ixml.h"
#include "upnptools.h"

#include "protocol.h"
#include "../logger.h"


#if _WIN64
//#include <socket.h>
#define gettid() GetCurrentThreadId()
#endif
#define LOG_LINE_SIZE 1024
#if LOG_LINE_SIZE <= 64
#error "TOO SMALL BUFFER SIZE MAY CAUSE FATAL ERROR."
#elif LOG_LINE_SIZE <= 256
#warning "MUST ENSURE ALL THE BUFFER NOT TO OVERFLOW."
#endif
namespace DLNA {

}


struct DLNAContext
{
    std::string uuid;
    std::string objid;
    std::string friendlyName;
    std::string manufacturer;
    std::string location;
};


struct UpnpDevice
{
    std::string UDN;
    std::string friendlyName;
    std::string location;
    std::string iconUrl;
    std::string manufacturer;
    enum DeviceType
    {
        UnknownDevice = 0,
        MediaServer = 1,
        MediaRenderer = 2,
    }deviceType;

    UpnpDevice(const UpnpDevice& other)
        : UDN(other.UDN)
        , friendlyName(other.friendlyName)
        , location(other.location)
        , iconUrl(other.iconUrl)
        , manufacturer(other.manufacturer)
        , deviceType(other.deviceType)
    {
    }

    UpnpDevice(const std::string& udn)
        : UDN(udn)
        , deviceType(UnknownDevice)
    {
    }

    UpnpDevice(const std::string& udn, const std::string& friendlyName, const std::string& location, const std::string& iconUrl, const std::string& manufacturer)
        : UDN(udn)
        , friendlyName(friendlyName)
        , location(location)
        , iconUrl(iconUrl)
        , manufacturer(manufacturer)
        , deviceType(UnknownDevice)
    {
    }
};

static const char* MEDIA_SERVER_DEVICE_TYPE = "urn:schemas-upnp-org:device:MediaServer:1";
static const char* CONTENT_DIRECTORY_SERVICE_TYPE = "urn:schemas-upnp-org:service:ContentDirectory:1";
UpnpClient_Handle handle;
volatile bool isDLNAModuleRunning = false;

std::mutex UpnpDeviceMapMutex;
std::map<std::string, UpnpDevice> UpnpDeviceMap;

static void ParseNewServer(IXML_Document* doc, const char* location)
{
    if (!doc || !location)
        return;

    const char* baseURL = location;
    /* Try to extract baseURL */
    IXML_NodeList* urlList = ixmlDocument_getElementsByTagName(doc, "URLBase");
    if (urlList)
    {
        if (IXML_Node* urlNode = ixmlNodeList_item(urlList, 0))
        {
            IXML_Node* firstUrlNode = ixmlNode_getFirstChild(urlNode);
            if (firstUrlNode)
                baseURL = ixmlNode_getNodeValue(firstUrlNode);
        }
        ixmlNodeList_free(urlList);
    }

    /* Get devices */
    IXML_NodeList* deviceList = ixmlDocument_getElementsByTagName(doc, "device");
    if (!deviceList)
        return;

    for (unsigned int i = 0; i < ixmlNodeList_length(deviceList); i++)
    {
        IXML_Element* device = (IXML_Element*)ixmlNodeList_item(deviceList, i);
        if (!device)
            continue;

        const char* deviceType = ixmlElement_getFirstChildElementValue(device, "deviceType");
        if (!deviceType)
        {
            continue;
        }

        const char* udn = ixmlElement_getFirstChildElementValue(device, "UDN");
        {
            std::lock_guard<std::mutex> lock(UpnpDeviceMapMutex);
            if (!udn || UpnpDeviceMap.find(udn) != UpnpDeviceMap.end())
                continue;
        }

        const char* friendlyName = ixmlElement_getFirstChildElementValue(device, "friendlyName");
        if (!friendlyName)
        {
            continue;
        }

        const char* manufacturer = ixmlElement_getFirstChildElementValue(device, "manufacturer");
        std::string manufacturerString = manufacturer ? manufacturer : "";
        std::string iconUrl/* = GetIconURL(device, baseURL)*/;

        {
            std::lock_guard<std::mutex> lock(UpnpDeviceMapMutex);
            if (UpnpDeviceMap.find(udn) == UpnpDeviceMap.end())
            {
                UpnpDeviceMap.emplace(std::piecewise_construct, std::forward_as_tuple(udn),
                    std::forward_as_tuple(udn, friendlyName, location, iconUrl, manufacturerString));
                logger::Log(logger::logLevel::Info, "Device found: DeviceType={}, UDN={}, Name={}", deviceType, udn, friendlyName);
            }
        }

        /* Check for ContentDirectory service. */
        IXML_NodeList* serviceList = ixmlElement_getElementsByTagName(device, "service");
        if (!serviceList)
            continue;

        for (unsigned int j = 0; j < ixmlNodeList_length(serviceList); j++)
        {
            IXML_Element* service = (IXML_Element*)ixmlNodeList_item(serviceList, j);

            const char* serviceType = ixmlElement_getFirstChildElementValue(service, "serviceType");
            if (!serviceType || strncmp(CONTENT_DIRECTORY_SERVICE_TYPE, serviceType, strlen(CONTENT_DIRECTORY_SERVICE_TYPE) - 1))
            {
                continue;
            }

            const char* controlURL = ixmlElement_getFirstChildElementValue(service, "controlURL");
            if (!controlURL)
            {
                continue;
            }

            /* Try to browse content directory. */
            logger::Log(logger::logLevel::Info, "{} support service:{}, BaseURL={}, ControlURL={}", friendlyName, serviceType, baseURL, controlURL);
            std::lock_guard<std::mutex> lock(UpnpDeviceMapMutex);
            auto itr = UpnpDeviceMap.find(udn);
            if (itr != UpnpDeviceMap.end())
                itr->second.deviceType = UpnpDevice::DeviceType::MediaServer;

            char* url = (char*)malloc(strlen(baseURL) + strlen(controlURL) + 1);
            if (!url)
                continue;

            int ret = UpnpResolveURL(baseURL, controlURL, url);
            if (ret == UPNP_E_SUCCESS)
            {
                logger::Log(logger::logLevel::Info, "UpnpResolveURL success, add device {}", friendlyName);
                itr->second.location = url;
                //std::lock_guard<std::mutex> deviceQueueLock(deviceQueueMutex);
                //queueAddDeviceInfo.emplace(std::make_shared<UpnpDevice>(itr->second));
            }
            else logger::Log(logger::logLevel::Error, "UpnpResolveURL return {}, error: {}", ret, errno);
            free(url);
        }
        ixmlNodeList_free(serviceList);
    }
    ixmlNodeList_free(deviceList);
}

static void RemoveServer(const char* udn)
{
    if (!udn)
        return;

    {
        std::lock_guard<std::mutex> lock(UpnpDeviceMapMutex);
        auto it = UpnpDeviceMap.find(udn);
        if (it != UpnpDeviceMap.end())
            UpnpDeviceMap.erase(it);
    }

    //std::lock_guard<std::mutex> lock(deviceQueueMutex);
    //GetInstance().queueRemoveDeviceInfo.emplace(std::make_shared<UpnpDevice>(udn));
}

static int UpnpRegisterClientCallback(Upnp_EventType eventType, const void* event, void* cookie)
{
    switch (eventType)
    {
    case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
    case UPNP_DISCOVERY_SEARCH_RESULT:
    {
        UpnpDiscovery* discoverResult = (UpnpDiscovery*)event;
        IXML_Document* description = nullptr;
        int res = UpnpDownloadXmlDoc(UpnpString_get_String(UpnpDiscovery_get_Location(discoverResult)), &description);
        if (res != UPNP_E_SUCCESS)
            return res;

        ParseNewServer(description, UpnpString_get_String(UpnpDiscovery_get_Location(discoverResult)));
        ixmlDocument_free(description);
    }
    break;

    case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
    {
        UpnpDiscovery* discoverResult = (UpnpDiscovery*)event;
        RemoveServer(UpnpString_get_String(UpnpDiscovery_get_DeviceID(discoverResult)));
    }
    break;

    case UPNP_DISCOVERY_SEARCH_TIMEOUT:
    case UPNP_EVENT_RECEIVED:
    case UPNP_EVENT_SUBSCRIBE_COMPLETE:
    case UPNP_EVENT_AUTORENEWAL_FAILED:
    case UPNP_EVENT_SUBSCRIPTION_EXPIRED:
        break;

    default:
        break;
    }

    return UPNP_E_SUCCESS;
}

static std::string BrowseAction(const char* objectID,
    const char* flag,
    const char* filter,
    const char* startingIndex,
    const char* requestCount,
    const char* sortCriteria,
    const char* controlUrl)
{
    IXML_Document* actionDoc = nullptr;
    IXML_Document* browseResultXMLDocument = nullptr;
    char* rawXML = nullptr;
    std::string browseResultString;

    int res = UpnpAddToAction(&actionDoc, "Browse",
        CONTENT_DIRECTORY_SERVICE_TYPE, "ObjectID", objectID);

    if (res != UPNP_E_SUCCESS)
    {
        goto browseActionCleanup;
    }

    res = UpnpAddToAction(&actionDoc, "Browse",
        CONTENT_DIRECTORY_SERVICE_TYPE, "BrowseFlag", flag);

    if (res != UPNP_E_SUCCESS)
    {
        goto browseActionCleanup;
    }

    res = UpnpAddToAction(&actionDoc, "Browse",
        CONTENT_DIRECTORY_SERVICE_TYPE, "Filter", filter);

    if (res != UPNP_E_SUCCESS)
    {
        goto browseActionCleanup;
    }

    res = UpnpAddToAction(&actionDoc, "Browse",
        CONTENT_DIRECTORY_SERVICE_TYPE, "StartingIndex", startingIndex);
    if (res != UPNP_E_SUCCESS)
    {
        goto browseActionCleanup;
    }

    res = UpnpAddToAction(&actionDoc, "Browse",
        CONTENT_DIRECTORY_SERVICE_TYPE, "RequestedCount", requestCount);

    if (res != UPNP_E_SUCCESS)
    {
        goto browseActionCleanup;
    }

    res = UpnpAddToAction(&actionDoc, "Browse",
        CONTENT_DIRECTORY_SERVICE_TYPE, "SortCriteria", sortCriteria);

    if (res != UPNP_E_SUCCESS)
    {
        goto browseActionCleanup;
    }

    res = UpnpSendAction(handle,
        controlUrl,
        CONTENT_DIRECTORY_SERVICE_TYPE,
        nullptr, /* ignored in SDK, must be NULL */
        actionDoc,
        &browseResultXMLDocument);

    if (res || !browseResultXMLDocument)
    {
        logger::Log(logger::logLevel::Error, "UpnpSendAction return {}", UpnpGetErrorMessage(res));
        goto browseActionCleanup;
    }

    rawXML = ixmlDocumenttoString(browseResultXMLDocument);
    if (rawXML != nullptr)
    {
        browseResultString = rawXML;
        ixmlFreeDOMString(rawXML);
    }

browseActionCleanup:
    if (browseResultXMLDocument)
        ixmlDocument_free(browseResultXMLDocument);

    ixmlDocument_free(actionDoc);
    return browseResultString;
}

#if _WIN64
static char8_t* GetBestAdapterInterfaceName()
{
    wchar_t psz_uri[32];
    ULONG size = 32;
    ULONG adapts_size = 0;
    PIP_ADAPTER_ADDRESSES adapts_item;
    PIP_ADAPTER_ADDRESSES adapts = nullptr;
    PIP_ADAPTER_UNICAST_ADDRESS p_best_ip = nullptr;
    PIP_ADAPTER_ADDRESSES bestAdapter = nullptr;

    /* Get Adapters addresses required size. */
    int ret = GetAdaptersAddresses(AF_UNSPEC,
        GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_DNS_SERVER,
        NULL,
        adapts,
        &adapts_size);
    if (ret != ERROR_BUFFER_OVERFLOW)
    {
        logger::Log(logger::logLevel::Error, "GetAdaptersAddresses failed to find list of adapters");
        return NULL;
    }

    /* Allocate enough memory. */
    adapts = (PIP_ADAPTER_ADDRESSES)malloc(adapts_size);
    ret = GetAdaptersAddresses(AF_UNSPEC,
        GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_DNS_SERVER,
        NULL,
        adapts,
        &adapts_size);
    if (ret != 0)
    {
        logger::Log(logger::logLevel::Error, "GetAdaptersAddresses failed to find list of adapters");
        return NULL;
    }

    /* find one with multicast capabilities */
    for (adapts_item = adapts; adapts_item != NULL; adapts_item = adapts_item->Next)
    {
        if (adapts_item->Flags & IP_ADAPTER_NO_MULTICAST ||
            adapts_item->OperStatus != IfOperStatusUp)
            continue;

        /* make sure it supports 239.255.255.250 */
        for (PIP_ADAPTER_MULTICAST_ADDRESS p_multicast = adapts_item->FirstMulticastAddress;
            p_multicast != NULL;
            p_multicast = p_multicast->Next)
        {
            if (((struct sockaddr_in*)p_multicast->Address.lpSockaddr)->sin_addr.S_un.S_addr == inet_addr("239.255.255.250"))
            {
                /* get an IPv4 address */
                for (PIP_ADAPTER_UNICAST_ADDRESS p_unicast = adapts_item->FirstUnicastAddress;
                    p_unicast != NULL;
                    p_unicast = p_unicast->Next)
                {
                    //size = sizeof(psz_uri) / sizeof(wchar_t);
                    //if (WSAAddressToString(p_unicast->Address.lpSockaddr,
                    //    p_unicast->Address.iSockaddrLength,
                    //    NULL, psz_uri, &size) == 0)
                    if (p_best_ip == NULL || p_best_ip->ValidLifetime > p_unicast->ValidLifetime)
                    {
                        p_best_ip = p_unicast;
                        bestAdapter = adapts_item;
                    }
                }
                break;
            }
        }
    }

    if (p_best_ip != NULL)
        goto done;

    /* find any with IPv4 */
    for (adapts_item = adapts; adapts_item != NULL; adapts_item = adapts_item->Next)
    {
        if (adapts_item->Flags & IP_ADAPTER_NO_MULTICAST ||
            adapts_item->OperStatus != IfOperStatusUp)
            continue;

        for (PIP_ADAPTER_UNICAST_ADDRESS p_unicast = adapts_item->FirstUnicastAddress;
            p_unicast != NULL;
            p_unicast = p_unicast->Next)
        {
            if (p_best_ip == NULL || p_best_ip->ValidLifetime > p_unicast->ValidLifetime)
            {
                p_best_ip = p_unicast;
                bestAdapter = adapts_item;
            }
        }
    }

done:
    if (p_best_ip != NULL)
    {
        size = sizeof(psz_uri) / sizeof(wchar_t);
        WSAAddressToString(p_best_ip->Address.lpSockaddr,
            p_best_ip->Address.iSockaddrLength,
            NULL, psz_uri, &size);
        char tmpIp[32] = { 0 };
        wcstombs(tmpIp, psz_uri, size);

        char8_t* tmpIfName = (char8_t*)calloc(LINE_SIZE, sizeof(char8_t));
        int ret = WideCharToMultiByte(CP_UTF8, 0, bestAdapter->FriendlyName, -1, NULL, 0, NULL, NULL);
        ret = WideCharToMultiByte(CP_UTF8, 0, bestAdapter->FriendlyName, ret, reinterpret_cast<char*>(tmpIfName), ret, NULL, NULL);
        logger::Log(logger::logLevel::Info, "Get the best ip is {}, ifname is {}", tmpIp, reinterpret_cast<char*>(tmpIfName));
        free(adapts);
        return tmpIfName;
    }
    free(adapts);
    return NULL;
}
#endif

static int dlna_init()
{
#if __ANDROID__ || __linux__
    int res = UpnpInit2(nullptr, 0);
#elif _WIN64
    UpnpSetLogLevel(Upnp_LogLevel::UPNP_ERROR);
    UpnpInitLog();
    char8_t* bestAdapterName = GetBestAdapterInterfaceName();
    int res = UpnpInit2(reinterpret_cast<char*>(bestAdapterName), 0);
    free(bestAdapterName);
#endif
    if (res != UPNP_E_SUCCESS)
    {
        logger::Log(logger::logLevel::Error, "Upnp SDK Init error {}", UpnpGetErrorMessage(res));
        return res;
    }
    logger::Log(logger::logLevel::Info, "Upnp SDK init success");
    ixmlRelaxParser(1);

    /* Register a control point */
    res = UpnpRegisterClient(UpnpRegisterClientCallback, nullptr, &handle);
    if (res != UPNP_E_SUCCESS)
    {
        logger::Log(logger::logLevel::Error, "Upnp control point register failed, return {}", UpnpGetErrorMessage(res));
        return res;
    }
    logger::Log(logger::logLevel::Info, "Upnp control point register success, handle is {}", handle);
    UpnpSetMaxContentLength(INT_MAX);
    isDLNAModuleRunning = true;
    return res;
}

static int dlna_finish()
{
    int res = UpnpUnRegisterClient(handle);
    if (res != UPNP_E_SUCCESS)
    {
        logger::Log(logger::logLevel::Error, "Upnp control point unregister failed, return {}", UpnpGetErrorMessage(res));
        return res;
    }

    res = UpnpFinish();
    if (res != UPNP_E_SUCCESS)
    {
        logger::Log(logger::logLevel::Error, "Upnp SDK finished failed");
        return res;
    }
    logger::Log(logger::logLevel::Info, "Upnp SDK finished success");
    isDLNAModuleRunning = false;
    return res;
}

static int dlna_discover(std::shared_ptr<void> ctx)
{
    /* Search for media servers */
    int res = UpnpSearchAsync(handle, MAX_SEARCH_TIME, MEDIA_SERVER_DEVICE_TYPE, nullptr);
    if (res != UPNP_E_SUCCESS)
    {
        logger::Log(logger::logLevel::Error, "Searching server failed, return {}", UpnpGetErrorMessage(res));
        return res;
    }
    logger::Log(logger::logLevel::Info, "Searching server success");
    return res;
}

static int dlna_browse(std::shared_ptr<void> ctx)
{
    std::shared_ptr<DLNAContext> dlnac = std::static_pointer_cast<DLNAContext>(ctx);

    std::string res = BrowseAction(dlnac->objid.c_str(), "BrowseDirectChildren", "*", "0", "10000", "", dlnac->location.data());

    res = std::regex_replace(res, std::regex{ "&amp;" }, "&");
    res = std::regex_replace(res, std::regex{ "&quot;" }, "\"");
    res = std::regex_replace(res, std::regex{ "&gt;" }, ">");
    res = std::regex_replace(res, std::regex{ "&lt;" }, "<");
    res = std::regex_replace(res, std::regex{ "&apos;" }, "'");
    res = std::regex_replace(res, std::regex{ "<unknown>" }, "unknown");

    if (dlnac->manufacturer != "Microsoft Corporation")
    {
        res = std::regex_replace(res, std::regex{ R"((pv:subtitleFileType=")([^"]*)(")|(pv:subtitleFileUri=")([^"]*)("))" }, "");
        res = std::regex_replace(res, std::regex{ "\xc3\x97" }, "x");
    }

    IXML_Document* parseDoc = ixmlParseBuffer(res.data());
    if (parseDoc == nullptr)
    {
        logger::Log(logger::logLevel::Error, "Parse result to XML format failed");
        res.clear();
    }
    else
    {
        char* tmp = ixmlDocumenttoString(parseDoc);
        res = tmp;
        ixmlFreeDOMString(tmp);
    }

    if (res.empty())
        logger::Log(logger::logLevel::Error, "Browse failed");
    else
        logger::Log(logger::logLevel::Info, "{}", res);
    return 0;
}



extern const Protocol dlna_protocol =
{
    .name = "dlna",
    .init = dlna_init,
    .finish = dlna_finish,
    .discover = dlna_discover,
    .browse = dlna_browse
};