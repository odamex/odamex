#include <sstream>
#include <time.h>

#import "CocoaUIController.h"
#import "i_net.h"
#import "../common/version.h"

unsigned long I_MSTime (void)
{
	struct timeval tv;
	struct timezone tz;
	
	gettimeofday(&tv, &tz);
	
	return (unsigned long)(tv.tv_sec)*1000 + (unsigned long)(tv.tv_usec)/1000;
}

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
	{ @"launch", @"odalaunch", @"Connect", @"Connect to selected server" },
	{ @"refresh", @"btnrefresh", @"Refresh", @"Refresh servers" },
	{ @"refreshall", @"btnrefreshall", @"Reload", @"Reload server list" },
	{ @"prev", @"btnlist", @"RefreshSelected", @"Refresh selected server" },
	{ @"pref", @"btnsettings", @"Preferences", @"Preferences" },
	{ @"help", @"btnhelp", @"Help", @"Help" },
	{ @"quit", @"btnexit", @"Quit", @"Quit Cocoa UI" }
};

static char *Masters[] = { "odamex.net:15000", "voxelsoft.com:15000"};
static bool MasterReply[] = {false, false}, MasterFirstReply;

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
	[NSThread detachNewThreadSelector: @selector(threadRecv:) toTarget:self withObject:nil];
//	[NSTimer scheduledTimerWithTimeInterval:invocation:repeats:]
	[NSTimer scheduledTimerWithTimeInterval:(NSTimeInterval)0.2 target:self selector:@selector(timerRefresh:) userInfo:nil repeats:(BOOL)true];
	
	[self reloadMasters];
}

- (void)textDidEndEditing:(NSNotification *)aNotification
{
	NSLog(@"lol!!!");
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
		@"launch", 
		NSToolbarSeparatorItemIdentifier,
		@"refresh", 
		@"refreshall", 
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
/*static NSString *serverColumns[] = { 
	@"serverName", @"ping", @"players", 
	@"wads", @"map", @"type", 
	@"gameIWAD", @"addressPort"
};
*/
/*
 *	Players NSTableView column keys
 */
//static NSString *playerColums[] = { 
//	@"playerName", @"frags", @"ping", @"unknown" 
//};


/*
 *	recreate the array on each call
 */
- (void)fillServersArray: (bool)merge
{
	if(!merge)
	{
		if (serversArray != nil) [serversArray release];
		serversArray = [[NSMutableArray alloc] init];
	}
	else
	{
		if(serversArray == nil)
			serversArray = [[NSMutableArray alloc] init]; 
	}
	
	unsigned short servers = MSG_ReadShort();

	for (size_t i = 0; i < servers; i++)
	{
		if(MSG_BytesLeft() < 6)
			break;

		netadr_t remote;
		remote.ip[0] = MSG_ReadByte();
		remote.ip[1] = MSG_ReadByte();
		remote.ip[2] = MSG_ReadByte();
		remote.ip[3] = MSG_ReadByte();
		remote.port = ntohs(MSG_ReadShort());

		if(merge)
		{
			size_t num_servers = [serversArray count], j;
			
			for(j = 0; j < num_servers; j++)
			{
				netadr_t expect;
				
				NSMutableDictionary *r = [serversArray objectAtIndex:j];
				NSString *address = [r objectForKey:@"addressPort"];
				NET_StringToAdr((char *)[address UTF8String], &expect);

				if(NET_CompareAdr(expect, remote))
					break;
			}
			
			NSMutableDictionary *row = [[NSMutableDictionary alloc] init];
			[row setObject:[NSString stringWithCString:NET_AdrToString(remote)] forKey: @"addressPort"];
			
			if(j == num_servers)
			{
				[serversArray addObject: row];
			}
			else
			{
				[serversArray replaceObjectAtIndex: j withObject: row];
			}
		}
		else
		{
			NSMutableDictionary *row = [[NSMutableDictionary alloc] init];
			[row setObject:[NSString stringWithCString:NET_AdrToString(remote)] forKey: @"addressPort"];
			[serversArray addObject: row];
		}
	}

	[totalServers setStringValue: [NSString stringWithFormat: @"Total Servers: %d", [serversArray count]]];
	[serversTableView reloadData];
}

- (void)fillPlayersArray: (int)rowIndex
{
	if (playersArray != nil) [playersArray release];
	playersArray = [[NSMutableArray alloc] init];

	NSMutableDictionary *server_row = [serversArray objectAtIndex:rowIndex];
	NSMutableArray *players = [server_row objectForKey:@"playerinfos"];

	size_t count = [players count];

	for (size_t j = 0; j < count; j++)
	{
		NSMutableDictionary *player = [players objectAtIndex:j];
		NSMutableDictionary *player_row = [[NSMutableDictionary alloc] init];
		[player_row setObject:[player objectForKey:@"playerName"] forKey:@"playerName"];
		[player_row setObject:[player objectForKey:@"frags"] forKey:@"frags"];
		[player_row setObject:[player objectForKey:@"ping"] forKey:@"ping"];
		[player_row setObject:[player objectForKey:@"team"] forKey:@"team"];
		[playersArray addObject:player_row];
	}	

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
	
	if ([itemIdent isEqualToString: @"launch"])
	{
		[self launch:serversTableView];
		//[masterPing setStringValue: 
		//	[NSString stringWithFormat: @"Master Ping: %d", masterPingCount++]];
	}
	else if ([itemIdent isEqualToString: @"refresh"])
	{
		refreshListPosition = 0;
	}
	else if ([itemIdent isEqualToString: @"refreshall"])
	{
		[self reloadMasters];
	}
	else if ([itemIdent isEqualToString: @"prev"]) {
		/*
		 *	Preview - not implemented
		 */
		NSRunAlertPanel(@"Cocoa UI", @"Not implemented", @"OK", nil, nil);
	}
	else if ([itemIdent isEqualToString: @"pref"])
	{
		[windowPreferences makeKeyAndOrderFront:nil];
	}
	else if ([itemIdent isEqualToString: @"help"])
	{
		/*
		 *	Help - not implemented
		 */
		NSRunAlertPanel(@"Odamex OSX Launcher", @"By Denis Lukianov\nCopyright (C) 2007 The Odamex Team", @"OK", nil, nil);						
	}
	else if ([itemIdent isEqualToString: @"quit"])
	{
		/*
		 *	Quit - terminate the application
		 */
		[NSApp terminate: self];
	}
	else 
		NSLog(@"Item clicked: %@", [toolbarItem itemIdentifier]);
}

- (void)launch: (NSTableView *)tableView
{
	int clickedRowIndex = [tableView selectedRow];
	
	/*
	 * check if a row was clicked
	 */
	if (clickedRowIndex == -1)
		return;
	
	NSArray *arguments = [[clientParameters stringValue] componentsSeparatedByString:@" "];
	NSString *path = [NSString stringWithFormat:@"%@/..", [[NSBundle mainBundle] bundlePath]];
	NSString *address = [[serversArray objectAtIndex:clickedRowIndex] objectForKey:@"addressPort"];
	NSArray *connect = [NSArray arrayWithObjects:@"-connect", address, nil];
	arguments = [connect arrayByAddingObjectsFromArray:arguments];
	
	NSTask *odamex = [[NSTask alloc] init];
	[odamex setCurrentDirectoryPath:path];
	[odamex setArguments:arguments];
	[odamex setLaunchPath:[NSString stringWithFormat:@"%@/odamex", path]];

	[odamex launch];
}

- (void)serversTableViewDoubleClicked: (NSTableView *)tableView
{
	[self launch:tableView];
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
	
	MasterFirstReply = false;
	
	for(size_t i = 0; i < sizeof(Masters)/sizeof(char *); i++)
	{
		NET_StringToAdr(Masters[i], &to);
		NET_SendPacket(buf.cursize, buf.data, to);
		MasterReply[i] = false;
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

- (void)threadServerPing:(id)param;
{

}

- (void)timerRefresh:(NSTimer*)theTimer
{
	size_t num_servers = [serversArray count];

	netadr_t to;
	buf_t buf(16);
	SZ_Clear(&buf);
	MSG_WriteLong(&buf, LAUNCHER_CHALLENGE);
	MSG_WriteLong(&buf, 1234); // todo check this anti-spoof key on reply, use random

	NSString *timestr = [NSString stringWithFormat: @"%d", I_MSTime()];

	for(size_t i = 0; i < 4 && refreshListPosition < num_servers; i++) // several servers at a time
	{
		NSMutableDictionary *row = [serversArray objectAtIndex:refreshListPosition++];

		NSString *address = [row objectForKey:@"addressPort"];

		NET_StringToAdr((char *)[address UTF8String], &to);
		NET_SendPacket(buf.cursize, buf.data, to);

		[row setObject:timestr forKey: @"pingSent"];
	}
}

- (void)threadRecv:(id)param;
{
	netadr_t expect;
	size_t i;

	while(NET_GetPacket())
	{
		// reply from a master?
		for(i = 0; i < sizeof(Masters)/sizeof(char *); i++)
		{
			NET_StringToAdr(Masters[i], &expect);
			
			if(!NET_CompareAdr(expect, net_from))
				continue;
				
			if(MSG_ReadLong() != LAUNCHER_CHALLENGE)
				break;
				
			// already got this master's rply?
			if(MasterReply[i])
				break;

			MasterReply[i] = true;
				
			// first master reply wipes the list
			if(MasterFirstReply)
			{
				if (serversArray != nil) [serversArray release];
				serversArray = [[NSMutableArray alloc] init];
				MasterFirstReply = true;
			}

			// merge results into server list
			[self fillServersArray: true];
			
			// initiate a server refresh
			refreshListPosition = 0;
		}
		
		if(i != sizeof(Masters)/sizeof(char *))
			continue;
		
		// reply from a server, then?
		size_t num_servers = [serversArray count];
		
		for(i = 0; i < num_servers; i++)
		{
			NSMutableDictionary *row = [serversArray objectAtIndex:i];
			NSString *address = [row objectForKey:@"addressPort"];
			NET_StringToAdr((char *)[address UTF8String], &expect);

			if(NET_CompareAdr(expect, net_from))
			{
				if(MSG_ReadLong() != CHALLENGE)
					break;

				std::stringstream sst([[row objectForKey: @"pingSent"] UTF8String]);
				if(sst.str() != "N/A")
				{
					time_t old, current = I_MSTime();
					sst >> old;

					[row setObject:[NSString stringWithFormat: @"%d", current - old] forKey: @"ping"];
					[row setObject:@"N/A" forKey: @"pingSent"];
				}

				// enrich row entry
				MSG_ReadLong(); // token we don't really need here
				MSG_ReadLong(); // launcher key to prevent spoofing
				
				const char *title = MSG_ReadString();
				
				[row setObject:[NSString stringWithCString:title] forKey: @"serverName"];

				unsigned players = MSG_ReadByte();
				unsigned maxplayers = MSG_ReadByte();
				
				std::stringstream ss;
				ss << players << "/" << maxplayers;

				[row setObject:[NSString stringWithCString:ss.str().c_str()] forKey: @"players"];

				const char *map = MSG_ReadString();

				[row setObject:[NSString stringWithCString:map] forKey: @"map"];

				unsigned char numwads = MSG_ReadByte();
				
				[row setObject:[NSString stringWithCString:MSG_ReadString()] forKey: @"gameIWAD"];

				std::stringstream ssw;
				for(unsigned j = 1; j < numwads; j++)
					ssw << MSG_ReadString() << " ";

				[row setObject:[NSString stringWithCString:ssw.str().c_str()] forKey: @"wads"];

				bool deathmatch = MSG_ReadByte();
				MSG_ReadByte(); // skill
				bool teamplay = MSG_ReadByte();
				bool ctfmode = MSG_ReadByte();
				
				if(ctfmode)
				{
					[row setObject:[NSString stringWithCString:"CTF"] forKey: @"type"];
				}
				else if (teamplay)
				{
					if (deathmatch)
						[row setObject:[NSString stringWithCString:"TEAM"] forKey: @"type"];
					else
						[row setObject:[NSString stringWithCString:"TEAM COOP"] forKey: @"type"];
				}
				else if (deathmatch)
				{
					[row setObject:[NSString stringWithCString:"DM"] forKey: @"type"];
				}
				else
				{
					[row setObject:[NSString stringWithCString:"COOP"] forKey: @"type"];
				}
				
				
				NSMutableArray *playerinfos = [[NSMutableArray alloc] init];
				for(unsigned k = 0; k < players; k++)
				{
					NSMutableDictionary *player = [[NSMutableDictionary alloc] init];
					
					[player setObject:[NSString stringWithCString:MSG_ReadString()] forKey:@"playerName"];

					int frags = MSG_ReadShort(), ping = MSG_ReadLong(), team;
					if (teamplay || ctfmode)
						team = MSG_ReadByte();
					
					std::stringstream fss, pss, tss;
					fss << frags;
					pss << ping;
					tss << team;
					[player setObject:[NSString stringWithCString:fss.str().c_str()] forKey:@"frags"];
					[player setObject:[NSString stringWithCString:pss.str().c_str()] forKey:@"ping"];

					if (teamplay || ctfmode)
						[player setObject:[NSString stringWithCString:tss.str().c_str()] forKey:@"team"];
					else
						[player setObject:[NSString stringWithCString:"N/A"] forKey:@"team"];

					[playerinfos addObject:player];
				}
				[row setObject:playerinfos forKey: @"playerinfos"];
				
				if(i == [serversTableView selectedRow])
					[self fillPlayersArray:i];
				
				[serversTableView reloadData];
			}
		}
	} // end while
}

@end
