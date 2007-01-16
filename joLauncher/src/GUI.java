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
//	The mishmesh of code known as the GUI.  I tried to make it as nice looking
//	and easy to follow as possible.  Honest.
//
//-----------------------------------------------------------------------------


import javax.swing.*;        
import javax.swing.table.*;
import javax.swing.event.*;
import java.awt.*;
import java.awt.event.*;

public class GUI extends JPanel implements ActionListener 
{

static JTable	jtbServers;
static DefaultTableModel dtmServersData;
static JScrollPane jspServers;
static JTextArea jtaConsole;
static JScrollPane jspConsole;
static JTextField jtfCommandLine;

// Creates an instance of the Action class
 
Action actAction = new Action();

	// Setting up the JPanel

	public void panelPopulate(Container conMain)
	{
		// Sets up a grid layout
		
		conMain.setLayout(new BorderLayout());
		
		// Defining the different parts of the mainframe

		JToolBar jtbToolBar	= new JToolBar();
		JScrollPane jspServers = this.guiServers();
		JScrollPane jspConsole = this.guiConsole();
		JTextField jtfCommandLine = this.guiCommandLine();
		JSplitPane jlpServersConsole = new JSplitPane(JSplitPane.VERTICAL_SPLIT, jspServers, jspConsole); 
		
		guiAddButtons(jtbToolBar);
		
		// Keep the toolbar stationary

		jtbToolBar.setFloatable(false);

		// Force a vertical scrollbar and remove horizonal scrollbar on
		// the server list
		
		jspServers.setHorizontalScrollBarPolicy(ScrollPaneConstants.HORIZONTAL_SCROLLBAR_NEVER);
		jspServers.setVerticalScrollBarPolicy(ScrollPaneConstants.VERTICAL_SCROLLBAR_ALWAYS);

		// Remove Tooltips from the table
		
		ToolTipManager.sharedInstance().unregisterComponent(jtbServers);
		ToolTipManager.sharedInstance().unregisterComponent(jtbServers.getTableHeader());

		// Impliment Single Selection on the Table

		jtbServers.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
		
		// Force a vertical scrollbar and remove horizonal scrollbar on
		// the console

		jspConsole.setHorizontalScrollBarPolicy(ScrollPaneConstants.HORIZONTAL_SCROLLBAR_NEVER);
		jspConsole.setVerticalScrollBarPolicy(ScrollPaneConstants.VERTICAL_SCROLLBAR_ALWAYS);
		
		// Adding some ActionListeners
		
		jtfCommandLine.addActionListener(this);
		jtfCommandLine.setActionCommand("CONSOLE");

		// Adding them to the panel

		conMain.add(jtbToolBar, BorderLayout.PAGE_START);
		conMain.add(jlpServersConsole, BorderLayout.CENTER);
		conMain.add(jtfCommandLine, BorderLayout.PAGE_END);
	}

	public void initializeGUI()
	{
		// Creating and setting up the mainframe

		JFrame jfrMainFrame = new JFrame("JoLauncher ALPHA");
		jfrMainFrame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

		// Look and Feel setting

		JFrame.setDefaultLookAndFeelDecorated(true);

		// Defining the toolbar and menu

		JMenuBar jmbMainMenu = this.guiMainMenu();
		
		// Setting up the panel
		
		panelPopulate(jfrMainFrame.getContentPane());

		// Adding them to the mainframe

		jfrMainFrame.setJMenuBar(jmbMainMenu);

		// Setting the Frame Size

		jfrMainFrame.pack();

		// Display the frame

		jfrMainFrame.setVisible(true);

		// WE DONE!

		GUI.printlnConsole("GUI Initialized...done!");
		GUI.printlnConsole("Welcome to JoLauncher ALPHA!");
	}

	// Menu

	private JMenuBar guiMainMenu()
	{
		// Getting all the initializing out of the way

		JMenuBar menuBar;
		JMenu menu, submenu;
		JMenuItem menuItem;

		// Setting the menu bar up

		menuBar = new JMenuBar();

		// Launcher
		
		menu = new JMenu("Launcher");
		menu.setMnemonic(KeyEvent.VK_L);
		menuBar.add(menu);
		
		menuItem = new JMenuItem("Connect...", KeyEvent.VK_C);
		menuItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_F10, 0));
		menuItem.addActionListener(this);
		menuItem.setActionCommand("CONNECT");
		menu.add(menuItem);
		
		menu.addSeparator();

		menuItem = new JMenuItem("Exit", KeyEvent.VK_X);
		menuItem.addActionListener(this);
		menuItem.setActionCommand("EXIT");
		menu.add(menuItem);
		
		// Server

		menu = new JMenu("Server");
		menu.setMnemonic(KeyEvent.VK_S);
		menuBar.add(menu);

		menuItem = new JMenuItem("Update", KeyEvent.VK_U);
		menuItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_F5, 0));
		menuItem.addActionListener(this);
		menuItem.setActionCommand("UPDATE");
		menu.add(menuItem);

		menuItem = new JMenuItem("Refresh All", KeyEvent.VK_R);
		menuItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_F5, ActionEvent.CTRL_MASK));
		menuItem.addActionListener(this);
		menuItem.setActionCommand("REFRESH");
		menu.add(menuItem);

		menuItem = new JMenuItem("Refresh Server", KeyEvent.VK_E);
		menuItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_R, ActionEvent.CTRL_MASK));
		menuItem.addActionListener(this);
		menuItem.setActionCommand("REFRESH_ONE");
		menu.add(menuItem);

		// Options

		menu = new JMenu("Options");
		menu.setMnemonic(KeyEvent.VK_O);
		menuBar.add(menu);
		
		menuItem = new JMenuItem("Launcher Options...", KeyEvent.VK_L);
		menuItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_F2, 0));
		menu.add(menuItem);
		
		menuItem = new JMenuItem("Sorting Options...", KeyEvent.VK_S);
		menuItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_F3, 0));
		menu.add(menuItem);
		
		menuItem = new JMenuItem("Filter Options...", KeyEvent.VK_F);
		menuItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_F4, 0));
		menu.add(menuItem);
		
		// Help

		menu = new JMenu("Help");
		menu.setMnemonic(KeyEvent.VK_H);
		menuBar.add(menu);

		menuItem = new JMenuItem("About...", KeyEvent.VK_A);
		menuItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_F1, 0));
		menuItem.addActionListener(this);
		menuItem.setActionCommand("ABOUT");
		menu.add(menuItem);

		return menuBar;
	}

	// Toolbar
	
	private void guiAddButtons(JToolBar jtbToolBar)
	{
		JButton button = null;

		button = new JButton("Connect");
		button.setToolTipText("Connect");
		button.addActionListener(this);
		button.setActionCommand("CONNECT");
		jtbToolBar.add(button);
		
		jtbToolBar.addSeparator();

		button = new JButton("Update");
		button.setToolTipText("Update");
		button.addActionListener(this);
		button.setActionCommand("UPDATE");
		jtbToolBar.add(button);

		button = new JButton("Refresh All");
		button.setToolTipText("Refresh All");
		button.addActionListener(this);
		button.setActionCommand("REFRESH");
		jtbToolBar.add(button);
		
		button = new JButton("Refresh Server");
		button.setToolTipText("Refresh Server");
		button.addActionListener(this);
		button.setActionCommand("REFRESH_ONE");
		jtbToolBar.add(button);

		jtbToolBar.addSeparator();

		button = new JButton("Launcher Options");
		button.setToolTipText("Launcher Options");
		button.addActionListener(this);
		button.setActionCommand("LAUNCHER_OPTIONS");
		jtbToolBar.add(button);
		
		button = new JButton("Sorting Options");
		button.setToolTipText("Sorting Options");
		button.addActionListener(this);
		button.setActionCommand("SORTING_OPTIONS");
		jtbToolBar.add(button);

		button = new JButton("Filter Options");
		button.setToolTipText("Filter Options");
		button.addActionListener(this);
		button.setActionCommand("FILTER_OPTIONS");
		jtbToolBar.add(button);

		jtbToolBar.addSeparator();
		
		button = new JButton("About");
		button.setToolTipText("About");
		button.addActionListener(this);
		button.setActionCommand("ABOUT");
		jtbToolBar.add(button);
	}
	
	// Server Table
	
	private JScrollPane guiServers()
	{
		// Sets up the table headers

		Object objColumn[] = {"Server", "Address", "Players", "IWAD", "PWAD", "Map", "Ping"};
		
		// Sets up the Data Table Model
		
		dtmServersData = new DefaultTableModel(objColumn, 0)
		{
			public boolean isCellEditable(int a, int b)
			{
				return false;
			}
		};

		// Sets up the table itself
		
		jtbServers = new JTable(dtmServersData);

		/*
		
		jtbServers.setAutoCreateColumnsFromModel(false);

		TableColumn column;

		for(int i = 0; i < objColumn.length; i++)
		{

			column = new TableColumn(i);
			column.setHeaderValue(objColumn[i]);

			jtbServers.addColumn(column);
		
		}

		*/
		
		// Slaps a scroll bar on it
		
		jspServers = new JScrollPane(jtbServers);

		return jspServers;
	}
	
	// Console

	private JScrollPane guiConsole()
	{
		// Sets up the Console

		jtaConsole = new JTextArea(6, 80);
		jtaConsole.setEditable(false);

		// Slaps a scroll bar on it

		jspConsole = new JScrollPane(jtaConsole);

		return jspConsole;
	}

	// Command Line

	private JTextField guiCommandLine()
	{
		// Initializes the command line

		jtfCommandLine = new JTextField(80);

		return jtfCommandLine;
	}

	// These methods are interaction methods, as opposed to creation
	// methods
	
	// Adds a row to the server table

	public static void addServer(String strName, String strAddress, String strPlayers, String strMapName, String strIWAD, String strPWAD, String strPing)
	{
		Object objRow[] = {strName, strAddress, strPlayers, strIWAD, strPWAD ,strMapName ,strPing};

		dtmServersData.addRow(objRow);
	}

	// Replaces an already-existing row on the server table
	
	public static void replaceServer(String strName, int intIndex, String strPlayers, String strMapName, String strIWAD, String strPWAD, String strPing)
	{
		Object objRow[] = {strName, "", strPlayers, strIWAD, strPWAD, strMapName, strPing};
		
		// Loop through the avalable columns

		for(int i = 0; i <= 6; i++)
		{

			// Skip 1, since it's already known and by defenition can't
			// change, since it was our criteria for finding the dupe
			// in the first place
			
			if(i != 1)
			{
				dtmServersData.setValueAt(objRow[i], intIndex, i);
			}
		}
	}

	// Deletes a server
	
	public static void removeServer(int intIndex)
	{
		dtmServersData.removeRow(intIndex);
	}
	
	// Checks the server table for an IP that already exists.  Returns
	// -1 if not found, retuns row number if found
	
	public static int findServer(String strServer)
	{
		int intReturn = -1;
		
		// Loops through the list of servers
		
		for(int i = 0; i < dtmServersData.getRowCount(); i++)
		{
			// Gets the contents of the Address field
			
			String strAddressPort = String.valueOf(dtmServersData.getValueAt(i,1));
			
			if(strServer.equals(strAddressPort) == true)
			{
				intReturn = i;
			}
		}

		return intReturn;
	}
	
	// Clears the server table

	public static void clearServers()
	{
		while(dtmServersData.getRowCount() != 0)
		{
			dtmServersData.removeRow(0);
		}
	}
		
	// Prints to the console

	public static void printConsole(String strInput)
	{
		jtaConsole.append(strInput);
		GUI.scrollBottom();
	}

	// Prints a line to the console

	public static void printlnConsole(String strInput)
	{
		printConsole(strInput + "\n");
		GUI.scrollBottom();
	}

	// Scroll down automatically
	//
	// FIXME: Sometimes the scroll bar will not scroll all the way down

	public static void scrollBottom()
	{
		JScrollBar jsbVertical = jspConsole.getVerticalScrollBar();
		jsbVertical.setValue(jsbVertical.getMaximum());
	}
	
	// Needs more ACTION.  This handles the Action Events

	public void actionPerformed(java.awt.event.ActionEvent e)
	{
		String strAction = e.getActionCommand();

		if(strAction.equals("ABOUT"))
		{
			actAction.parse("about");
		}
		
		// Someone entered something in the console

		else if(strAction.equals("CONSOLE"))
		{

			// Parse the console to the parser
			
			String strInput = jtfCommandLine.getText();
			actAction.parse(strInput);

			// Because we're actually getting something from the command line
			// we have to clean up after ourselves
			
			jtfCommandLine.setText("");
		}

		else if(strAction.equals("EXIT"))
		{
			actAction.parse("exit");
		}
		
		// This particular action can ONLY be called from a menu or button
		else if(strAction.equals("CONNECT"))
		{
			// Sanity checks

			if(jtbServers.getSelectedRow() == -1)
			{
				Messenger.ERROR_NO_SERVER_SELECTED();
			}
			else
			{
				actAction.parse("connect " + String.valueOf(jtbServers.getValueAt(jtbServers.getSelectedRow(), 1)));
			}
		}
		
		else if(strAction.equals("REFRESH"))
		{
			actAction.parse("refresh");
		}

		// This particular action can ONLY be called from a menu or button
		else if(strAction.equals("REFRESH_ONE"))
		{
			// Sanity checks

			if(jtbServers.getSelectedRow() == -1)
			{
				Messenger.ERROR_NO_SERVER_SELECTED();
			}
			else
			{
				actAction.parse("refresh " + String.valueOf(jtbServers.getValueAt(jtbServers.getSelectedRow(), 1)));
			}
		}
		
		else if(strAction.equals("UPDATE"))
		{
			actAction.parse("update");
		}
	}
}
