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
//	This class is concerned with parsing the configuration file 
//	for JoLauncher and returning a certian value when requested
//
//-----------------------------------------------------------------------------


import javax.swing.*;
import java.io.*;

public class ConfigFile
{

	// The variables we need to fill
	
	static String strPath = "";
	
	// Loads the config file.  Returns true if sucessful.

	public static boolean refresh()
	{
		boolean bolReturn = false;
		
		try
		{
			// Open the file
			
			BufferedReader brIn = new BufferedReader(new FileReader("JoLauncher.cfg"));

			// Parse the config file
			
			String strLine;
		
			while ((strLine = brIn.readLine()) != null)
			{

				// This is temporary.  I intend to include a full .cfg file
				// implimentation here soon
				
				strPath = strLine;
				
			}

			// Check to make sure we have everything
			
			if(strPath.equals("") == true)
			{
				error();
			}
			else
			{
				bolReturn = true;
			}

			// Close file
			
			brIn.close();
			
		}
		catch(IOException e)
		{
			error();
		}

		return bolReturn;
	
	}

	// Retun various things
	
	public static String returnPath()
	{
		return strPath;
	}
	
	// Error message
	
	private static void error()
	{
		JOptionPane.showMessageDialog(null ,"Your JoLauncher.cfg seems to be non-existant, corrupt,\n or missing vital information.  JoLauncher will now close.", "JoLauncher", JOptionPane.WARNING_MESSAGE);
	}
}
