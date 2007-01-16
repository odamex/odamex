// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 2006-2007 by The Odamex Team.
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
//	OK, I lied, THIS is where the boring computational stuff is.  Stupid Java
//	and its lack of unsigned ints...
//
//-----------------------------------------------------------------------------


public class Math
{
	// Converts an array of four bytes to an unsigned long

	public static long toLong(byte bytInput[]) 
	{
		long lonResult = 0;

		lonResult += ((bytInput[3] & 0xFF) << 24);
		lonResult += ((bytInput[2] & 0xFF) << 16);
		lonResult += ((bytInput[1] & 0xFF) << 8);
		lonResult += (bytInput[0] & 0xFF);

		return lonResult;
	}

	// Converts an array of two bytes to an unsigned int

	public static int toInt(byte bytInput[]) 
	{
		int intResult = 0;

		intResult += ((bytInput[1] & 0xFF) << 8);
		intResult += (bytInput[0] & 0xFF);

		return intResult;
	}

	// Converts a single byte to an unsigned short

	public static short toShort(byte bytInput) 
	{
		short shoResult = 0;

		shoResult += (bytInput & 0xFF);

		return shoResult;
	}

	// Converts an unsigned long to an array of four bytes

	public static byte[] toDWord(long lonInput)
	{
		byte[] bytResult = new byte[4];

		bytResult[0] = (byte) (lonInput & 0x00FF);
		bytResult[1] = (byte) ((lonInput >> 8) & 0x000000FF);
		bytResult[2] = (byte) ((lonInput >> 16) & 0x000000FF);
		bytResult[3] = (byte) ((lonInput >> 24) & 0x000000FF);

		return bytResult;
	}
}
