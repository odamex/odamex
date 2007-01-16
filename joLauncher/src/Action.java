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
//	Full of Action.  Also, boring computational stuff that I hope I'll forget
//	about in six months
//
//-----------------------------------------------------------------------------


import java.util.StringTokenizer;
import java.util.Timer;
import java.util.TimerTask;
import java.io.IOException;

public class Action
{
	// Multithreading
	
	Timer timer;
	
	// Creates the MasterPacket ahead of time
	
	static MasterPacket mpaMaster; 
	
	// About window
	
	public static void about()
	{
		Messenger.MESSAGE_ABOUT();
	}

	public static void exit()
	{
		System.exit(0);			
	}

	// The almighty console parser
	
	public void parse(String strInput)
	{
		// Console
		// TODO: Make an option here to make the prompt silent

		Messenger.MESSAGE_PROMPT(strInput);
		strInput.toLowerCase();
		
		if(strInput.equals("about"))
		{
			Action.about();	
		}
		if(strInput.startsWith("connect "))
		{
			StringTokenizer stkInput = new StringTokenizer(strInput.substring(8), ":");
			if(stkInput.countTokens() != 2)
			{
				Messenger.ERROR_BAD_SYNTAX();
			}
			else
			{
				try
				{
					timer = new Timer();
					timer.schedule(new Connect(stkInput.nextToken(), Integer.parseInt(stkInput.nextToken())), 0);
				}
				// Just in case some smartass wants to put a non-integer for
				// the port
				catch(NumberFormatException e)
				{					
					Messenger.ERROR_BAD_SYNTAX();
					timer.cancel();
				}
			}
		}
		else if(strInput.equals("quit") || strInput.equals("exit"))
		{
			Action.exit();
		}
		else if(strInput.equals("refresh"))
		{
			// Checks to see if the master server has something to give
			// this method, if not, tell the user

			if(mpaMaster.hasRun() == false)
			{
				Messenger.ERROR_NO_MASTER_LIST();
			}		
			else
			{
				timer = new Timer();
				timer.schedule(new RefreshThread(), 0);
			}
		}
		else if(strInput.startsWith("refresh "))
		{
			// You don't need a master server to refresh an individual
			// server, so we don't do the master server check in this
			// method.  However, we do have to check to make sure the
			// user didn't fuck up his statements.  This criteria, however
			// is very relaxed, as we are looking for is a colon in the
			// address given after the space
			
			StringTokenizer stkInput = new StringTokenizer(strInput.substring(8), ":");
			if(stkInput.countTokens() != 2)
			{
				Messenger.ERROR_BAD_SYNTAX();
			}
			else
			{
				try
				{
				timer = new Timer();
				timer.schedule(new RefreshOneThread(stkInput.nextToken(), Integer.parseInt(stkInput.nextToken())), 0);
				}
				// Just in case some smartass wants to put a non-integer for
				// the port
				catch(NumberFormatException e)
				{					
					Messenger.ERROR_BAD_SYNTAX();
					timer.cancel();
				}
			}
		}
		else if(strInput.equals("unidoom"))
		{
			Messenger.MESSAGE_UD();
		}
		else if(strInput.equals("update"))
		{
			timer = new Timer();
			timer.schedule(new UpdateThread(), 0);
		}
		else
		{
			Messenger.ERROR_BAD_COMMAND();
		}
	}

	// Connect to a specified server
	
	public static void connect(String strAddress, int intPort)
	{
		Messenger.MESSAGE_CONNECT_SERVER(strAddress,intPort);

		// Refresh the server directly.

		// TODO: I think this should refresh the server on the server
		//       list as well.  This will come once I figure out how
		//       to get a return from a thread
		
		ServerPacket spaServer = new ServerPacket(strAddress, intPort);
		String strAddressPort = strAddress + ":" + intPort;			
		
		if(spaServer.refresh() == true)
		{
			String strCommandLine[];
		
			// If there aren't any pwads, then don't bother with them
		
			if(spaServer.returnPWADName().equals("") == true)
			{
				strCommandLine = new String[]{ConfigFile.returnPath(), "-connect", strAddressPort, "-iwad", spaServer.returnIWADName()};
			}
			else
			{
				strCommandLine = new String[]{ConfigFile.returnPath(), "-connect", strAddressPort, "-iwad", spaServer.returnIWADName(), "-file", spaServer.returnPWADName()};
			}
			
			// The time has come....to START THE PROGRAM
			
			try
			{
				Process child = Runtime.getRuntime().exec(strCommandLine);
			}
			catch(Throwable t)
			{
				Messenger.ERROR_CANT_START_ODAMEX();
			}
		}
		else
		{
			Messenger.ERROR_CANT_FIND_SERVER(strAddress + intPort);
		}
	}
	
	// Update the master list
	
	public static void update()
	{
		Messenger.PREFIX_MASTER();

		// Start up the MasterPacket and refresh it

		mpaMaster = new MasterPacket("mancubus.net",15000);
		
		if(mpaMaster.update() == true)
		{
			Messenger.SUFFIX_DONE();
			
			// Creates a new Action instance and parses a refresh
			
			Action actAction = new Action();
			actAction.parse("refresh");
		}
		else
		{
			Messenger.SUFFIX_ERROR();
		}
	}

	// Refresh the servers from the master list
	
	public static void refresh()
	{	
		// Clear the server list before starting
		
		GUI.clearServers();
		
		// Parse the individual servers

		if(mpaMaster.countServers() == 0)
		{
			Messenger.ERROR_NO_SERVERS();
		}
		else
		{
			// Loop through the servers.
			
			for(int i = 1; i <= mpaMaster.countServers();i ++)
			{
				String strAddress = mpaMaster.returnAddress(i);
				int intPort = mpaMaster.returnPort(i);

				// Create a new instance of Action and use it to run the 
				// command parser
				
				Action actAction = new Action();
				actAction.parse("refresh " + strAddress + ":" + intPort);
			}
		}
	}

	public static void refresh(String strAddress, int intPort)
	{
		Messenger.MESSAGE_PARSE_SERVER(strAddress,intPort);
			
		// Create a new instance of ServerPacket and ring the server
		// in question

		ServerPacket spaServer = new ServerPacket(strAddress, intPort);

		// Check to make sure that the entry doesn't already exist

		String strAddressPort = strAddress + ":" + intPort;			
		int intIndex = GUI.findServer(strAddressPort);
		
		if(spaServer.refresh() == true)
		{
			Messenger.MESSAGE_FOUND_SERVER(spaServer.returnName());
			
			// If the entry was found and already exists, replace it
			// otherwise, add it

			if(intIndex != -1)
			{
				GUI.replaceServer(spaServer.returnName(), intIndex, spaServer.returnPlayers(), spaServer.returnMapName(), spaServer.returnIWADName(), spaServer.returnPWADName(), spaServer.returnPing());
			}
			else
			{
				GUI.addServer(spaServer.returnName(), strAddressPort, spaServer.returnPlayers(), spaServer.returnMapName(), spaServer.returnIWADName(), spaServer.returnPWADName(), spaServer.returnPing());
			}
			
		}
		else
		{
			Messenger.ERROR_CANT_FIND_SERVER(strAddressPort);
		
			// If the entry was not found and already exists, remove it
			// otherwise do nothing
			
			if(intIndex != -1)
			{
				GUI.removeServer(intIndex);
			}
		}
	}
	// These classes simply redirect to their appropriate normal
	// threads.  Only difference is that this extra step multi-threads
	// the process and allows UI to respond
	
	class UpdateThread extends TimerTask
	{
		public void run()
		{
			Action.update();
			timer.cancel();	
		}
	}
	class RefreshThread extends TimerTask
	{
		public void run()
		{
			Action.refresh();
			timer.cancel();	
		}
	}
	class Connect extends TimerTask
	{
		String strAddress;
		int intPort;
		
		public Connect(String strInput, int intInput)
		{
			strAddress = strInput;
			intPort = intInput;
		}
		
		public void run()
		{
			Action.connect(strAddress, intPort);
			timer.cancel();
		}
	}
	class RefreshOneThread extends TimerTask
	{
		String strAddress;
		int intPort;
		
		public RefreshOneThread(String strInput, int intInput)
		{
			strAddress = strInput;
			intPort = intInput;
		}
		
		public void run()
		{
			Action.refresh(strAddress, intPort);
			timer.cancel();	
		}
	}
}

