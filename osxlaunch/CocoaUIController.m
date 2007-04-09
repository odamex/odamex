#import "CocoaUIController.h"
#import "i_net.h"
#import "../common/version.h"

@implementation CocoaUIController


/*
 *	info used to create the toolbar items 
 */
struct PrvToolbarItemInfo {
	NSString *name;
	NSString *imageName;
	NSString *label;
	NSString *tooltip;
};

static struct PrvToolbarItemInfo toolbarItemsInfo[] = {
	{ @"tool0", @"odalaunch", @"Tool 0", @"Increment the ping counter (not useful :P)" },
	{ @"tool1", @"btnrefresh", @"Tool 1", @"Reload server list" },
	{ @"tool2", @"btnrefreshall", @"Tool 2", @"Tool 2 tooltip" },
	{ @"prev", @"btnlist", @"Preview", @"Preview the file" },
	{ @"pref", @"btnsettings", @"Preferences", @"Show the preferences dialog" },
	{ @"help", @"btnhelp", @"Help", @"Show help window" },
	{ @"quit", @"btnexit", @"Quit", @"Quit Cocoa UI" }
};

static char *Masters[] = { "odamex.net:15000", "voxelsoft.com:15000"};

- (void)awakeFromNib
{
	masterPingCount = 0;
	serversArray = playersArray = nil;
	
	/*
	 * Initialize the toolbar
	 */
	int i;
	toolbarItems = [[NSMutableDictionary alloc] init];
	
	for (i = 0; i < sizeof(toolbarItemsInfo) / sizeof(toolbarItemsInfo[0]); i++) {
		NSToolbarItem *toolbarItem = [[NSToolbarItem alloc]
			initWithItemIdentifier: toolbarItemsInfo[i].name];

		[toolbarItem setLabel: toolbarItemsInfo[i].label];
		[toolbarItem setToolTip: toolbarItemsInfo[i].tooltip];
		[toolbarItem setImage:
			[NSImage imageNamed: toolbarItemsInfo[i].imageName]];
		[toolbarItem setTarget: self];
		[toolbarItem setAction: @selector(toolbarItemClicked:)];
		
		[toolbarItems setObject: toolbarItem forKey: toolbarItemsInfo[i].name]; 
		
		[toolbarItem release];
	}
			
	toolbar = [[NSToolbar alloc] initWithIdentifier: @"mainToolbar"];
	[toolbar setDisplayMode: NSToolbarDisplayModeIconOnly];
	[toolbar setDelegate: self];
	
	[window setToolbar: toolbar];
	[window setTitle: @"Odamex v" DOTVERSIONSTR " - OSX Launcher"];
	
	/*
	 *	bind the double-click event of servers NSTableView to the controller
	 */
	[serversTableView setTarget: self];
	[serversTableView setDoubleAction: @selector(serversTableViewDoubleClicked:)];

	/*
	 *	bind the double-click event of players NSTableView to the controller
	 */
	[playersTableView setTarget: self];
	[playersTableView setDoubleAction: @selector(playersTableViewDoubleClicked:)];

	// Startup networking
	localport = LAUNCHERPORT;
	InitNetCommon();
	
	// Startup packet receiving thread
	[NSThread detachNewThreadSelector: @selector(thread:) toTarget:self withObject:nil];
	
	[self reloadMasters];
}

- (void)release
{
	/*[toolbarItems release];*/
}


/*
 * NSToolbar delegates
 */
 
- (NSArray *)toolbarAllowedItemIdentifiers: (NSToolbar *)toolbar
{
	return [toolbarItems allKeys];
}

- (NSArray *)toolbarDefaultItemIdentifiers: (NSToolbar *)toolbar
{
	/*
	 *	return the item identifiers in the right order
	 *	(including the separators)
	 */
	return [NSArray arrayWithObjects: 
		@"tool0", 
		NSToolbarSeparatorItemIdentifier,
		@"tool1", 
		@"tool2", 
		@"prev", 
		NSToolbarSeparatorItemIdentifier,
		@"pref",
		@"help", 
		NSToolbarSeparatorItemIdentifier, 
		@"quit", 
		nil];
}

- (NSToolbarItem *)toolbar: (NSToolbar *)toolbar
	itemForItemIdentifier: (NSString *)itemIdentifier
	willBeInsertedIntoToolbar: (BOOL)flag
{
	return [toolbarItems objectForKey: itemIdentifier];
}


/*
 *	NSTableView related methods
 */

/*
 *	Servers NSTableView column keys
 */
static NSString *serverColumns[] = { 
	@"serverName", @"ping", @"players", 
	@"wads", @"map", @"type", 
	@"gameIWAD", @"addressPort"
};

/*
 *	Players NSTableView column keys
 */
//static NSString *playerColums[] = { 
//	@"playerName", @"frags", @"ping", @"unknown" 
//};


/*
 *	recreate the array on each call
 */
- (void)fillServersArray
{
	int i;
	
	if (serversArray != nil) [serversArray release];
	serversArray = [[NSMutableArray alloc] init]; 
	
	unsigned short servers = MSG_ReadShort();

	for (i = 0; i < servers; i++) {
	
		if(MSG_BytesLeft() < 6)
			break;

		NSMutableDictionary *row = [[NSMutableDictionary alloc] init];
		//int j;
		
		netadr_t remote;
		remote.ip[0] = MSG_ReadByte();
		remote.ip[1] = MSG_ReadByte();
		remote.ip[2] = MSG_ReadByte();
		remote.ip[3] = MSG_ReadByte();
		remote.port = ntohs(MSG_ReadShort());
		
		/*
		 *	build one row
		 */
		/*for (j = 0; j < sizeof(serverColumns) / sizeof(NSString *); j++) {
			NSString *value;
			
			switch (j) {
			case 0:
				value = [NSString stringWithFormat: @"Server Name %d", i];
				break;
			case 1:
				value = [NSString stringWithFormat: @"Ping %d", i];
				break;
			case 2:
				value = [NSString stringWithFormat: @"Players %d", i];
				break;
			case 3:
				value = [NSString stringWithFormat: @"WADs %d", i];
				break;
			case 4:
				value = [NSString stringWithFormat: @"Map %d", i];
				break;
			case 5:
				value = [NSString stringWithFormat: @"Type %d", i];
				break;
			case 6:
				value = [NSString stringWithFormat: @"Game IWAD %d", i];
				break;
			case 7:
				value = [NSString stringWithFormat: @"Address : Port %d", i];
				break;
			}*/
		[row setObject:[NSString stringWithCString:NET_AdrToString(remote)] forKey: serverColumns[7]]; 
		//}
		
		[serversArray addObject: row];
		[row release];
	}
	
	[totalServers setStringValue: 
		[NSString stringWithFormat: @"Total Servers: %d", [serversArray count]]];
	[serversTableView reloadData];
}

- (void)fillPlayersArray: (int)rowIndex
{
	/*static NSString *values[2][3][4] = {
		{
			{ @"Homer Simpson", @"12", @"13", @"Duh!!!" },
			{ @"Bugs Bunny", @"12", @"5", @"What's up doc ?" },
			{ @"Mickey Mouse", @"33", @"2", @"" }
		},
		{
			{ @"James Bond", @"12", @"34", @"My name is Bond, James Bond." },
			{ @"Popeye", @"44", @"3", @"" },
			{ @"Jackie Chan", @"55", @"44", @"" }
		}
	};
	int k = rowIndex % 2;
	int i, j;
	
	if (playersArray != nil) [playersArray release];
	playersArray = [[NSMutableArray alloc] init];
	
	for (j = 0; j < 3; j++) {
		NSMutableDictionary *row = [[NSMutableDictionary alloc] init];
		
		for (i = 0; i < 4; i++)
			[row setObject: values[k][j][i] forKey: playerColums[i]];
			
		[playersArray addObject: row];
		[row release];
	}
	
	[totalPlayers setStringValue: 
		[NSString stringWithFormat: @"Total Players: %d", [playersArray count]]];*/
	[playersTableView reloadData];
}

- (int)numberOfRowsInTableView: (NSTableView *)tableView
{
	if (tableView == serversTableView)
		return [serversArray count];
	else if (tableView == playersTableView)
		return [playersArray count];
		
	return 0;
}

- (id)tableView: (NSTableView *)tableView
	objectValueForTableColumn: (NSTableColumn *)tableColumn
	row: (int)rowIndex
{
	if (tableView == serversTableView)
		return [[serversArray objectAtIndex: rowIndex] objectForKey: [tableColumn identifier]];
	else if (tableView == playersTableView)
		return [[playersArray objectAtIndex: rowIndex] objectForKey: [tableColumn identifier]];
		
	return nil;
}


/*
 *	Event handling
 */
- (void)toolbarItemClicked: (NSToolbarItem *)toolbarItem
{
	NSString *itemIdent = [toolbarItem itemIdentifier];
	
	if ([itemIdent isEqualToString: @"tool0"]) {
		/*
		 *	Tool 0 - increments the master ping count
		 */	
		[masterPing setStringValue: 
			[NSString stringWithFormat: @"Master Ping: %d", masterPingCount++]];
	} else if ([itemIdent isEqualToString: @"tool1"]) {
		/*
		 *	Tool 1 - reload the server list
		 */
		[serversTableView reloadData];
	} else if ([itemIdent isEqualToString: @"tool2"]) {
		/*
		 *	Tool 2 - not implemented
		 */
		NSRunAlertPanel(@"Cocoa UI", @"Tool 2 code goes here...", @"OK", nil, nil);
	} else if ([itemIdent isEqualToString: @"prev"]) {
		/*
		 *	Preview - not implemented
		 */
		NSRunAlertPanel(@"Cocoa UI", @"Preview code goes here...", @"OK", nil, nil);
	} else if ([itemIdent isEqualToString: @"pref"]) {
		/*
		 *	Preferences - not implemented
		 */
		NSRunAlertPanel(@"Cocoa UI", @"Preferences code goes here...", @"OK", nil, nil);
	} else if ([itemIdent isEqualToString: @"help"]) {
		/*
		 *	Help - not implemented
		 */
		NSRunAlertPanel(@"Cocoa UI", @"Help code goes here...", @"OK", nil, nil);						
	} else if ([itemIdent isEqualToString: @"quit"]) {
		/*
		 *	Quit - terminate the application
		 */
		[NSApp terminate: self];
	} else 
		NSLog(@"Item clicked: %@", [toolbarItem itemIdentifier]);
}

- (void)serversTableViewDoubleClicked: (NSTableView *)tableView
{
	int clickedRowIndex = [tableView clickedRow];
	
	/*
	 * check if a row was clicked
	 */
	if (clickedRowIndex == -1)
		return;
	
	NSTask *odamex = [[NSTask alloc] init];
	
	NSString *address = [[serversArray objectAtIndex:clickedRowIndex] objectForKey:@"addressPort"];
	NSArray *arguments;
    arguments = [NSArray arrayWithObjects:@"-connect", address, nil];

    [odamex setLaunchPath:@"../../../odamex"];
	[odamex setArguments:arguments];

	[odamex launch];
}

- (void)tableViewSelectionDidChange: (NSNotification *)notification
{
	NSTableView *tableView = [notification object];
	
	if (tableView == serversTableView) {
		[self fillPlayersArray: [tableView selectedRow]];
		[playersTableView reloadData];
	}
}

- (void)playersTableViewDoubleClicked: (NSTableView *)tableView
{
	int clickedRowIndex = [tableView clickedRow];
	
	/*
	 * check if a row was clicked
	 */
	if (clickedRowIndex != -1)
		NSRunAlertPanel(@"Cocoa UI",
			@"The row with index %d was clicked.\nThe player name is \"%@\".",
			@"OK", nil, nil, 
			clickedRowIndex, 
			[[playersArray objectAtIndex: clickedRowIndex] objectForKey: @"playerName"]);
}

- (void)tableView: (NSTableView*)inTableView didClickTableColumn:(NSTableColumn *)inTableColumn
{
//	[self sortByColumn:inTableColumn];
	NSRunAlertPanel(@"Cocoa UI", @"Not implemented", @"OK", nil, nil);						
}

- (void)reloadMasters
{
	// Request server list from master
	netadr_t to;
	buf_t buf(16);
	SZ_Clear(&buf);
	MSG_WriteLong(&buf, LAUNCHER_CHALLENGE);
	
	for(size_t i = 0; i < sizeof(Masters)/sizeof(char *); i++)
	{
		NET_StringToAdr(Masters[i], &to);
		NET_SendPacket(buf.cursize, buf.data, to);
	}
}

- (void)reloadServers
{
	// Request all server replies
	netadr_t to;
	buf_t buf(16);
	SZ_Clear(&buf);
	MSG_WriteLong(&buf, LAUNCHER_CHALLENGE);
	
	size_t num_servers = [serversArray count];
	
	for(size_t i = 0; i < num_servers; i++)
	{
		NSString *address = [[serversArray objectAtIndex:i] objectForKey:@"addressPort"];
		NET_StringToAdr((char *)[address UTF8String], &to);
		NET_SendPacket(buf.cursize, buf.data, to);
	}
}

- (void)thread:(id)param;
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	netadr_t expect;
	size_t i;

	while(NET_GetPacket())
	{
		// reply from a master?
		for(i = 0; i < sizeof(Masters)/sizeof(char *); i++)
		{
			NET_StringToAdr(Masters[i], &expect);
			
			if(MSG_ReadLong() != LAUNCHER_CHALLENGE)
				continue;
			
			if(NET_CompareAdr(expect, net_from))
			{
				// populate server list
				[self fillServersArray];
				
				// initiate a server refresh
				//todo
				
				break;
			}
		}
		
		if(i != sizeof(Masters)/sizeof(char *))
			continue;
			
		// reply from a server, then?
		size_t num_servers = [serversArray count];
		
		for(i = 0; i < num_servers; i++)
		{
			NSString *address = [[serversArray objectAtIndex:i] objectForKey:@"addressPort"];
			NET_StringToAdr((char *)[address UTF8String], &expect);

			if(NET_CompareAdr(expect, net_from))
			{
				// enrich row entry
				continue;
			}
		}
	}
	
	[pool release];
}

@end
