// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2020 by The Odamex Team.
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
//	Memory pool allocation
//	Allocates a large pool of memory and allocates blocks of it when asked.
//	Memory can only be freed by the clear() function for simplicity. If
//	the intial memory pool is exhausted, additional pools are allocated. These
//	are consolodated into one large pool the next time clear() is called.
//
//    
//-----------------------------------------------------------------------------


#pragma once


template <typename T>
class Pool
{
public:
	Pool(size_t initial_max_count) :
		num_blocks(0), block_size(NULL), data_block(NULL), free_block(NULL)
	{
		resize(initial_max_count);
	}

	~Pool()
	{
		free_data();
	}

	void clear()
	{
		if (num_blocks <= 1)
		{
			free_block = data_block[0];
			return;
		}

		size_t new_size = 0;
		if (block_size != NULL)
			new_size = block_size[num_blocks - 1];
		free_data();
		resize(new_size);
	}

	T* alloc(size_t count = 1)
	{
		while (free_block + count * sizeof(T) > data_block[num_blocks - 1] + block_size[num_blocks - 1])
			resize(2 * block_size[num_blocks - 1]);

		T* ptr = reinterpret_cast<T*>(free_block);
		free_block += count * sizeof(T);
		return ptr;
	}

private:
	void resize(size_t new_max_count)
	{
		uint32_t new_size = new_max_count * sizeof(T);

		if (new_size == 0)
			return;

		num_blocks++;

		size_t* new_block_size = new size_t[num_blocks];
		if (block_size != NULL)
		{
			memcpy(new_block_size, block_size, (num_blocks - 1) * sizeof(size_t));
			delete [] block_size;
		}
		new_block_size[num_blocks - 1] = new_size;
		block_size = new_block_size;

		byte** new_data_block = new byte*[num_blocks];
		if (data_block != NULL)
		{
			memcpy(new_data_block, data_block, (num_blocks - 1) * sizeof(byte*));
			delete [] data_block;
		}
		new_data_block[num_blocks - 1] = new byte[new_size];
		data_block = new_data_block;

		free_block = data_block[num_blocks - 1];
	}

	void free_data()
	{
		for (size_t i = 0; i < num_blocks; i++)
			delete [] data_block[i];	

		delete [] block_size;
		delete [] data_block;
		num_blocks = 0;
		block_size = NULL;
		data_block = NULL;
		free_block = NULL;
	}

	size_t		num_blocks;
	size_t*		block_size;
	byte**		data_block;
	byte*		free_block;
};
