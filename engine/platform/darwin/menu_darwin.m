#include <TargetConditionals.h>

#if XASH_APPLE && !TARGET_OS_IPHONE

#include <AppKit/AppKit.h>
#include "common.h"

enum
{
	DARWIN_MENU_NEW_GAME = 1,
	DARWIN_MENU_LOAD_GAME,
	DARWIN_MENU_SAVE_GAME,
	DARWIN_MENU_MAIN_MENU,
	DARWIN_MENU_DISCONNECT,
	DARWIN_MENU_TOGGLE_FULLSCREEN,
	DARWIN_MENU_TOGGLE_CONSOLE,
	DARWIN_MENU_VIDEO_SETTINGS,
	DARWIN_MENU_POWERPC_OPTIONS,
};

@class XashMenuTarget;

static NSMenu *s_mainMenu = nil;
static XashMenuTarget *s_menuTarget = nil;

static void Darwin_QueueCommand( const char *command )
{
	Cbuf_AddText( command );
	Cbuf_AddText( "\n" );
}

@interface XashMenuTarget : NSObject
- (void)performEngineAction:(id)sender;
@end

@implementation XashMenuTarget
- (void)performEngineAction:(id)sender
{
	switch( [sender tag] )
	{
	case DARWIN_MENU_NEW_GAME:
		Darwin_QueueCommand( "newgame" );
		break;
	case DARWIN_MENU_LOAD_GAME:
		Darwin_QueueCommand( "menu_loadgame" );
		break;
	case DARWIN_MENU_SAVE_GAME:
		Darwin_QueueCommand( "menu_savegame" );
		break;
	case DARWIN_MENU_MAIN_MENU:
		Darwin_QueueCommand( "menu_main" );
		break;
	case DARWIN_MENU_DISCONNECT:
		Darwin_QueueCommand( "disconnect" );
		break;
	case DARWIN_MENU_TOGGLE_FULLSCREEN:
		Darwin_QueueCommand( "toggle fullscreen" );
		break;
	case DARWIN_MENU_TOGGLE_CONSOLE:
		Darwin_QueueCommand( "toggleconsole" );
		break;
	case DARWIN_MENU_VIDEO_SETTINGS:
		Darwin_QueueCommand( "menu_video" );
		break;
	case DARWIN_MENU_POWERPC_OPTIONS:
		Darwin_QueueCommand( "menu_powerpcoptions" );
		break;
	default:
		break;
	}
}
@end

static XashMenuTarget *Darwin_GetMenuTarget( void )
{
	if( !s_menuTarget )
		s_menuTarget = [[XashMenuTarget alloc] init];

	return s_menuTarget;
}

static NSMenuItem *Darwin_AddItem( NSMenu *menu, NSString *title, SEL action, NSString *keyEquivalent, NSInteger tag, id target )
{
	NSMenuItem *item = [[[NSMenuItem alloc] initWithTitle:title action:action keyEquivalent:keyEquivalent] autorelease];
	[item setTarget:target];
	[item setTag:tag];
	[menu addItem:item];
	return item;
}

static void Darwin_BuildAppMenu( NSMenu *rootMenu )
{
	NSString *appName = [[NSProcessInfo processInfo] processName];
	NSMenuItem *appRoot = [[[NSMenuItem alloc] initWithTitle:@"" action:NULL keyEquivalent:@""] autorelease];
	NSMenu *appMenu = [[[NSMenu alloc] initWithTitle:appName] autorelease];

	[rootMenu addItem:appRoot];
	[rootMenu setSubmenu:appMenu forItem:appRoot];

	[appMenu addItemWithTitle:[NSString stringWithFormat:@"About %@", appName] action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@""];
	[appMenu addItem:[NSMenuItem separatorItem]];
	[appMenu addItemWithTitle:[NSString stringWithFormat:@"Hide %@", appName] action:@selector(hide:) keyEquivalent:@"h"];
	[appMenu addItemWithTitle:@"Hide Others" action:@selector(hideOtherApplications:) keyEquivalent:@"h"];
	[[appMenu itemAtIndex:[appMenu numberOfItems] - 1] setKeyEquivalentModifierMask:(NSCommandKeyMask|NSAlternateKeyMask)];
	[appMenu addItemWithTitle:@"Show All" action:@selector(unhideAllApplications:) keyEquivalent:@""];
	[appMenu addItem:[NSMenuItem separatorItem]];
	[appMenu addItemWithTitle:[NSString stringWithFormat:@"Quit %@", appName] action:@selector(terminate:) keyEquivalent:@"q"];
}

static void Darwin_BuildGameMenu( NSMenu *rootMenu, XashMenuTarget *target )
{
	NSMenuItem *root = [[[NSMenuItem alloc] initWithTitle:@"Game" action:NULL keyEquivalent:@""] autorelease];
	NSMenu *menu = [[[NSMenu alloc] initWithTitle:@"Game"] autorelease];

	[rootMenu addItem:root];
	[rootMenu setSubmenu:menu forItem:root];

	Darwin_AddItem( menu, @"New Game", @selector(performEngineAction:), @"", DARWIN_MENU_NEW_GAME, target );
	Darwin_AddItem( menu, @"Load Game", @selector(performEngineAction:), @"", DARWIN_MENU_LOAD_GAME, target );
	Darwin_AddItem( menu, @"Save Game", @selector(performEngineAction:), @"", DARWIN_MENU_SAVE_GAME, target );
	[menu addItem:[NSMenuItem separatorItem]];
	Darwin_AddItem( menu, @"Main Menu", @selector(performEngineAction:), @"", DARWIN_MENU_MAIN_MENU, target );
	Darwin_AddItem( menu, @"Disconnect", @selector(performEngineAction:), @"", DARWIN_MENU_DISCONNECT, target );
}

static void Darwin_BuildViewMenu( NSMenu *rootMenu, XashMenuTarget *target )
{
	NSMenuItem *root = [[[NSMenuItem alloc] initWithTitle:@"View" action:NULL keyEquivalent:@""] autorelease];
	NSMenu *menu = [[[NSMenu alloc] initWithTitle:@"View"] autorelease];

	[rootMenu addItem:root];
	[rootMenu setSubmenu:menu forItem:root];

	Darwin_AddItem( menu, @"Toggle Fullscreen", @selector(performEngineAction:), @"", DARWIN_MENU_TOGGLE_FULLSCREEN, target );
	Darwin_AddItem( menu, @"Console", @selector(performEngineAction:), @"", DARWIN_MENU_TOGGLE_CONSOLE, target );
	[menu addItem:[NSMenuItem separatorItem]];
	Darwin_AddItem( menu, @"Video Settings", @selector(performEngineAction:), @"", DARWIN_MENU_VIDEO_SETTINGS, target );
	Darwin_AddItem( menu, @"PowerPC Options", @selector(performEngineAction:), @"", DARWIN_MENU_POWERPC_OPTIONS, target );
}

static void Darwin_BuildWindowMenu( NSMenu *rootMenu )
{
	NSMenuItem *root = [[[NSMenuItem alloc] initWithTitle:@"Window" action:NULL keyEquivalent:@""] autorelease];
	NSMenu *menu = [[[NSMenu alloc] initWithTitle:@"Window"] autorelease];

	[rootMenu addItem:root];
	[rootMenu setSubmenu:menu forItem:root];

	[menu addItemWithTitle:@"Minimize" action:@selector(performMiniaturize:) keyEquivalent:@"m"];
	[menu addItem:[NSMenuItem separatorItem]];
	[menu addItemWithTitle:@"Bring All to Front" action:@selector(arrangeInFront:) keyEquivalent:@""];

	[NSApp setWindowsMenu:menu];
}

void Darwin_InitMenuBar( void )
{
	XashMenuTarget *target;

	[NSApplication sharedApplication];

	if( s_mainMenu )
	{
		[NSApp setMainMenu:s_mainMenu];
		return;
	}

	target = Darwin_GetMenuTarget();
	s_mainMenu = [[NSMenu alloc] initWithTitle:@""];

	Darwin_BuildAppMenu( s_mainMenu );
	Darwin_BuildGameMenu( s_mainMenu, target );
	Darwin_BuildViewMenu( s_mainMenu, target );
	Darwin_BuildWindowMenu( s_mainMenu );

	[NSApp setMainMenu:s_mainMenu];
}

void Darwin_ShutdownMenuBar( void )
{
	if( s_mainMenu )
	{
		if( [NSApp mainMenu] == s_mainMenu )
			[NSApp setMainMenu:nil];

		[s_mainMenu release];
		s_mainMenu = nil;
	}

	if( s_menuTarget )
	{
		[s_menuTarget release];
		s_menuTarget = nil;
	}
}

#endif // XASH_APPLE && !TARGET_OS_IPHONE
