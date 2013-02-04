// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2005 by Simon Howard
// Copyright (C) 2006-2010 by The Odamex Team
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
// DESCRIPTION:
//	[Russell] - Added some functions and cleaned up a few areas
//    
//-----------------------------------------------------------------------------


#ifndef MEMIO_H
#define MEMIO_H

// returns the length of an "c array" on the stack.
#define STACKARRAY_LENGTH(a) ((sizeof(a) / sizeof(a[0])))

typedef struct _MEMFILE MEMFILE;

typedef enum 
{
	MEM_SEEK_SET,
	MEM_SEEK_CUR,
	MEM_SEEK_END
} mem_rel_t;

MEMFILE *mem_fopen_read(void *buf, size_t buflen);
size_t mem_fread(void *buf, size_t size, size_t nmemb, MEMFILE *stream);
MEMFILE *mem_fopen_write(void);
size_t mem_fwrite(const void *ptr, size_t size, size_t nmemb, MEMFILE *stream);
void mem_get_buf(MEMFILE *stream, void **buf, size_t *buflen);
void mem_fclose(MEMFILE *stream);
long mem_ftell(MEMFILE *stream);
int mem_fseek(MEMFILE *stream, signed long offset, mem_rel_t whence);
size_t mem_fsize(MEMFILE *stream); // [Russell] - get size of stream
char *mem_fgetbuf(MEMFILE *stream); // [Russell] - return stream buffer
#endif /* #ifndef MEMIO_H */
	  
