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
// This class is a packet itself.  It will return useful things
//
//-----------------------------------------------------------------------------


import java.io.*;
import java.net.*;

public class MasterPacket
{

	// Challenge and expected response
	
	final	static long CHALLENGE = 777123;
	final static long SUCCESS = 777123;

	// Global variables

	static boolean bolRunOnce = false;// True if the packet has gotten
																		// an update from the master at
																		// least once
	static String strMasterName;	// Master Server
	static int intMasterPort;			// Master Server port
	static byte bytResponse[];		// Raw byte array of Master response
	static Integer intServerNum;	// Number of servers returned by Master
	
	// Creating the packet without anything.
	
	public MasterPacket()
	{
		strMasterName = "";
		intMasterPort = 0;
	}
	
	// Stick the appropriate information into the MasterPacket
	
	public MasterPacket(String strInputServer, int intInputPort)
	{
		strMasterName = strInputServer;
		intMasterPort = intInputPort;
	}

	// This method prepares the appropriate information, so the class
	// can actually return some useful data.  Returns false if something
	// goes wrong
	
	public boolean update()
	{
		boolean bolReturn = false;
		try
		{
			
			// Creates the Challenge
			
			byte[] bytChallenge = Math.toDWord(CHALLENGE);

			// Master Server

			InetAddress inaMasterName = InetAddress.getByName(strMasterName);

			// Creates the DatagramSocket
			
			DatagramSocket dgsSocket = new DatagramSocket();
			dgsSocket.setSoTimeout(10000);

			// Creates the DatagramPacket challenge to be sent to the
			// server
			
			DatagramPacket dgpSendPacket = new DatagramPacket(bytChallenge, bytChallenge.length, inaMasterName, intMasterPort);

			// Sends the challenge

			dgsSocket.send(dgpSendPacket);

			// Waits for a response
			
			byte[] bytGetBuffer = new byte[65535];
			DatagramPacket dgpGetPacket = new DatagramPacket(bytGetBuffer, bytGetBuffer.length);
			dgsSocket.receive(dgpGetPacket);
			
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
				bolRunOnce = true;
				bolReturn = true;
			}
		}
		catch(IOException e)
		{
			bolReturn = false;
		}
	
	return bolReturn;	
	}

	public static boolean hasRun()
	{
		return bolRunOnce;
	}
	
	// countServers: Returns the number of servers
	
	public int countServers()
	{
		byte[] bytServers = {bytResponse[4], bytResponse[5]};
		return Math.toInt(bytServers);
	}
	
	// return: Return the address or port based on a certain server count
	//         These are based on what order they were obtained in, NOT
	//         WHAT INDEX THEY MIGHT BE IN THE FINAL LIST OF SERVERS!!!
	
	public String returnAddress(int intIndex)
	{
		int intServers[] = new int [4];

		// Each IP block is 4 bytes long, with two extra for a port.
		// Because the handshake and number of servers is identical to
		// each of these blocks in size, it's trivial to find the byte
		// for a certain server simply by multiplying by six.
		//
		// EX: 6 * 1 = 6, which just so happens to be the index of the byte
		//     array where the first number in the IP is stored.
		//

		intServers[0] = Math.toShort(bytResponse[intIndex * 6]);
		intServers[1] = Math.toShort(bytResponse[(intIndex * 6) + 1]);
		intServers[2] = Math.toShort(bytResponse[(intIndex * 6) + 2]);
		intServers[3] = Math.toShort(bytResponse[(intIndex * 6) + 3]);

		return intServers[0] + "." + intServers[1] + "." + intServers[2] + "." + intServers[3];
	}

	public int returnPort(int intIndex)
	{
		// See returnAddress for an explination of my math
		// This one is similar, except I have to put the two numbers into
		// a byte array and figure out the result seporately

		byte bytPort[] = new byte[2];
		bytPort[0] = bytResponse[(intIndex * 6) + 4];
		bytPort[1] = bytResponse[(intIndex * 6) + 5];

		return Math.toInt(bytPort);
	}
}
