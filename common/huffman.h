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
// DESCRIPTION:
//  The idea here is to use huffman coding without having to send
//  the huffman tree explicitly across the network.
//
//		* For each packet, the client sends an ACK
//		* For each ACK the server gets back, both client and server have a 
//			copy of the packet
//		* Statistically over time, packets contain similar data 
//
//  Therefore:
//  -> Client and server can build huffman trees using past packet data
//  -> Client and server can use this tree to compress new data
//
//-----------------------------------------------------------------------------


#ifndef __C_HUFFMAN_H__
#define __C_HUFFMAN_H__

#include <iostream>
#include <cstring>

class huffman
{
	// Structures
	struct huff_bitstream_t
	{
		unsigned char *BytePtr;
		unsigned int  BitPos;
	};

	struct huff_sym_t
	{
		int Symbol;
		unsigned int Count;
		unsigned int Code;
		unsigned int Bits;
	};

	struct huff_encodenode_t
	{
		huff_encodenode_t *ChildA, *ChildB;
		int Count;
		int Symbol;
	};

	// Histogram of character frequency
	huff_sym_t		sym[256];
	unsigned int	total_count;

	// Flag to indicate that tree needs rebuilding
	bool			fresh_histogram;

	// Tree generated from the histogram
	/* The maximum number of nodes in the Huffman tree is 2^(8+1)-1 = 511 */
	huff_encodenode_t nodes[511];
	huff_encodenode_t *root;

	void _Huffman_InitBitstream( huff_bitstream_t *stream,  unsigned char *buf );
	unsigned int _Huffman_ReadBit( huff_bitstream_t *stream );
	unsigned int _Huffman_Read8Bits( huff_bitstream_t *stream );
	void _Huffman_WriteBits( huff_bitstream_t *stream, unsigned int x, unsigned int bits );
	void _Huffman_Hist( unsigned char *in, huff_sym_t *sym, unsigned int size );
	huff_encodenode_t *_Huffman_MakeTree( huff_sym_t *sym, huff_encodenode_t *nodes);
	void _Huffman_StoreTree( huff_encodenode_t *node, huff_sym_t *sym, unsigned int code, unsigned int bits );

	bool Huffman_Compress_Using_Histogram( unsigned char *in, size_t insize, unsigned char *out, size_t &outsize, huff_sym_t *sym );
	bool Huffman_Uncompress_Using_Tree( unsigned char *in, size_t insize, unsigned char *out, size_t &outsize, huff_encodenode_t *tree_root );

public:

	// Clear statistics
	void reset();

	// Analyse some raw data and add it to the compression statistics
	void extend( unsigned char *data, size_t len);

	// Compress a chunk of data using only previously generated stats
	bool compress( unsigned char *in_data, size_t in_len, unsigned char *out_data, size_t &out_len);

	// Decompress a chunk of data using only previously generated stats
	bool decompress( unsigned char *in_data, size_t in_len, unsigned char *out_data, size_t &out_len);

	// For debugging, this count can be used to see if two codecs have had the same length input
	int get_count() { return total_count; }
	
	// Constructor
	huffman();
	
	// Copying invalidates all tree pointers
	huffman(const huffman &other) : total_count(other.total_count), fresh_histogram(true)
	{
		memcpy(sym, other.sym, sizeof(sym));
	} 
};

#define HUFFMAN_RENEGOTIATE_DELAY	256

class huffman_server
{
	huffman alpha, beta, tmpcodec;
	bool active_codec;
	
	unsigned int last_packet_id, last_ack_id;

	unsigned int missed_acks;

	bool awaiting_ack;

public:

	huffman &get_codec() { return active_codec ? alpha : beta; }
	unsigned char get_codec_id() { return active_codec ? 1 : 0; }

	bool packet_sent(unsigned int id, unsigned char *in_data, size_t len);
	void packet_acked(unsigned int id);
	
	huffman_server() : active_codec(0), last_packet_id(0), last_ack_id(0),
        missed_acks(0), awaiting_ack(false)
	{}
	huffman_server(const huffman_server &other) :
		alpha(other.alpha),
		beta(other.beta),
		tmpcodec(other.tmpcodec),
		active_codec(other.active_codec),
		last_packet_id(other.last_packet_id),
		last_ack_id(other.last_ack_id),
		missed_acks(other.missed_acks),
		awaiting_ack(other.awaiting_ack)
	{}
};

class huffman_client
{
	huffman alpha, beta, tmpcodec;
	bool active_codec;

	bool awaiting_ackack;

public:

	void reset();

	void ack_sent(unsigned char *in_data, size_t len);
	huffman &codec_for_received(unsigned char id);

	huffman_client() { reset(); }
	huffman_client(const huffman_client &other) :
		alpha(other.alpha),
		beta(other.beta),
		tmpcodec(other.tmpcodec),
		active_codec(other.active_codec),
		awaiting_ackack(other.awaiting_ackack)
	{}
};

#endif

