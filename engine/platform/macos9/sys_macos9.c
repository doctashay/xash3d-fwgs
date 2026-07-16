/* Native Classic Mac OS system integration not supplied by SDL. */

#include "platform/platform.h"
#include <sys/stat.h>

extern int MacOS9_CreateDirectory( const char *path );

int mkdir( const char *path, mode_t mode )
{
	(void)mode;
	return MacOS9_CreateDirectory( path ) ? 0 : -1;
}

void Platform_ShellExecute( const char *path, const char *parms )
{
	/* Launch Services is unavailable on pure Classic. Keep this non-fatal. */
	(void)parms;
	Con_Printf( S_WARN "Opening external path is unsupported on Mac OS 9: %s\n", path );
}

void Platform_SetStatus( const char *status )
{
	(void)status;
}

qboolean Platform_DebuggerPresent( void )
{
	return false;
}
