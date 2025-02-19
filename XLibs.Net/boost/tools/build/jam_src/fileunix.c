/*
 * Copyright 1993-2002 Christopher Seiwald and Perforce Software, Inc.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/*  This file is ALSO:
 *  (C) Copyright David Abrahams 2001. Permission to copy, use,
 *  modify, sell and distribute this software is granted provided this
 *  copyright notice appears in all copies. This software is provided
 *  "as is" without express or implied warranty, and with no claim as
 *  to its suitability for any purpose.
 */

# include "jam.h"
# include "filesys.h"
# include "strings.h"
# include "pathsys.h"
# include <stdio.h>

#if defined(sun) || defined(__sun)
# include <unistd.h> /* needed for read and close prototype */
#endif

# ifdef USE_FILEUNIX

#if defined(sun) || defined(__sun)
# include <unistd.h> /* needed for read and close prototype */
#endif

# if defined( OS_SEQUENT ) || \
     defined( OS_DGUX ) || \
     defined( OS_SCO ) || \
     defined( OS_ISC ) 
# define PORTAR 1
# endif

# ifdef __EMX__
# include <sys/types.h>
# include <sys/stat.h>
# endif

# if defined( OS_RHAPSODY ) || \
     defined( OS_MACOSX ) || \
     defined( OS_NEXT )
/* need unistd for rhapsody's proper lseek */
# include <sys/dir.h>
# include <unistd.h>
# define STRUCT_DIRENT struct direct 
# else
# include <dirent.h>
# define STRUCT_DIRENT struct dirent 
# endif

# ifdef OS_COHERENT
# include <arcoff.h>
# define HAVE_AR
# endif

# if defined( OS_MVS ) || \
		 defined( OS_INTERIX )

#define	ARMAG	"!<arch>\n"
#define	SARMAG	8
#define	ARFMAG	"`\n"

struct ar_hdr		/* archive file member header - printable ascii */
{
	char	ar_name[16];	/* file member name - `/' terminated */
	char	ar_date[12];	/* file member date - decimal */
	char	ar_uid[6];	/* file member user id - decimal */
	char	ar_gid[6];	/* file member group id - decimal */
	char	ar_mode[8];	/* file member mode - octal */
	char	ar_size[10];	/* file member size - decimal */
	char	ar_fmag[2];	/* ARFMAG - string to end header */
};

# define HAVE_AR
# endif

# if defined( OS_QNX ) || \
     defined( OS_BEOS ) || \
     defined( OS_MPEIX )
# define NO_AR
# define HAVE_AR
# endif

# ifndef HAVE_AR

# ifdef OS_AIX
/* Define those for AIX to get the definitions for both the small and the
 * big variant of the archive file format. */
#	 define __AR_SMALL__
#	 define __AR_BIG__
# endif

# include <ar.h>
# endif	

/*
 * fileunix.c - manipulate file names and scan directories on UNIX/AmigaOS
 *
 * External routines:
 *
 *	file_dirscan() - scan a directory for files
 *	file_time() - get timestamp of file, if not done by file_dirscan()
 *	file_archscan() - scan an archive for files
 *
 * File_dirscan() and file_archscan() call back a caller provided function
 * for each file found.  A flag to this callback function lets file_dirscan()
 * and file_archscan() indicate that a timestamp is being provided with the
 * file.   If file_dirscan() or file_archscan() do not provide the file's
 * timestamp, interested parties may later call file_time().
 *
 * 04/08/94 (seiwald) - Coherent/386 support added.
 * 12/19/94 (mikem) - solaris string table insanity support
 * 02/14/95 (seiwald) - parse and build /xxx properly
 * 05/03/96 (seiwald) - split into pathunix.c
 * 11/21/96 (peterk) - BEOS does not have Unix-style archives
 */

/*
 * file_dirscan() - scan a directory for files
 */

void
file_dirscan( 
	char *dir,
	scanback func,
	void *closure )
{
	PATHNAME f;
	DIR *d;
	STRUCT_DIRENT *dirent;
        string filename[1];

	/* First enter directory itself */

	memset( (char *)&f, '\0', sizeof( f ) );

	f.f_dir.ptr = dir;
	f.f_dir.len = strlen(dir);

	dir = *dir ? dir : ".";

	/* Special case / : enter it */

	if( f.f_dir.len == 1 && f.f_dir.ptr[0] == '/' )
	    (*func)( closure, dir, 0 /* not stat()'ed */, (time_t)0 );

	/* Now enter contents of directory */

	if( !( d = opendir( dir ) ) )
	    return;

	if( DEBUG_BINDSCAN )
	    printf( "scan directory %s\n", dir );

        string_new( filename );
	while( dirent = readdir( d ) )
	{
# ifdef old_sinix
	    /* Broken structure definition on sinix. */
	    f.f_base.ptr = dirent->d_name - 2;
# else
	    f.f_base.ptr = dirent->d_name;
# endif
	    f.f_base.len = strlen( f.f_base.ptr );

            string_truncate( filename, 0 );
	    path_build( &f, filename, 0 );

	    (*func)( closure, filename->value, 0 /* not stat()'ed */, (time_t)0 );
	}
        string_free( filename );

	closedir( d );
}

/*
 * file_time() - get timestamp of file, if not done by file_dirscan()
 */

int
file_time(
	char	*filename,
	time_t	*time )
{
	struct stat statbuf;

	if( stat( filename, &statbuf ) < 0 )
	    return -1;

    /* Technically, existing files can have 0 as statbuf.st_mtime 
       --- in particular, the /cygdrive directory under cygwin. However, 
       though all the code jam assumes that timestamp of 0 means
       "does not exist" and will try to create the "missing" target, causing
       problems. Work around this problem by chanding 0 to 1.
    */
	*time = statbuf.st_mtime ? statbuf.st_mtime : 1 ;
	return 0;
}

/*
 * file_archscan() - scan an archive for files
 */

# ifndef AIAMAG	/* God-fearing UNIX */

# define SARFMAG 2
# define SARHDR sizeof( struct ar_hdr )

void
file_archscan(
	char *archive,
	scanback func,
	void *closure )
{
# ifndef NO_AR
	struct ar_hdr ar_hdr;
	char buf[ MAXJPATH ];
	long offset;
	char    *string_table = 0;
	int fd;

	if( ( fd = open( archive, O_RDONLY, 0 ) ) < 0 )
	    return;

	if( read( fd, buf, SARMAG ) != SARMAG ||
	    strncmp( ARMAG, buf, SARMAG ) )
	{
	    close( fd );
	    return;
	}

	offset = SARMAG;

	if( DEBUG_BINDSCAN )
	    printf( "scan archive %s\n", archive );

	while( read( fd, &ar_hdr, SARHDR ) == SARHDR &&
	       !memcmp( ar_hdr.ar_fmag, ARFMAG, SARFMAG ) )
	{
	    char    lar_name_[257];
            char*   lar_name = lar_name_ + 1;
	    long    lar_date;
	    long    lar_size;
	    long    lar_offset;
	    char *c;
	    char    *src, *dest;

	    strncpy( lar_name, ar_hdr.ar_name, sizeof(ar_hdr.ar_name) );

	    sscanf( ar_hdr.ar_date, "%ld", &lar_date );
	    sscanf( ar_hdr.ar_size, "%ld", &lar_size );

	    if (ar_hdr.ar_name[0] == '/')
	    {
		if (ar_hdr.ar_name[1] == '/')
		{
		    /* this is the "string table" entry of the symbol table,
		    ** which holds strings of filenames that are longer than
		    ** 15 characters (ie. don't fit into a ar_name
		    */

		    string_table = (char *)malloc(lar_size);
		    lseek(fd, offset + SARHDR, 0);
		    if (read(fd, string_table, lar_size) != lar_size)
			printf("error reading string table\n");
		}
		else if (string_table && ar_hdr.ar_name[1] != ' ')
		{
		    /* Long filenames are recognized by "/nnnn" where nnnn is
		    ** the offset of the string in the string table represented
		    ** in ASCII decimals.
		    */
		    dest = lar_name;
		    lar_offset = atoi(lar_name + 1);
		    src = &string_table[lar_offset];
		    while (*src != '/')
			*dest++ = *src++;
		    *dest = '/';
		}
	    }

	    c = lar_name - 1;
	    while( *++c != ' ' && *c != '/' )
		;
	    *c = '\0';

	    if ( DEBUG_BINDSCAN )
		printf( "archive name %s found\n", lar_name );

	    sprintf( buf, "%s(%s)", archive, lar_name );

	    (*func)( closure, buf, 1 /* time valid */, (time_t)lar_date );

	    offset += SARHDR + ( ( lar_size + 1 ) & ~1 );
	    lseek( fd, offset, 0 );
	}

	if (string_table)
	    free(string_table);

	close( fd );

# endif /* NO_AR */

}

# else /* AIAMAG - RS6000 AIX */

static void file_archscan_small(
    int fd, char const *archive, scanback func, void *closure)
{
    struct fl_hdr fl_hdr;

    struct {
        struct ar_hdr hdr;
        char pad[ 256 ];
    } ar_hdr ;

    char buf[ MAXJPATH ];
    long offset;

    if( read( fd, (char *)&fl_hdr, FL_HSZ ) != FL_HSZ)
        return;

    sscanf( fl_hdr.fl_fstmoff, "%ld", &offset );
  
    if( DEBUG_BINDSCAN )
        printf( "scan archive %s\n", archive );
  
    while( offset > 0
           && lseek( fd, offset, 0 ) >= 0
           && read( fd, &ar_hdr, sizeof( ar_hdr ) ) >= sizeof( ar_hdr.hdr ) )
    {
        long    lar_date;
        int     lar_namlen;
    
        sscanf( ar_hdr.hdr.ar_namlen, "%d", &lar_namlen );
        sscanf( ar_hdr.hdr.ar_date, "%ld", &lar_date );
        sscanf( ar_hdr.hdr.ar_nxtmem, "%ld", &offset );
    
        if( !lar_namlen )
            continue;
      
        ar_hdr.hdr._ar_name.ar_name[ lar_namlen ] = '\0';

        sprintf( buf, "%s(%s)", archive, ar_hdr.hdr._ar_name.ar_name );

        (*func)( closure, buf, 1 /* time valid */, (time_t)lar_date );
    }
}

/* Check for OS version which supports the big variant. */
#ifdef AR_HSZ_BIG

static void file_archscan_big(
    int fd, char const *archive, scanback func, void *closure)
{
    struct fl_hdr_big fl_hdr;

    struct {
        struct ar_hdr_big hdr;
        char pad[ 256 ];
    } ar_hdr ;

    char buf[ MAXJPATH ];
    long long offset;

    if( read( fd, (char *)&fl_hdr, FL_HSZ_BIG) != FL_HSZ_BIG)
        return;

    sscanf( fl_hdr.fl_fstmoff, "%lld", &offset );

    if( DEBUG_BINDSCAN )
        printf( "scan archive %s\n", archive );

    while( offset > 0
           && lseek( fd, offset, 0 ) >= 0
           && read( fd, &ar_hdr, sizeof( ar_hdr ) ) >= sizeof( ar_hdr.hdr ) )
    {
        long    lar_date;
        int     lar_namlen;

        sscanf( ar_hdr.hdr.ar_namlen, "%d", &lar_namlen );
        sscanf( ar_hdr.hdr.ar_date, "%ld", &lar_date );
        sscanf( ar_hdr.hdr.ar_nxtmem, "%lld", &offset );

        if( !lar_namlen )
            continue;

        ar_hdr.hdr._ar_name.ar_name[ lar_namlen ] = '\0';

        sprintf( buf, "%s(%s)", archive, ar_hdr.hdr._ar_name.ar_name );

        (*func)( closure, buf, 1 /* time valid */, (time_t)lar_date );
    }

}

#endif /* AR_HSZ_BIG */

void file_archscan(char *archive, scanback func, void *closure)
{
    int fd;
    char fl_magic[SAIAMAG];

    if(( fd = open(archive, O_RDONLY, 0)) < 0)
        return;
  
    if(read( fd, fl_magic, SAIAMAG) != SAIAMAG
       || lseek(fd, 0, SEEK_SET) == -1)
    {
        close(fd);
        return;
    }

    if (strncmp(AIAMAG, fl_magic, SAIAMAG) == 0)
    {
        /* read small variant */
        file_archscan_small(fd, archive, func, closure);
    }
#ifdef AR_HSZ_BIG
    else if (strncmp(AIAMAGBIG, fl_magic, SAIAMAG) == 0)
    {
        /* read big variant */
        file_archscan_big(fd, archive, func, closure);
    }
#endif
  
    close( fd );
}

# endif /* AIAMAG - RS6000 AIX */

# endif /* USE_FILEUNIX */
