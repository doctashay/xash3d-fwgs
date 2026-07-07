#include "common.h"
#include <TargetConditionals.h>

#if XASH_APPLE && !TARGET_OS_IPHONE

#include <AppKit/AppKit.h>
#include <IOKit/pwr_mgt/IOPMLib.h>

static NSMenu *s_mainMenu = nil;
static IOPMAssertionID s_powerAssertion = kIOPMNullAssertionID;

static NSString *Darwin_AppName( void )
{
	NSBundle *bundle = [NSBundle mainBundle];
	NSString *appName = [bundle objectForInfoDictionaryKey:@"CFBundleDisplayName"];

	if( !appName )
		appName = [bundle objectForInfoDictionaryKey:@"CFBundleName"];
	if( !appName )
		appName = [[NSProcessInfo processInfo] processName];

	return appName;
}

static void Darwin_BuildAppMenu( NSMenu *rootMenu )
{
	NSString *appName = Darwin_AppName();
	NSMenuItem *appRoot = [[[NSMenuItem alloc] initWithTitle:@"" action:NULL keyEquivalent:@""] autorelease];
	NSMenu *appMenu = [[[NSMenu alloc] initWithTitle:appName] autorelease];
	NSMenuItem *hideOthers;

	[rootMenu addItem:appRoot];
	[rootMenu setSubmenu:appMenu forItem:appRoot];

	[appMenu addItemWithTitle:[NSString stringWithFormat:@"About %@", appName] action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@""];
	[appMenu addItem:[NSMenuItem separatorItem]];
	[appMenu addItemWithTitle:[NSString stringWithFormat:@"Hide %@", appName] action:@selector(hide:) keyEquivalent:@"h"];
	hideOthers = [appMenu addItemWithTitle:@"Hide Others" action:@selector(hideOtherApplications:) keyEquivalent:@"h"];
	[hideOthers setKeyEquivalentModifierMask:(NSCommandKeyMask|NSAlternateKeyMask)];
	[appMenu addItemWithTitle:@"Show All" action:@selector(unhideAllApplications:) keyEquivalent:@""];
	[appMenu addItem:[NSMenuItem separatorItem]];
	[appMenu addItemWithTitle:[NSString stringWithFormat:@"Quit %@", appName] action:@selector(terminate:) keyEquivalent:@"q"];
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
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	[NSApplication sharedApplication];

	if( s_mainMenu )
	{
		[NSApp setMainMenu:s_mainMenu];
		[pool release];
		return;
	}

	s_mainMenu = [[NSMenu alloc] initWithTitle:@""];
	Darwin_BuildAppMenu( s_mainMenu );
	Darwin_BuildWindowMenu( s_mainMenu );

	[NSApp setMainMenu:s_mainMenu];
	[pool release];
}

void Darwin_ShutdownMenuBar( void )
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	if( s_mainMenu )
	{
		if( [NSApp mainMenu] == s_mainMenu )
			[NSApp setMainMenu:nil];

		[s_mainMenu release];
		s_mainMenu = nil;
	}

	[pool release];
}

void Darwin_AcquirePowerAssertion( void )
{
	if( s_powerAssertion != kIOPMNullAssertionID )
		return;

	if( IOPMAssertionCreate( CFSTR( "NoDisplaySleepAssertion" ), kIOPMAssertionLevelOn, &s_powerAssertion ) != kIOReturnSuccess )
		s_powerAssertion = kIOPMNullAssertionID;
}

void Darwin_ReleasePowerAssertion( void )
{
	if( s_powerAssertion == kIOPMNullAssertionID )
		return;

	IOPMAssertionRelease( s_powerAssertion );
	s_powerAssertion = kIOPMNullAssertionID;
}

#endif // XASH_APPLE && !TARGET_OS_IPHONE
