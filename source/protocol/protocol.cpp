#include "protocol.h"
#include "../logger.h"

extern const Protocol smb_protocol;
extern const Protocol dlna_protocol;

const std::map<const std::string, const Protocol&> protocols
{
    {"dlna",dlna_protocol},
    {"smb",smb_protocol}
};

void InitProtocols()
{
    for (auto& it : protocols)
    {
        auto& init_func = it.second.init;
        if (init_func)
            std::invoke(init_func);
    }
}

std::optional<const Protocol> FindProtocol(const std::string& name)
{
    return protocols.contains(name) ? std::optional<const Protocol>{protocols.at(name)} : std::nullopt;
}
