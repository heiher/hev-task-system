/* hev-win-netdb.c - Name resolution on top of Winsock */

#define HEV_WIN_RAW
#include "hev-sys-private.h"

int
hev_win_getaddrinfo (const char *node, const char *service,
                     const struct addrinfo *hints, struct addrinfo **res)
{
    int ret;

    if (hev_win_wsa_init () < 0)
        return EAI_FAIL;

    ret = getaddrinfo (node, service, hints, res);
    if (ret != 0)
        hev_win_set_errno_from_wsa_code (ret);

    hev_win_wsa_fini ();

    return ret;
}

int
hev_win_getnameinfo (const struct sockaddr *addr, socklen_t addrlen, char *node,
                     socklen_t nodelen, char *service, socklen_t servicelen,
                     int flags)
{
    int ret;

    if (hev_win_wsa_init () < 0)
        return EAI_FAIL;

    ret =
        getnameinfo (addr, addrlen, node, nodelen, service, servicelen, flags);
    if (ret != 0)
        hev_win_set_errno_from_wsa_code (ret);

    hev_win_wsa_fini ();

    return ret;
}

void
hev_win_freeaddrinfo (struct addrinfo *res)
{
    freeaddrinfo (res);
}
