// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2012 by The Odamex Team.
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
//	Wii Remote Debugging Support
//
// AUTHORSHIP:
//  This code was originally written by Qiang Lin <qiang0@gmail.com>.
//  He provided this code to the community without licensing here:
//         http://wiibrew.org/wiki/User:Qiang0/Debugging
//  I e-mailed the author and requested permission to include his code
//  in Odamex licensed under the GPL. The author granted that permission
//  through e-mail exchange on April 27, 2010. -- Hyper_Eye
// 
//-----------------------------------------------------------------------------
#ifdef GEKKO
#ifdef DEBUG

#include <sys/types.h>
#include <network.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "net_print.h"

/* It opens a connection to the host name rhost that is listening
   on the specified port. It returns the socket descriptor, or -1
   in case of failure.
 */
static int clientsocket(const char *rhost, unsigned short port)
{
  struct hostent *ptrh;  /* pointer to a host table entry     */
  struct sockaddr_in sad;/* structure to hold server's address*/
  int    fd;             /* socket descriptor                 */

  memset((char *)&sad, 0, sizeof(sad)); /* clear sockaddr structure */
  sad.sin_family = AF_INET;  /* set family to Internet */
  sad.sin_port = htons((u_short)port); 
  /* Convert host name to equivalent IP address and copy sad */
  ptrh = net_gethostbyname(rhost);
  if (((char *)ptrh) == NULL) {
    fprintf(stderr, "invalid host: %s\n", rhost);
    return (-1);
  }
  memcpy(&sad.sin_addr, ptrh->h_addr, ptrh->h_length);
  
  /* Create a socket */
  fd = net_socket(PF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    fprintf(stderr, "socket creation failed\n");
    return (-1);;
  }
  
  /* Connect the socket to the specified server */
  if (net_connect(fd, (struct sockaddr *)&sad, sizeof(sad)) < 0) {
    fprintf(stderr, "connect failed\n");
    return (-1);
  }

  return fd;
}

static int _net_print_socket = -1;

int net_print_init(const char *rhost, unsigned short port)
{
	int	sk = -1;
	int	wd = 0x12345678;

	if ( _net_print_socket < 0 ) {
		if (rhost==NULL){
			rhost = DEFAULT_NET_PRINT_HOST;
		}
		if (port <= 0 ){
			port = DEFAULT_NET_PRINT_PORT;
		}

		sk = clientsocket( rhost, port);
		if ( sk >= 0 ) {
			_net_print_socket = sk;
			net_print_string( __FILE__, __LINE__, "net_print_init() successful, socket=%d, testing for hi-low-byte using int 0x12345678:\n", _net_print_socket);

 	  		net_print_binary( 'X', &wd, sizeof(wd));
		}
	}


	return _net_print_socket;
}

int net_print_string( const char* file, int line, const char* format, ...)
{
	va_list	ap;
	int len;
	int ret;
	char buffer[512];

	va_start(ap, format);

	if ( _net_print_socket < 0 ) {
		return	-1;
	}

	len = 0;
	if ( file != NULL) {
		len = sprintf( buffer, "%s:%d, ", file, line);
	}

	len += vsprintf( buffer+len, format, ap);
	va_end(ap);

	ret = net_send( _net_print_socket, buffer, len, 0);
	return ret;
}

int net_print_binary( int format, const void* data, int len)
{
	int col, k, ret;
	char line[80], *out;
	const unsigned char *in;
	const char* binary = (const char *)data;

	if ( _net_print_socket < 0 ) {
		return	-1;
	}

	in = (unsigned char*)binary;
	for( k=0; k<len ; ) {
		out = line;
		switch( format ) {
		case 'x':
			for( col=0; col<8 && k<len && ((len-k)/2>0); col++, k+=2) {
				ret = sprintf( out, "%02X%02X ", *in, *(in+1));
				if ( col==3) {
					strcat( out++, " ");
				}
				out += ret;
				in+=2;
			}
			break;
		case 'X':
			for( col=0; col<4 && k<len && ((len-k)/4>0); col++, k+=4) {
				ret = sprintf( out, "%02X%02X%02X%02X ", *in, *(in+1), *(in+2), *(in+3));
				if ( col==1) {
					strcat( out++, " ");
				}
				out += ret;
				in+=4;
			}
			break;
				
		case 'c':
			for( col=0; col<16 && k<len; col++, k++) {
				if ( isprint( *in) && !isspace(*in)) {
					ret = sprintf( out, "%c  ", *in);
				} else {
					ret = sprintf( out, "%02X ", *in);
				}
				if ( col==7) {
					strcat( out++, " ");
				}
				out += ret;
				in++;
			}
			break;

		default:
			for( col=0; col<16 && k<len; col++, k++) {
				ret = sprintf( out, "%02X ", *in);
				if ( col==7) {
					strcat( out++, " ");
				}
				out += ret;
				in++;
			}
		}
		if ( out != line ) {
			strcat( out, "\n");
			net_print_string( NULL, 0, "%s", line);
		} else {
			break;
		}
	}
/***
	if ( out!=line) {
		strcat( out, "\n");
		net_print_string( NULL, 0, "%s", line);
	}
***/
	return len;
}

#endif
#endif
