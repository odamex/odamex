#ifndef _I_WIIU_H
#define _I_WIIU_H

#ifdef __WIIU__

#include <nsysnet/socket.h>

#define WII_DATAPATH "sd:/odx_data/"

#define FIONBIO SO_BIO

#define gethostbyname wiiu_gethostbyname
#define gethostname wiiu_gethostname
#define ioctl wiiu_ioctl

// Ch0wW : THIS MUST BE REMOVED ONCE WE HAVE PROPER SUPPORT FOR THIS
struct hostent {
	char    *h_name;        /* official name of host */
	char    **h_aliases;    /* alias list */
	uint16_t     h_addrtype;     /* host address type */
	uint16_t     h_length;       /* length of address */
	char    **h_addr_list;  /* list of addresses from name server */
};

int wiiu_getsockname(int socket, struct sockaddr *address, socklen_t *address_len);
int wiiu_gethostname(char *name, size_t namelen);

hostent *wiiu_gethostbyname(const char *addrString);
int32_t wiiu_ioctl(int32_t s, uint32_t cmd, void *argp);

#endif
#endif