// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2015 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//
// DESCRIPTION:
//	Wii Support
//
//-----------------------------------------------------------------------------

#ifndef _I_PSVITA_H
#define _I_PSVITA_H

#define VITA_DATAPATH "ux0:/data/odamex"

#define PathIsRelative vita_pathisrelative
#define scandir vita_scandir
#define alphasort vita_alphasort

bool vita_pathisrelative(const char *path);

/*int wii_getsockname(int socket, struct sockaddr *address, socklen_t *address_len);
int wii_gethostname(char *name, size_t namelen);*/

int vita_scandir(const char *dir, struct  dirent ***namelist,
							int (*select)(const struct dirent *),
							int (*compar)(const struct dirent **, const struct dirent **));
int vita_alphasort(const struct dirent **a, const struct dirent **b);

#define mkdir sceIoMkdir

//--------------------------------------
// NETWORK-RELATED DEFINES
//--------------------------------------
struct in_addr {
    unsigned long s_addr;  // load with inet_aton()
};

struct sockaddr_in {
    short            sin_family;   // e.g. AF_INET
    unsigned short   sin_port;     // e.g. htons(3490)
    struct in_addr   sin_addr;     // see struct in_addr, below
    char             sin_zero[8];  // zero this if you want to
};

struct sockaddr {
    unsigned short    sa_family;    // address family, AF_xxx
    char              sa_data[14];  // 14 bytes of protocol address
};

typedef unsigned int socklen_t;

#define AF_INET SCE_NET_AF_INET
#define PF_INET AF_INET

#define SOCK_DGRAM SCE_NET_SOCK_DGRAM
#define IPPROTO_UDP SCE_NET_IPPROTO_UDP
#define SOL_SOCKET SCE_NET_SOL_SOCKET
#define MSG_PEEK SCE_NET_MSG_PEEK
#define INADDR_BROADCAST SCE_NET_INADDR_BROADCAST
#define SO_BROADCAST SCE_NET_SO_BROADCAST
#define INADDR_ANY SCE_NET_INADDR_ANY
#define SO_NBIO SCE_NET_SO_NBIO
#define FIONBIO SO_NBIO

struct hostent {
  char    *h_name;        /* official name of host */
  char    **h_aliases;    /* alias list */
  unsigned short   h_addrtype;     /* host address type */
  unsigned short   h_length;       /* length of address */
  char    **h_addr_list;  /* list of addresses from name server */
  char* 	h_addr;
};

//---

#define socket vita_socket
#define bind vita_bind
#define close vita_close
#define gethostname vita_gethostname
#define gethostbyname vita_gethostbyname
#define getsockname vita_getsockname
#define recvfrom vita_recvfrom
#define sendto vita_sendto
#define select vita_select
#define ioctl vita_ioctl
#define inet_ntoa vita_inet_ntoa
#define htons sceNetHtons
#define ntohs sceNetNtohs

int vita_socket(int domain, int type, int protocol);
int vita_recvfrom(int sockfd, void* buf, long len, int flags, struct sockaddr* src_addr, unsigned int* addrlen);
int vita_getsockname(int sockfd, struct sockaddr *addr, unsigned int *addrlen);
int vita_bind(int sockfd, const struct sockaddr* addr, unsigned int addrlen);
int vita_close(int sockfd);
hostent *vita_gethostbyname(const char *name);
unsigned int vita_sendto(int sockfd, const void *buf, unsigned int len, int flags, const struct sockaddr *dest_addr, unsigned int addrlen);

//--
int vita_gethostname(char *name, size_t namelen);
char *vita_inet_ntoa(struct in_addr in);
int vita_ioctl(int32_t s, uint32_t cmd, void *argp);
int vita_select(short maxfdp1,fd_set *readset,fd_set *writeset,fd_set *exceptset,struct timeval *timeout);


//---------------------

unsigned int vita_sleep(unsigned int seconds);
int vita_usleep(useconds_t usec);

#define sleep vita_sleep
#define usleep vita_usleep

#endif
