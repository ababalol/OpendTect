/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 1995
 * FUNCTION : Stream Provider functions
-*/

#include "Pmacros.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>

#ifdef __msvc__
# include <windows.h>
# define popen _popen
# define pclose _pclose
# define fileno(s) _fileno(s)
#include "errh.h"
#endif

#include "strmprov.h"
#include "filegen.h"
#include "string2.h"
#include "binidsel.h"
#include "strmoper.h"


static const char* rcsID = "$Id: strmprov.cc,v 1.15 2002-01-07 13:21:23 nanne Exp $";

static FixedString<1024> oscommand;
#ifdef __msvc__
#define exeCmd(comm) false
#else
#define exeCmd(comm) system((const char*)comm) ? false : true
#endif

DefineClassID(StreamProvider);
const char* StreamProvider::sStdIO = "Std-IO";


void StreamData::close()
{
    if ( fp && fp != stdin && fp != stdout && fp != stderr )
	{ if ( ispipe ) pclose(fp); else fclose(fp); }
    if ( istrm && istrm != &cin )
	delete istrm;
    if ( ostrm )
    {
	ostrm->flush();
	if ( ostrm != &cout && ostrm != &cerr )
	delete ostrm;
    }

    init();
}


void StreamData::init()
{
    fp = 0; istrm = 0; ostrm = 0; ispipe = false;
}


bool StreamData::usable() const
{
    return ( istrm || ostrm )
	&& ( !istrm || !istrm->bad() )
	&& ( !ostrm || (!ostrm->bad() && !ostrm->eof()) )
	&& ( !ispipe || fp );
}


StreamProvider::StreamProvider( const char* devname )
{
    set( devname );
}


StreamProvider::StreamProvider( const char* hostnm, const char* fnm,
				StreamConn::Type typ )
	: isbad(false)
	, type_(typ)
	, hostname(hostnm)
	, fname(fnm?fnm:sStdIO)
{
    if ( fname == "" ) isbad = true;
}


void StreamProvider::set( const char* devname )
{
    type_ = StreamConn::File;
    isbad = false;
    blocksize = 0;
    hostname = "";

    if ( !devname || !strcmp(devname,sStdIO) )
    {
	type_ = StreamConn::File;
	fname = sStdIO;
	return;
    }
    else if ( !*devname )
    {
	isbad = true; fname = "";
	return;
    }

    char* ptr = (char*)devname;
    if ( *ptr == '@' ) { type_ = StreamConn::Command; ptr++; }
    fname = ptr;

#ifndef __msvc__
    // separate hostname from filename
    ptr = strchr( fname.buf(), ':' );
    if ( ptr )
    {
	*ptr++ = '\0';
	hostname = fname;
    }
    else
#endif
	ptr = fname.buf();

    if ( *ptr == '@' ) { type_ = StreamConn::Command; ptr++; }

    char* ptrname = fname.buf();
    while ( *ptr ) *ptrname++ = *ptr++;
    *ptrname = '\0';

    if ( type_ != StreamConn::Command && matchString( "/dev/", fname ) )
	type_ = StreamConn::Device;
}


bool StreamProvider::isNormalFile() const
{
    return type_ == StreamConn::File && hostname == "";
}


bool StreamProvider::rewind() const
{
    if ( isbad ) return false;
    else if ( type_ != StreamConn::Device ) return true;

    if ( hostname[0] )
	sprintf( oscommand.buf(), "rsh %s \"mt -f %s rewind\"",
		 (const char*)hostname, (const char*)fname );
    else
	sprintf( oscommand.buf(), "mt -f %s rewind", (const char*)fname );
    return exeCmd(oscommand);
}


bool StreamProvider::offline() const
{
    if ( isbad ) return false;
    else if ( type_ != StreamConn::Device ) return true;

    if ( hostname[0] )
	sprintf( oscommand.buf(), "rsh %s \"mt -f %s offline\"",
			    (const char*)hostname, (const char*)fname );
    else
	sprintf( oscommand.buf(), "mt -f %s offline", (const char*)fname );
    return exeCmd(oscommand);
}


bool StreamProvider::skipFiles( int nr ) const
{
    if ( isbad ) return false;
    if ( type_ != StreamConn::Device ) return false;

    if ( hostname[0] )
	sprintf( oscommand.buf(), "rsh %s \"mt -f %s fsf %d\"",
			(const char*)hostname, (const char*)fname, nr );
    else
	sprintf( oscommand.buf(), "mt -f %s fsf %d", (const char*)fname, nr );
    return exeCmd(oscommand);
}


const char* StreamProvider::fullName() const
{
    oscommand = "";
    if ( type_ == StreamConn::Command )
	oscommand += "@";
    if ( hostname[0] ) 
    {
	oscommand += hostname;
	oscommand += ":";
    }
    oscommand += (const char*)fname;

    return oscommand;
}


void StreamProvider::addPathIfNecessary( const char* path )
{
    if ( isbad ) return;
    if ( type_ != StreamConn::File
      || !path || ! *path
      || fname == sStdIO
      || File_isAbsPath(fname)
      || (*(const char*)fname == '.' && *(((const char*)fname)+1) == *sDirSep) )
	return;

    FileNameString pth( path );
    fname = File_getFullPath( pth, fname );
}


StreamData StreamProvider::makeIStream() const
{
    StreamData sd;
    if ( isbad || !*(const char*)fname )
	return sd;
    if ( fname == sStdIO )
    {
	sd.istrm = &cin;
	return sd;
    }
    if ( type_ != StreamConn::Command && !hostname[0] )
    {
	if ( File_exists(fname) )
#ifdef __msvc__
/*

"
    From: Paul Lutus (nospam@nosite.com)
    Subject: Re: Question of fstream 
    Newsgroups: comp.lang.c++
    Date: 2000/03/14 

    Unix platforms end text file lines with a linefeed.

    DOS/Windows platforms use a carriage return/linefeed to end each line in a
    text file.

    C and C++ automatically translate these different formats into the Unix
    version as a text file is read, for the sake of uniformity (so two different
    versions of each program don't have to be written). This is true *unless*
    the file is opened in "binary mode". If binary mode is used, no translation
    is done. On Unix, binary mode has no effect, because no translation is
    performed anyway. On DOS/Windows, binary mode changes the behavior.

    A similar transformation is made on write, using the same rules -- all
    linefeeds are replaced by a carriage return/linefeed pair, if the platform
    is DOS/Windows and binary mode is not specified.
"

CONCLUSION: use binary mode, and windows will read&write unix format ;))

*/

	    // should work the same on win & unix, but, you never know.
	    sd.istrm = new ifstream( fname, ios_base::in | ios_base::binary );
#else
	    sd.istrm = new ifstream( fname );
#endif
	return sd;
    }

#ifdef __msvc__

    pErrMsg("Pipes are not supported on windows platform");

/*
    MSVC chokes on the following lines:
	sd.istrm = new ifstream( fileno(sd.fp) );
	sd.ostrm = new ofstream( fileno(sd.fp) );

    This means you can not open a fstream with just a file pointer. You would
    need a 'filedesc' for that. You can't make that from a file pointer, 
    however.
    Fortunately, this behavior is only required for more advanced useage, so
    we leave it out for now.

    A possible solution might be using named pipes. 
    See http://www.codeguru.com/console/dualmode.html

*/

#else

    if ( !hostname[0] )
	oscommand = (const char*)fname;
    else
    {
	switch ( type_ )
	{
	case StreamConn::Device:
	    if ( blocksize )
		sprintf( oscommand.buf(), "rsh %s dd if=%s ibs=%ld",
			    (const char*)hostname, (const char*)fname,
			    blocksize );
	    else
		sprintf( oscommand.buf(), "rsh %s dd if=%s",
			    (const char*)hostname, (const char*)fname );
	break;
	case StreamConn::Command:
	    sprintf( oscommand.buf(), "rsh %s %s", (const char*)hostname,
				    (const char*)fname );
	break;
	case StreamConn::File:
	    sprintf( oscommand.buf(), "rsh %s cat %s",
			    (const char*)hostname, (const char*)fname );
	break;
	}
    }

    sd.fp = popen( oscommand, "r" );
    sd.ispipe = true;
    if ( sd.fp )
	sd.istrm = new ifstream( fileno(sd.fp) );
#endif

    return sd;
}


StreamData StreamProvider::makeOStream() const
{
    StreamData sd;
    if ( isbad ||  !*(const char*)fname )
	return sd;
    else if ( fname == sStdIO )
    {
	sd.ostrm = &cout;
	return sd;
    }
    if ( type_ != StreamConn::Command && !hostname[0] )
    {
#ifdef __msvc__
        // should work the same on win & unix, but, you never know.
	sd.ostrm = new ofstream( fname, ios_base::out|ios_base::binary );
#else
	sd.ostrm = new ofstream( fname );
#endif
	return sd;
    }

#ifdef __msvc__

    pErrMsg("Pipes are not supported on windows platform");

#else

    if ( !hostname[0] )
	oscommand = (const char*)fname;
    else
    {
	switch ( type_ )
	{
	case StreamConn::Device:
	    if ( blocksize )
		sprintf( oscommand.buf(), "rsh %s dd of=%s obs=%ld",
				  (const char*)hostname, (const char*)fname,
				    blocksize );
	    else
		sprintf( oscommand.buf(), "rsh %s dd of=%s",
				(const char*)hostname, (const char*)fname );
	break;
	case StreamConn::Command:
	    sprintf( oscommand.buf(), "rsh %s %s", (const char*)hostname,
				    (const char*)fname );
	break;
	case StreamConn::File:
	    sprintf( oscommand.buf(), "rsh %s tee %s > /dev/null",
			      (const char*)hostname, (const char*)fname );
	break;
	}
    }

    sd.fp = popen( oscommand, "w" );
    sd.ispipe = true;
    if ( sd.fp )
	sd.ostrm = new ofstream( fileno(sd.fp) );
#endif

    return sd;
}


bool StreamProvider::exists( int fr ) const
{
    if ( isbad ) return false;

    if ( type_ == StreamConn::Command )
	return fr;

    if ( !hostname[0] )
	return fname == sStdIO ? true : File_exists( (const char*)fname );

    sprintf( oscommand.buf(), "rsh %s 'test -%c %s && echo 1'",
	      (const char*)hostname, fr ? 'r' : 'w', (const char*)fname );
    FILE* fp = popen( oscommand, "r" );
    int i = 0;
    fscanf( fp, "%d", &i );
    pclose( fp );
    return i;
}


bool StreamProvider::remove() const
{
    if ( isbad || type_ != StreamConn::File ) return false;

    if ( !hostname[0] )
	return fname == sStdIO ? false :
			File_remove( (const char*)fname, YES, YES );
    else
    {
	sprintf( oscommand.buf(), "rsh %s '/bin/rm -f %s && echo 1'",
		  (const char*)hostname, (const char*)fname );
	FILE* fp = popen( oscommand, "r" );
	char c;
	fscanf( fp, "%c", &c );
	pclose( fp );
	return c == '1';
    }
}
