#include "protocol.h"

extern const Protocol smb_protocol;
extern const Protocol dlna_protocol;

Protocol FindProtocol(const std::string& name)
{
    if (name == "smb")
        return smb_protocol;
    else if (name == "dlna")
        return dlna_protocol;
}
