// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2019 by The Odamex Team.
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
//	Playstation Vita network functions. It includes an unix-like recreation
//	of the network interface.
//
//	Credits to Rinnegatamante !
//
//-----------------------------------------------------------------------------

#include <vitasdk.h>
#include <dirent.h>
#include <unistd.h>

#include "i_psvita.h"

int convertSceNetSockaddrIn(struct SceNetSockaddrIn* src, struct sockaddr_in* dst){
	if (dst == NULL || src == NULL) return -1;
	dst->sin_family = src->sin_family;
	dst->sin_port = src->sin_port;
	dst->sin_addr.s_addr = src->sin_addr.s_addr;
	return 0;
}

int convertSockaddrIn(struct SceNetSockaddrIn* dst, const struct sockaddr_in* src){
	if (dst == NULL || src == NULL) return -1;
	dst->sin_family = src->sin_family;
	dst->sin_port = src->sin_port;
	dst->sin_addr.s_addr = src->sin_addr.s_addr;
	return 0;
}

int convertSceNetSockaddr(struct SceNetSockaddr* src, struct sockaddr* dst){
	if (dst == NULL || src == NULL) return -1;
	dst->sa_family = src->sa_family;
	memcpy(dst->sa_data,src->sa_data,14);
	return 0;
}

int convertSockaddr(struct SceNetSockaddr* dst, const struct sockaddr* src){
	if (dst == NULL || src == NULL) return -1;
	dst->sa_family = src->sa_family;
	memcpy(dst->sa_data,src->sa_data,14);
	return 0;
}

int vita_socket(int domain, int type, int protocol){
	return sceNetSocket("Socket", domain, type, protocol);
}

int vita_bind(int sockfd, const struct sockaddr* addr, unsigned int addrlen){
	struct SceNetSockaddr tmp;
	convertSockaddr(&tmp, addr);
	return sceNetBind(sockfd, &tmp, addrlen);
}

int vita_recvfrom(int sockfd, void* buf, long len, int flags, struct sockaddr* src_addr, unsigned int* addrlen){
	struct SceNetSockaddr tmp;
	int res = sceNetRecvfrom(sockfd, buf, len, flags, &tmp, addrlen);
	if (src_addr != NULL) convertSceNetSockaddr(&tmp, src_addr);
	return res;
}

int vita_getsockname(int sockfd, struct sockaddr *addr, unsigned int *addrlen){
	struct SceNetSockaddr tmp;
	convertSockaddr(&tmp, addr);
	int res = sceNetGetsockname(sockfd, &tmp, addrlen);
	convertSceNetSockaddr(&tmp, addr);
	return res;
}

int vita_close(int sockfd){
	return sceNetSocketClose(sockfd);
}

unsigned int vita_sendto(int sockfd, const void *buf, unsigned int len, int flags, const struct sockaddr *dest_addr, unsigned int addrlen){
	struct SceNetSockaddr tmp;
	convertSockaddr(&tmp, dest_addr);
	return sceNetSendto(sockfd, buf, len, flags, &tmp, addrlen);
}

// Copy-pasted from xyz code
hostent *vita_gethostbyname(const char *name)
{
    static struct hostent ent;
    static char sname[64] = "";
    static struct SceNetInAddr saddr = { 0 };
    static char *addrlist[2] = { (char *) &saddr, NULL };

    int rid;
    int err;
    rid = sceNetResolverCreate("resolver", NULL, 0);
    if(rid < 0) {
        return NULL;
    }

    err = sceNetResolverStartNtoa(rid, name, &saddr, 0, 0, 0);
    sceNetResolverDestroy(rid);
    if(err < 0) {
        return NULL;
    }

    ent.h_name = sname;
    ent.h_aliases = 0;
    ent.h_addrtype = SCE_NET_AF_INET;
    ent.h_length = sizeof(struct SceNetInAddr);
    ent.h_addr_list = addrlist;
    ent.h_addr = addrlist[0];

    return &ent;
}

int vita_gethostname(char *name, size_t namelen)
{
	return 0;
}

char *vita_inet_ntoa(struct in_addr in)
{
    static char buf[32];
    SceNetInAddr addr;
    addr.s_addr = in.s_addr;
    sceNetInetNtop(SCE_NET_AF_INET, &addr, buf, sizeof(buf));
    return buf;
}

int vita_ioctl(int32_t s, uint32_t cmd, void *argp) {
	return 0;
}

int vita_select(short maxfdp1,fd_set *readset,fd_set *writeset,fd_set *exceptset,struct timeval *timeout)
{
	return -1;
}
