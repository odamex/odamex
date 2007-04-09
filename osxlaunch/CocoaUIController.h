/* CocoaUIController */

#import <Cocoa/Cocoa.h>

@interface CocoaUIController : NSObject
{
    IBOutlet NSWindow *window;
    IBOutlet NSWindow *windowPreferences;
	
	NSMutableDictionary *toolbarItems;
	NSToolbar *toolbar;	/* toolbar outlet */
	
	int masterPingCount;
	
	/*
	 * "status" NSTextFields outlets
	 */
	NSTextField *masterPing;
	NSTextField *totalServers;
	NSTextField *totalPlayers;
	
	/*
	 *	Data store for the NSTableView's
	 */
	NSMutableArray *serversArray;
	NSMutableArray *playersArray;
	
	/*
	 * NSTableView outlets
	 */
	NSTableView *serversTableView;
	NSTableView *playersTableView;
	
	int refreshListPosition;
}

- (void)awakeFromNib;
- (void)release;

/*
 *	NSToolbar delegates
 */
- (NSArray *)toolbarAllowedItemIdentifiers: (NSToolbar *)toolbar;
- (NSArray *)toolbarDefaultItemIdentifiers: (NSToolbar *)toolbar;
- (NSToolbarItem *)toolbar: (NSToolbar *)toolbar
	itemForItemIdentifier: (NSString *)itemIdentifier
	willBeInsertedIntoToolbar: (BOOL)flag;

 
/*
 *	NSTableView related methods
 */

- (void)fillServersArray;
- (void)fillPlayersArray: (int)rowIndex;

- (int)numberOfRowsInTableView: (NSTableView *)tableView;
- (id)tableView: (NSTableView *)tableView
	objectValueForTableColumn: (NSTableColumn *)tableColumn
	row: (int)rowIndex;


/*
 *	Event handling
 */
- (void)toolbarItemClicked: (NSToolbarItem *)toolbarItem;
- (void)serversTableViewDoubleClicked: (NSTableView *)tableView;
- (void)tableViewSelectionDidChange: (NSNotification *)notification;
- (void)playersTableViewDoubleClicked: (NSTableView *)tableView;
- (void)tableView: (NSTableView*)inTableView didClickTableColumn:(NSTableColumn *)inTableColumn;
- (void)launch: (NSTableView *)tableView;

/*
 * Server processes
 */
 
- (void)reloadMasters;
- (void)reloadServers;

- (void)timerRefresh:(NSTimer*)theTimer;
- (void)threadRecv:(id)param;

@end
