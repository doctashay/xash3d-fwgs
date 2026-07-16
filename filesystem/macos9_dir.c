/* macos9_dir.c - native Classic Mac OS directory enumeration */

#include "build.h"

#if XASH_MACOS9

#include <Files.h>
#include <fcntl.h>
#include <string.h>

#include "filesystem_internal.h"


static qboolean MacOS9_CStringToPascal( const char *src, size_t len, Str255 dst )
{
	if( len > 255 )
		return false;
	dst[0] = (unsigned char)len;
	memcpy( dst + 1, src, len );
	return true;
}

static OSErr MacOS9_GetDirectoryInfo( short vref, long dirid, CInfoPBRec *catalog )
{
	Str255 name;

	memset( catalog, 0, sizeof( *catalog ));
	catalog->dirInfo.ioNamePtr = name;
	catalog->dirInfo.ioVRefNum = vref;
	catalog->dirInfo.ioFDirIndex = -1;
	catalog->dirInfo.ioDrDirID = dirid;
	return PBGetCatInfoSync( catalog );
}

static OSErr MacOS9_FindDirectory( short vref, long parent, const char *name,
	size_t namelen, long *dirid )
{
	CInfoPBRec catalog;
	Str255 pascalName;
	OSErr err;

	if( !MacOS9_CStringToPascal( name, namelen, pascalName ))
		return bdNamErr;
	memset( &catalog, 0, sizeof( catalog ));
	catalog.dirInfo.ioNamePtr = pascalName;
	catalog.dirInfo.ioVRefNum = vref;
	catalog.dirInfo.ioFDirIndex = 0;
	catalog.dirInfo.ioDrDirID = parent;
	err = PBGetCatInfoSync( &catalog );
	if( err == noErr && !( catalog.dirInfo.ioFlAttrib & kioFlAttribDirMask ))
		return dirNFErr;
	if( err == noErr )
		*dirid = catalog.dirInfo.ioDrDirID;
	return err;
}

static OSErr MacOS9_ResolveDirectory( const char *path, short *vref, long *dirid )
{
	const char *component;
	OSErr err;

	err = HGetVol( NULL, vref, dirid );
	if( err != noErr )
		return err;

	component = path;
	while( *component )
	{
		const char *end;
		size_t len;

		while( *component == '/' || *component == '\\' )
			component++;
		if( !*component )
			break;

		end = component;
		while( *end && *end != '/' && *end != '\\' )
			end++;
		len = (size_t)( end - component );

		if( len == 1 && component[0] == '.' )
		{
			/* Already at the current directory. */
		}
		else if( len == 2 && component[0] == '.' && component[1] == '.' )
		{
			CInfoPBRec catalog;
			err = MacOS9_GetDirectoryInfo( *vref, *dirid, &catalog );
			if( err != noErr )
				return err;
			*dirid = catalog.dirInfo.ioDrParID;
		}
		else
		{
			err = MacOS9_FindDirectory( *vref, *dirid, component, len, dirid );
			if( err != noErr )
				return err;
		}

		component = end;
	}

	return noErr;
}

static OSErr MacOS9_GetPathInfo( const char *path, CInfoPBRec *catalog )
{
	char parentPath[1024];
	const char *end = path + strlen( path );
	const char *leaf;
	Str255 pascalName;
	short vref;
	long parent;
	size_t parentLength;

	while( end > path && ( end[-1] == '/' || end[-1] == '\\' ))
		end--;
	leaf = end;
	while( leaf > path && leaf[-1] != '/' && leaf[-1] != '\\' )
		leaf--;

	if( leaf == end )
		return fnfErr;

	parentLength = (size_t)( leaf - path );
	if( parentLength >= sizeof( parentPath ))
		return bdNamErr;
	memcpy( parentPath, path, parentLength );
	parentPath[parentLength] = '\0';
	if( MacOS9_ResolveDirectory( parentPath, &vref, &parent ) != noErr )
		return dirNFErr;

	if( !MacOS9_CStringToPascal( leaf, (size_t)( end - leaf ), pascalName ))
		return bdNamErr;
	memset( catalog, 0, sizeof( *catalog ));
	catalog->hFileInfo.ioNamePtr = pascalName;
	catalog->hFileInfo.ioVRefNum = vref;
	catalog->hFileInfo.ioFDirIndex = 0;
	catalog->hFileInfo.ioDirID = parent;
	return PBGetCatInfoSync( catalog );
}

qboolean MacOS9_FolderExists( const char *path )
{
	short vref;
	long dirid;
	return MacOS9_ResolveDirectory( path, &vref, &dirid ) == noErr;
}

qboolean MacOS9_FileExists( const char *path )
{
	CInfoPBRec catalog;
	OSErr err = MacOS9_GetPathInfo( path, &catalog );
	return err == noErr && !( catalog.hFileInfo.ioFlAttrib & kioFlAttribDirMask );
}

int MacOS9_FileTime( const char *path )
{
	CInfoPBRec catalog;
	const unsigned long macToUnixEpoch = 2082844800UL;

	if( MacOS9_GetPathInfo( path, &catalog ) != noErr ||
		( catalog.hFileInfo.ioFlAttrib & kioFlAttribDirMask ))
	{
		return -1;
	}
	if( catalog.hFileInfo.ioFlMdDat < macToUnixEpoch )
		return 0;
	return (int)( catalog.hFileInfo.ioFlMdDat - macToUnixEpoch );
}

qboolean MacOS9_FileOrFolderExists( const char *path )
{
	CInfoPBRec catalog;
	if( MacOS9_FolderExists( path ))
		return true;
	return MacOS9_GetPathInfo( path, &catalog ) == noErr;
}

int MacOS9_OpenFile( const char *path, int flags )
{
	char parentPath[1024];
	const char *end = path + strlen( path );
	const char *leaf;
	Str255 pascalName;
	short vref;
	short refnum;
	long parent;
	size_t parentLength;
	SInt8 permission;
	OSErr err;

	while( end > path && ( end[-1] == '/' || end[-1] == '\\' ))
		end--;
	leaf = end;
	while( leaf > path && leaf[-1] != '/' && leaf[-1] != '\\' )
		leaf--;
	if( leaf == end )
		return -1;

	parentLength = (size_t)( leaf - path );
	if( parentLength >= sizeof( parentPath ))
		return -1;
	memcpy( parentPath, path, parentLength );
	parentPath[parentLength] = '\0';
	if( MacOS9_ResolveDirectory( parentPath, &vref, &parent ) != noErr )
		return -1;

	if( !MacOS9_CStringToPascal( leaf, (size_t)( end - leaf ), pascalName ))
		return -1;
	if( flags & O_CREAT )
	{
		err = HCreate( vref, parent, pascalName, 'XASH', 'DATA' );
		if( err != noErr && err != dupFNErr )
			return -1;
	}

	switch( flags & O_ACCMODE )
	{
	case O_WRONLY: permission = fsWrPerm; break;
	case O_RDWR: permission = fsRdWrPerm; break;
	default: permission = fsRdPerm; break;
	}
	err = HOpenDF( vref, parent, pascalName, permission, &refnum );
	if( err != noErr )
		return -1;
	if( flags & O_TRUNC && SetEOF( refnum, 0 ) != noErr )
	{
		FSClose( refnum );
		return -1;
	}
	if( flags & O_APPEND && SetFPos( refnum, fsFromLEOF, 0 ) != noErr )
	{
		FSClose( refnum );
		return -1;
	}

	/* Retro68 libc reserves descriptors 0..9 and stores File Manager refs +10. */
	return refnum + 10;
}

static OSErr MacOS9_ResolveFileParent( const char *path, short *vref,
	long *parent, Str255 pascalName )
{
	char parentPath[1024];
	const char *end = path + strlen( path );
	const char *leaf;
	size_t parentLength;

	while( end > path && ( end[-1] == '/' || end[-1] == '\\' ))
		end--;
	leaf = end;
	while( leaf > path && leaf[-1] != '/' && leaf[-1] != '\\' )
		leaf--;
	if( leaf == end )
		return bdNamErr;

	parentLength = (size_t)( leaf - path );
	if( parentLength >= sizeof( parentPath ))
		return bdNamErr;
	memcpy( parentPath, path, parentLength );
	parentPath[parentLength] = '\0';
	if( !MacOS9_CStringToPascal( leaf, (size_t)( end - leaf ), pascalName ))
		return bdNamErr;
	return MacOS9_ResolveDirectory( parentPath, vref, parent );
}

int MacOS9_DeleteFile( const char *path )
{
	Str255 pascalName;
	short vref;
	long parent;
	OSErr err;

	err = MacOS9_ResolveFileParent( path, &vref, &parent, pascalName );
	if( err == fnfErr || err == dirNFErr )
		return 0;
	if( err != noErr )
		return (int)err;

	err = HDelete( vref, parent, pascalName );
	if( err == fnfErr || err == dirNFErr )
		return 0;
	return (int)err;
}

int MacOS9_RenameFile( const char *oldpath, const char *newpath )
{
	Str255 oldName, newName;
	short oldVRef, newVRef;
	long oldParent, newParent;
	OSErr err;

	err = MacOS9_ResolveFileParent( oldpath, &oldVRef, &oldParent, oldName );
	if( err != noErr )
		return (int)err;
	err = MacOS9_ResolveFileParent( newpath, &newVRef, &newParent, newName );
	if( err != noErr )
		return (int)err;
	if( oldVRef != newVRef )
		return (int)diffVolErr;

	if( oldParent == newParent )
		return (int)HRename( oldVRef, oldParent, oldName, newName );

	/* CatMove performs an intra-volume move and rename as one catalog
	   operation. Cross-volume rename has the same failure semantics as the
	   libc operation it replaces. */
	return (int)CatMove( oldVRef, oldParent, oldName, newParent, newName );
}

qboolean MacOS9_CreateDirectory( const char *path )
{
	char parentPath[1024];
	const char *end = path + strlen( path );
	const char *leaf;
	Str255 pascalName;
	short vref;
	long parent;
	long created;
	size_t parentLength;

	while( end > path && ( end[-1] == '/' || end[-1] == '\\' ))
		end--;
	leaf = end;
	while( leaf > path && leaf[-1] != '/' && leaf[-1] != '\\' )
		leaf--;
	if( leaf == end )
		return false;

	parentLength = (size_t)( leaf - path );
	if( parentLength >= sizeof( parentPath ))
		return false;
	memcpy( parentPath, path, parentLength );
	parentPath[parentLength] = '\0';
	if( MacOS9_ResolveDirectory( parentPath, &vref, &parent ) != noErr )
		return false;

	if( !MacOS9_CStringToPascal( leaf, (size_t)( end - leaf ), pascalName ))
		return false;
	return DirCreate( vref, parent, pascalName, &created ) == noErr;
}

qboolean MacOS9_SetCurrentDirectory( const char *path )
{
	short vref;
	long dirid;

	if( MacOS9_ResolveDirectory( path, &vref, &dirid ) != noErr )
		return false;
	return HSetVol( NULL, vref, dirid ) == noErr;
}

qboolean MacOS9_FlushFile( int fd )
{
	short vref;

	if( fd < 10 || GetVRefNum((short)( fd - 10 ), &vref ) != noErr )
		return false;
	return FlushVol( NULL, vref ) == noErr;
}

void MacOS9_ListDirectory( stringlist_t *list, const char *path, qboolean dirs_only )
{
	CInfoPBRec catalog;
	Str255 pascalName;
	short vref;
	short index;
	long dirid;

	if( MacOS9_ResolveDirectory( path, &vref, &dirid ) != noErr )
		return;

	for( index = 1; index > 0; index++ )
	{
		char name[256];
		unsigned char length;

		memset( &catalog, 0, sizeof( catalog ));
		catalog.dirInfo.ioNamePtr = pascalName;
		catalog.dirInfo.ioVRefNum = vref;
		catalog.dirInfo.ioFDirIndex = index;
		catalog.dirInfo.ioDrDirID = dirid;
		if( PBGetCatInfoSync( &catalog ) != noErr )
			break;
		if( dirs_only && !( catalog.dirInfo.ioFlAttrib & kioFlAttribDirMask ))
			continue;

		length = pascalName[0];
		memcpy( name, pascalName + 1, length );
		name[length] = '\0';
		stringlistappend( list, name );
	}
}

#endif /* XASH_MACOS9 */
