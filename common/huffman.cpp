// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006 by Denis Lukianov 
// Copyright (C) 2006-2013 by The Odamex Team.
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
//  Modified source from the Basic Compression Library 1.2.0
//  For the original, see http://bcl.sourceforge.net/
//
//-----------------------------------------------------------------------------


/*************************************************************************
* Name:        huffman.c
* Author:      Marcus Geelnard
* Description: Huffman coder/decoder implementation.
* Reentrant:   Yes
*
* This is a very straight forward implementation of a Huffman coder and
* decoder.
*
* Primary flaws with this primitive implementation are:
*  - Slow bit stream implementation
*  - Maximum tree depth of 32 (the coder aborts if any code exceeds a
*    size of 32 bits). If I'm not mistaking, this should not be possible
*    unless the input buffer is larger than 2^32 bytes, which is not
*    supported by the coder anyway (max 2^32-1 bytes can be specified with
*    an unsigned 32-bit integer).
*
* On the other hand, there are a few advantages of this implementation:
*  - The Huffman tree is stored in a very compact form, requiring only
*    10 bits per symbol (for 8 bit symbols), meaning a maximum of 320
*    bytes overhead.
*  - The code should be fairly easy to follow, if you are familiar with
*    how the Huffman compression algorithm works.
*
* Possible improvements (probably not worth it):
*  - Partition the input data stream into blocks, where each block has
*    its own Huffman tree. With variable block sizes, it should be
*    possible to find locally optimal Huffman trees, which in turn could
*    reduce the total size.
*  - Allow for a few different predefined Huffman trees, which could
*    reduce the size of a block even further.
*-------------------------------------------------------------------------
* Copyright (c) 2003-2006 Marcus Geelnard
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would
*    be appreciated but is not required.
*
* 2. Altered source versions must be plainly marked as such, and must not
*    be misrepresented as being the original software.
*
* 3. This notice may not be removed or altered from any source
*    distribution.
*
* Marcus Geelnard
* marcus.geelnard at home.se
*************************************************************************/

#include "version.h"
#include "huffman.h"

// For a memcpy
#include <cstring>

/*************************************************************************
*                           INTERNAL FUNCTIONS                           *
*************************************************************************/


/*************************************************************************
* _Huffman_InitBitstream() - Initialize a bitstream.
*************************************************************************/

void huffman::_Huffman_InitBitstream( huff_bitstream_t *stream, unsigned char *buf )
{
  stream->BytePtr  = buf;
  stream->BitPos   = 0;
}


/*************************************************************************
* _Huffman_ReadBit() - Read one bit from a bitstream.
*************************************************************************/

unsigned int huffman::_Huffman_ReadBit( huff_bitstream_t *stream )
{
  unsigned int  x, bit;
  unsigned char *buf;

  /* Get current stream state */
  buf = stream->BytePtr;
  bit = stream->BitPos;

  /* Extract bit */
  x = (*buf & (1<<(7-bit))) ? 1 : 0;
  bit = (bit+1) & 7;
  if( !bit )
  {
    ++ buf;
  }

  /* Store new stream state */
  stream->BitPos = bit;
  stream->BytePtr = buf;

  return x;
}


/*************************************************************************
* _Huffman_Read8Bits() - Read eight bits from a bitstream.
*************************************************************************/

unsigned int huffman::_Huffman_Read8Bits( huff_bitstream_t *stream )
{
  unsigned int  x, bit;
  unsigned char *buf;

  /* Get current stream state */
  buf = stream->BytePtr;
  bit = stream->BitPos;

  /* Extract byte */
  x = (*buf << bit) | (buf[1] >> (8-bit));
  ++ buf;

  /* Store new stream state */
  stream->BytePtr = buf;

  return x;
}


/*************************************************************************
* _Huffman_WriteBits() - Write bits to a bitstream.
*************************************************************************/

void huffman::_Huffman_WriteBits( huff_bitstream_t *stream, unsigned int x, unsigned int bits )
{
  unsigned int  bit, count;
  unsigned char *buf;
  unsigned int  mask;

  /* Get current stream state */
  buf = stream->BytePtr;
  bit = stream->BitPos;

  /* Append bits */
  mask = 1 << (bits-1);
  for( count = 0; count < bits; ++ count )
  {
    *buf = (*buf & (0xff^(1<<(7-bit)))) +
            ((x & mask ? 1 : 0) << (7-bit));
    x <<= 1;
    bit = (bit+1) & 7;
    if( !bit )
    {
      ++ buf;
    }
  }

  /* Store new stream state */
  stream->BytePtr = buf;
  stream->BitPos  = bit;
}


/*************************************************************************
* _Huffman_Hist() - Calculate (sorted) histogram for a block of data.
*************************************************************************/

void huffman::_Huffman_Hist( unsigned char *in, huff_sym_t *sym, unsigned int size )
{
  unsigned char *in_end = in + size;

  /* Build histogram */
  while( in != in_end )
  {
    sym[*in ++].Count ++;
  }

  total_count += size;

  // Tax all entries to prevent overflow
  while(total_count > 65000)
  {
	  for(int i = 0; i < 256; i++)
	  {
		  total_count -= sym[i].Count;
		  sym[i].Count /= 2;
		  total_count += sym[i].Count;
	  }
  }
}


/*************************************************************************
* _Huffman_StoreTree() - Store a Huffman tree in the output stream and
* in a look-up-table (a symbol array).
*************************************************************************/

void huffman::_Huffman_StoreTree( huff_encodenode_t *node, huff_sym_t *sym, unsigned int code, unsigned int bits )
{
  unsigned int sym_idx;

  /* Is this a leaf node? */
  if( node->Symbol >= 0 )
  {
    /* Find symbol index */
    for( sym_idx = 0; sym_idx < 256; ++ sym_idx )
    {
      if( sym[sym_idx].Symbol == node->Symbol ) break;
    }

    /* Store code info in symbol array */
    sym[sym_idx].Code = code;
    sym[sym_idx].Bits = bits;
    return;
  }

  /* Branch A */
  _Huffman_StoreTree( node->ChildA, sym, (code<<1)+0, bits+1 );

  /* Branch B */
  _Huffman_StoreTree( node->ChildB, sym, (code<<1)+1, bits+1 );
}


/*************************************************************************
* _Huffman_MakeTree() - Generate a Huffman tree.
*************************************************************************/

huffman::huff_encodenode_t *huffman::_Huffman_MakeTree( huff_sym_t *sym, huff_encodenode_t *nodes)
{
  huff_encodenode_t *node_1, *node_2, *new_root;
  unsigned int k, num_symbols, nodes_left, next_idx;

  /* Initialize all leaf nodes */
  num_symbols = 0;
  for( k = 0; k < 256; ++ k )
  {
    if( sym[k].Count > 0 )
    {
      nodes[num_symbols].Symbol = sym[k].Symbol;
      nodes[num_symbols].Count = sym[k].Count;
      nodes[num_symbols].ChildA = (huff_encodenode_t *) 0;
      nodes[num_symbols].ChildB = (huff_encodenode_t *) 0;
      ++ num_symbols;
    }
  }

  /* Build tree by joining the lightest nodes until there is only
     one node left (the root node). */
  new_root = (huff_encodenode_t *) 0;
  nodes_left = num_symbols;
  next_idx = num_symbols;
  while( nodes_left > 1 )
  {
    /* Find the two lightest nodes */
    node_1 = (huff_encodenode_t *) 0;
    node_2 = (huff_encodenode_t *) 0;
    for( k = 0; k < next_idx; ++ k )
    {
      if( nodes[k].Count > 0 )
      {
        if( !node_1 || (nodes[k].Count <= node_1->Count) )
        {
          node_2 = node_1;
          node_1 = &nodes[k];
        }
        else if( !node_2 || (nodes[k].Count <= node_2->Count) )
        {
          node_2 = &nodes[k];
        }
      }
    }
	
    /* Join the two nodes into a new parent node */
    new_root = &nodes[next_idx];
    new_root->ChildA = node_1;
    new_root->ChildB = node_2;
    new_root->Count = node_1->Count + node_2->Count;
    new_root->Symbol = -1;
    node_1->Count = 0;
    node_2->Count = 0;
    ++ next_idx;
    -- nodes_left;
  }

   /* Store the tree in the output stream, and in the sym[] array (the
      latter is used as a look-up-table for faster encoding) */
  if( new_root )
  {
    _Huffman_StoreTree( new_root, sym, 0, 0 );
  }
  else
  {
    /* Special case: only one symbol => no binary tree */
    root = &nodes[0];
    _Huffman_StoreTree( new_root, sym, 0, 1 );
  }

  return new_root;
}


/*************************************************************************
* Huffman_Compress() - Compress a block of data using a Huffman coder.
*  in     - Input (uncompressed) buffer.
*  out    - Output (compressed) buffer. This buffer must be 384 bytes
*           larger than the input buffer.
*  insize - Number of input bytes.
* The function returns the size of the compressed data.
*************************************************************************/

bool huffman::Huffman_Compress_Using_Histogram( unsigned char *in, size_t insize, unsigned char *out, size_t &outsize, huff_sym_t *sym )
{
  huff_sym_t       tmp;
  huff_bitstream_t stream;
  unsigned int     k, swaps, symbol;

  /* Do we have anything to compress? */
  if( insize < 1 )
  {
	  outsize = 0;
	  return true;
  }

  /* Initialize bitstream */
  _Huffman_InitBitstream( &stream, out );

  /* Sort histogram - first symbol first (bubble sort) */
  do
  {
    swaps = 0;
    for( k = 0; k < 255; ++ k )
    {
      if( sym[k].Symbol > sym[k+1].Symbol )
      {
        tmp      = sym[k];
        sym[k]   = sym[k+1];
        sym[k+1] = tmp;
        swaps    = 1;
      }
    }
  }
  while( swaps );

  // Init useful pointer for bounds check
  unsigned char *out_end = out + outsize;

  /* Encode input stream */
  for( k = 0; k < insize; ++ k )
  {
    if(stream.BytePtr + 17 >= out_end) // denis - todo - security, 17 masks potential terminator overflow, should really be a 1
	    return false;

	symbol = in[k];
    
    _Huffman_WriteBits( &stream, sym[symbol].Code, sym[symbol].Bits );
  }

  /* Calculate size of output data */
  outsize = (int)(stream.BytePtr - out);
  
  if( stream.BitPos > 0 )
  {
	  unsigned int left = 8 - stream.BitPos;
	  
	  // Write a terminator that will throw decompressor off the tree
	  for( k = 0; k < 256; ++ k )
	  {
		  if( sym[k].Bits > left )
		  {
			  _Huffman_WriteBits( &stream, sym[k].Code, sym[k].Bits );
			  break;
		  }
	  }
	  
	  outsize++;
  }

  return true;
}


/*************************************************************************
* Huffman_Uncompress() - Uncompress a block of data using a Huffman
* decoder.
*  in      - Input (compressed) buffer.
*  out     - Output (uncompressed) buffer. This buffer must be large
*            enough to hold the uncompressed data.
*  insize  - Number of input bytes.
*  outsize - Number of output bytes.
*************************************************************************/

bool huffman::Huffman_Uncompress_Using_Tree( unsigned char *in, size_t insize, unsigned char *out, size_t &outsize, huff_encodenode_t *root )
{
  huff_bitstream_t  stream;

  /* Do we have anything to decompress? */
  if( insize < 1 )
  {
	  outsize = 0;
	  return true;
  }

  // Prepare useful pointers
  unsigned char *in_end = in + insize;
  unsigned char *out_end = out + outsize;
  unsigned char *buf = out;

  /* Initialize bitstream */
  _Huffman_InitBitstream( &stream, in );

  /* Recover Huffman tree */
  huff_encodenode_t *node;

  /* Decode input stream */
  while(true)
  {
	  // End of output
	  if(buf >= out_end)
		  return false;
	  
	  node = root;
	  
	  /* Traverse tree until we find a matching leaf node */
	  while( node && node->Symbol < 0 && stream.BytePtr < in_end)
	  {
		  /* Get next node */
		  if( _Huffman_ReadBit( &stream ) )
			  node = node->ChildB;
		  else
			  node = node->ChildA;
	  }
	  
	  // End of input
	  if(stream.BytePtr >= in_end && node->Symbol < 0)
		break;

	  // Prematurely fell off the tree, nein! halt!
	  if(!node)
		  return false;
	  
	  /* We found the matching leaf node and have the symbol */
	  *buf ++ = (unsigned char) node->Symbol;
  }

  outsize = buf - out;

  return true;
}

/*************************************************************************
*                            PUBLIC FUNCTIONS                            *
*************************************************************************/

// Clear statistics
void huffman::reset()
{
	for( int k = 0; k < 256; ++ k )
	{
		sym[k].Symbol = k;
		sym[k].Count  = 1;
		sym[k].Code   = 0;
		sym[k].Bits   = 0;
	}

	total_count = 256;
	
	fresh_histogram = true;
}

// Analyse some raw data and add it to the compression statistics
void huffman::extend( unsigned char *data, size_t insize)
{
	_Huffman_Hist( data, sym, insize );
	fresh_histogram = true;
}

// Compress a chunk of data using only previously generated stats
bool huffman::compress( unsigned char *in_data, size_t in_len, unsigned char *out_data, size_t &out_len)
{
	if(fresh_histogram)
	{
		root = _Huffman_MakeTree( sym, (huff_encodenode_t *)&nodes );
		fresh_histogram = false;
	}
	
	return Huffman_Compress_Using_Histogram(in_data, in_len, out_data, out_len, sym);
}

// Decompress a chunk of data using only previously generated stats
bool huffman::decompress( unsigned char *in_data, size_t in_len, unsigned char *out_data, size_t &out_len)
{
	if(fresh_histogram)
	{
		root = _Huffman_MakeTree( sym, (huff_encodenode_t *)&nodes );
		fresh_histogram = false;
	}
	
	return Huffman_Uncompress_Using_Tree(in_data, in_len, out_data, out_len, root);
}

// Constructor
huffman::huffman()
{
	reset();
}

//
// Huffman Server
//

bool huffman_server::packet_sent(unsigned int id, unsigned char *in_data, size_t len)
{
	// already sent a packet, expecting one back
	// though if missed_packets is large, we should probably re-negotiate
	if(awaiting_ack && missed_acks < HUFFMAN_RENEGOTIATE_DELAY)
		return false;

	last_packet_id = id;
	
	// save current codec and extend it
	tmpcodec = active_codec ? alpha : beta;
	tmpcodec.extend(in_data, len);

	awaiting_ack = true;

	return true;
}

void huffman_server::packet_acked(unsigned int id)
{
	// looking for a reply?
	if(!awaiting_ack)
		return;
	
	// check if the correct packet was ack'ed
	if(last_packet_id != id)
	{
		if(missed_acks < HUFFMAN_RENEGOTIATE_DELAY)
			missed_acks++;
		
		return;
	}

	// codec change
	active_codec = !active_codec;
	huffman &update = active_codec ? alpha : beta;
	update = tmpcodec;
	last_ack_id = id;
	awaiting_ack = false;
	missed_acks = 0;
}

//
// Huffman Client
//

void huffman_client::ack_sent(unsigned char *in_data, size_t len)
{
	// update the codec only one ack at a time
	if(awaiting_ackack)
		return;
	
	tmpcodec = active_codec ? alpha : beta;
	tmpcodec.extend(in_data, len);

	awaiting_ackack = true;
}

huffman &huffman_client::codec_for_received(unsigned char id)
{
	// if got a shiny new packet with a different codec
	// and was expecting there to be one
	if(awaiting_ackack && (id ? 1 : 0) != active_codec)
	{
		// swap the codecs
		active_codec = !active_codec;
		huffman &update = active_codec ? alpha : beta;
		update = tmpcodec;
		awaiting_ackack = false;
	}
	
	return id ? alpha : beta;
}

void huffman_client::reset()
{
	active_codec = 0;
	awaiting_ackack = false;
	alpha.reset();
	beta.reset();
}


VERSION_CONTROL (huffman_cpp, "$Id$")

