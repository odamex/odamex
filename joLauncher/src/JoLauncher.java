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
//	This file should only have very high level preperation in it.  Short, sweet
//	and obvious
//
//-----------------------------------------------------------------------------


import javax.swing.*;

public class JoLauncher
{
	
	// Setting up the GUI

	final static GUI guiMainGUI = new GUI();

	public static void main(String args[])
	{

		// Parse the config file

		if(ConfigFile.refresh() != true)
		{
			// Whoops, we didn't get everything we were expecting to
			// get.  Exit the program.
			
			System.exit(1);
		}
		
		// Initialize the GUI

		javax.swing.SwingUtilities.invokeLater(new Runnable()
		{
			public void run()
			{

				guiMainGUI.initializeGUI();
			}
		});
	}
}


