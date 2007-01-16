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
//	This class is a packet itself.  It will return useful things
//	The refresh is almost identical to ServerPacket.  However, details
//	in the other methods, in my opinion, warrant a seporate class
//	instead of trying to mishmesh two packet types into one
//
//-----------------------------------------------------------------------------


import java.io.*;
import java.net.*;

public class ServerPacket
{

	// Challenge and expected response
	
	final	static long CHALLENGE = 777123;
	final static long SUCCESS = 5560020;

	// Global variables
	
	static String strServerName;	// Server.  This is an IP address
	static int intServerPort;			// Server port
	static byte bytResponse[];		// Raw byte array of Server response

	String strServerTitle;				// The title of the server.
	short shoPlayers;							// Total number of players
	short shoMaxPlayers;					// Maximum number of players
	String strMapName;						// Map name
	String strIWADName;						// Map name
	String strPWADName = "";						// Map name
	long lonPing;									// Ping to server
	
	// Creating the packet without anything.
	
	public ServerPacket()
	{
		strServerName = "";
		intServerPort = 0;
	}
	
	// Stick the appropriate information into the ServerPacket
	
	public ServerPacket(String strInputServer, int intInputPort)
	{
		strServerName = strInputServer;
		intServerPort = intInputPort;
	}

	// This method prepares the appropriate information, so the class
	// can actually return some useful data.  Returns false if something
	// goes wrong
	
	public boolean refresh()
	{
		boolean bolReturn = false;
		try
		{
			
			// Creates the Challenge
			
			byte[] bytChallenge = Math.toDWord(CHALLENGE);

			// Server Server

			InetAddress inaServerName = InetAddress.getByName(strServerName);

			// Creates the DatagramSocket
			
			DatagramSocket dgsSocket = new DatagramSocket();
			dgsSocket.setSoTimeout(10000);

			// Creates the DatagramPacket challenge to be sent to the
			// server
			
			DatagramPacket dgpSendPacket = new DatagramPacket(bytChallenge, bytChallenge.length, inaServerName, intServerPort);

			byte[] bytGetBuffer = new byte[65535];
			DatagramPacket dgpGetPacket = new DatagramPacket(bytGetBuffer, bytGetBuffer.length);

			// Sends the challenge

			long lonStart = System.currentTimeMillis();

			dgsSocket.send(dgpSendPacket);

			// Waits for a response
			
			dgsSocket.receive(dgpGetPacket);

			long lonEnd = System.currentTimeMillis();
			lonPing = lonEnd - lonStart;
			
			// Sticks the response in the global response variable
			
			bytResponse = dgpGetPacket.getData();

			// Now that we have the response, now we gotta figure out
			// what to do with it...

			byte[] bytSuccess = {bytResponse[0], bytResponse[1], bytResponse[2], bytResponse[3]};
			
			// Checks the magic number, if it's wrong, bail out now
			
			if(Math.toLong(bytSuccess) != SUCCESS)
			{
				bolReturn = false;
			}
			else
			{
				// We're not done yet.  In order to increase the ease with
				// which other methods can find the information it is looking
				// for, we will now parse the response and find the index's
				// that are relivent to certain things that we are interested
				// in.
				
				// Server TItle
				
				int intIndex = 4;
				int intLength;

				intLength = ServerPacket.asciizLength(intIndex);
				strServerTitle = new String(bytResponse, intIndex, intLength);
				
				// Players
				
				intIndex += 1;
				intIndex += intLength;
				shoPlayers = Math.toShort(bytResponse[intIndex]);

				// Max Players
				
				intIndex += 1;
				shoMaxPlayers = Math.toShort(bytResponse[intIndex]);
				
				// Map Name

				intIndex += 1;
				intLength = ServerPacket.asciizLength(intIndex);
				strMapName = new String(bytResponse, intIndex, intLength);
				
				// IWAD Name

				intIndex += (intLength + 1);
				intLength = ServerPacket.asciizLength(intIndex);
				strIWADName = new String(bytResponse, intIndex, intLength);

				// PWAD Names
				
				intIndex += (intLength + 1);
				short shoPWAD = Math.toShort(bytResponse[intIndex]);
				intIndex += 1;

				for(int i = 1; i <= shoPWAD; i++)
				{
					intLength = ServerPacket.asciizLength(intIndex);
					strPWADName = strPWADName + new String(bytResponse, intIndex, intLength);
					intIndex += (intLength + 1);
				}
				
				bolReturn = true;
			}
		}
		catch(IOException e)
		{
			bolReturn = false;
		}
		return bolReturn;
	}

	// Retruns various things
	
	public String returnName()
	{
		return strServerTitle;
	}

	public String returnPlayers()
	{
		return shoPlayers + "/" + shoMaxPlayers;
	}

	public String returnMapName()
	{
		return strMapName;
	}

	public String returnIWADName()
	{
		return strIWADName;
	}

	public String returnPWADName()
	{
		return strPWADName;
	}
	
	public String returnPing()
	{
		return Long.toString(lonPing);
	}
	
	// Returns the length of the ASCIIZ string
	// If it runs off the end of the array, it returns -1
	// The length returned does NOT include the null-termination, so
	// if you're figuring this into your calculations of the packet,
	// add one to the returned number to get the location of the next
	// item in the byte array
	
	// THIS METHOD DOES NOT WORK UNLESS IT HAS bytResponse TO WORK WITH
	// SO PLEASE DO NOT COPY AND PASTE THIS INTO YOUR OWN PROGRAM WITHOUT
	// AT LEAST SOME MODIFICATION
	
	private static int asciizLength(int intIndex)
	{

		int intReturn = 0;
		
		while (bytResponse[intIndex + intReturn] != Byte.parseByte("00"))
		{
			// So long as we don't get to the end of the ASCIIZ, we're
			// fine.
					
			intReturn++;

			// We gotta make sure that we don't run off the end of the
			// end of the array, though
					
			if(intIndex > bytResponse.length)
			{
				intReturn = -1;
				break;
			} 
		}
	return intReturn;	
	}
}
