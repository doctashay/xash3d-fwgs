#include "filesystem_internal.h"
#include <string.h>

#if XASH_MACOS9
/*
 * Classic uses the C filesystem API directly.  Keep the required factory
 * export without pulling in the much larger legacy VFileSystem009 C++ shim.
 */
void EXPORT *CreateInterface( const char *interface, int *retval )
{
	if( interface && strcmp( interface, FS_API_CREATEINTERFACE_TAG ) == 0 )
	{
		if( retval )
			*retval = 0;
		return (void *)&g_api;
	}

	if( retval )
		*retval = 1;
	return NULL;
}
#endif
