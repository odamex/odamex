// Copyright (C) 2020 Alex Mayfield
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

///
// A little "toy" buffer implementation for demonstration purposes.
//

unsigned char g_buffer[128];
size_t g_rpos = 0;
size_t g_wpos = 0;
bool overflowed = false;

void Reset()
{
	g_rpos = 0;
	g_wpos = 0;
}

void WriteByte(unsigned char b)
{
	g_buffer[g_wpos++] = b;
}

unsigned char ReadByte()
{
	return g_buffer[g_rpos++];
}

//
// Write an unsigned varint to the wire.
//
// Use these when you want to send an int and are reasonably sure that
// the int will usually be small.
//
// https://developers.google.com/protocol-buffers/docs/encoding#varints
//
void WriteUnVarint(unsigned int v)
{
	for (;;)
	{
		// Our next byte contains 7 bits of the number.
		int out = v & 0x7F;

		// Any bits left?
		v >>= 7;
		if (v == 0)
		{
			// We're done, bail after writing this byte.
			WriteByte(out);
			return;
		}

		// Flag the last bit to indicate more is coming.
		out |= 0x80;
		WriteByte(out);
	}
}

void WriteVarint(int v)
{
	// Zig-zag encoding for negative numbers.
	WriteUnVarint((v << 1) ^ (v >> 31));
}

//
// Read an unsigned varint off the wire.
//
// https://developers.google.com/protocol-buffers/docs/encoding#varints
//
unsigned int ReadUnVarint()
{
	unsigned char b;
	unsigned int out = 0;
	unsigned int offset = 0;

	for (;;)
	{
		// Read part of the variant.
		b = ReadByte();
		if (overflowed)
			return -1;

		// Shove the first seven bits into our output variable.
		out |= (unsigned int)(b & 0x7F) << offset;
		offset += 7;

		// Is the flag bit set?
		if (!(b & 0x80))
		{
			// Nope, we're done.
			return out;
		}

		if (offset >= 32)
		{
			// Our variant int is too big - overflow us.
			overflowed = true;
			return -1;
		}
	}
}

int ReadVarint()
{
	unsigned int uv = ReadUnVarint();
	if (overflowed)
		return -1;

	// Zig-zag encoding for negative numbers.
	return (uv >> 1) ^ (0U - (uv & 1));
}

// Testing app
int main()
{
	// loop goes here, if you want one.
	{
		int a = INT_MAX / 2;
		printf("checking %i\n", a);

		WriteVarint(a);
		int b = ReadVarint();
		if (a != b)
		{
			printf("Uh oh... %i != %i\n", a, b);
			return 1;
		}
		Reset();
	}

	printf("success\n");
	return 0;
}
