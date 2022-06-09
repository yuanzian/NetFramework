
#include "protocol.h"

static int smb_discover()
{
    return 0;
}

static int smb_browse()
{
    return 0;
}

const Protocol smb_protocol =
{
    .name = "smb",
    .discover = smb_discover,
    .browse = smb_browse
};