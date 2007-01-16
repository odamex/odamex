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
//	The Messenger service.  This handles all output to the console, and, under
//	certain circumstances, and depending on user settings, display a box
//	There are three types of messages.  Prefixes go before a computation
//	Suffixes go after.  Messages are constant and appear by themselves.
//	Errors are like messages, except they tell you when something bad
//	happens
//
//-----------------------------------------------------------------------------


import javax.swing.*;

public class Messenger
{

	public static void ERROR_BAD_COMMAND()
	{
		GUI.printlnConsole("Bad Command");
	}
	public static void ERROR_BAD_SYNTAX()
	{
		GUI.printlnConsole("Invalid Syntax");
	}
	public static void ERROR_CANT_FIND_SERVER(String strInput)
	{
		GUI.printlnConsole(strInput + " not found!");
	}
	public static void ERROR_CANT_START_ODAMEX()
	{
		GUI.printlnConsole("Can't start Odamex!");
	}
	public static void ERROR_NO_MASTER_LIST()
	{
		GUI.printlnConsole("No master list to parse!");
	}
	public static void ERROR_NO_SERVER_SELECTED()
	{
		GUI.printlnConsole("No server selected!");
	}
	public static void ERROR_NO_SERVERS()
	{
		GUI.printlnConsole("No servers found!");
	}

	public static void MESSAGE_ABOUT()
	{
		JOptionPane.showMessageDialog(new JFrame(), "Odamex ALPHA\nby AlexMax\n\nThanks to:\nThe Odamex Team", "About...", JOptionPane.INFORMATION_MESSAGE);
	}
	public static void MESSAGE_CONNECT_SERVER(String strAddress, int intPort)
	{
		GUI.printlnConsole("Connecting to " + strAddress + ":" + intPort + " ...");
	}
	public static void MESSAGE_FOUND_SERVER(String strInput)
	{
		GUI.printlnConsole("Found " + strInput);
	}
	public static void MESSAGE_PARSE_SERVER(String strAddress, int intPort)
	{
		GUI.printlnConsole("Parsing " + strAddress + ":" + intPort + " ...");
	}
	public static void MESSAGE_PROMPT(String strInput)
	{
		GUI.printlnConsole("> " + strInput);
	}
	public static void MESSAGE_UD()
	{
		GUI.printlnConsole("UD ON TOP!");
	}
	
	public static void PREFIX_MASTER()
	{
		GUI.printConsole("Connecting to Master Server...");
	}

	public static void SUFFIX_DONE()
	{
		GUI.printlnConsole("...done!");
	}
	public static void SUFFIX_ERROR()
	{
		GUI.printlnConsole("...failed!");
	}
}
