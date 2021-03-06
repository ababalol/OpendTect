/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2001
________________________________________________________________________

-*/

#include "uisegyexamine.h"

#include "uimain.h"
#include "prog.h"
#include "moddepmgr.h"

#ifdef __win__
#include "file.h"
#endif


int main( int argc_in, char ** argv_in )
{
    OD::SetRunContext( OD::UiProgCtxt );
    SetProgramArgs( argc_in, argv_in );
    OD::ModDeps().ensureLoaded( "uiSeis" );

    char** argv = GetArgV();
    int& argc = GetArgC();
    bool dofork = true;
    uiSEGYExamine::Setup su;
    int argidx = 1;
    while ( argc > argidx
	 && *argv[argidx] == '-' && *(argv[argidx]+1) == '-' )
    {
	const FixedString arg( argv[argidx]+2 );
#define mArgIs(s) arg == s
	if ( mArgIs("ns") )
	    { argidx++; su.fp_.ns_ = toInt( argv[argidx] ); }
	else if ( mArgIs("fmt") )
	    { argidx++; su.fp_.fmt_ = toInt( argv[argidx] ); }
	else if ( mArgIs("nrtrcs") )
	    { argidx++; su.nrtrcs_ = toInt( argv[argidx] ); }
	else if ( mArgIs("filenrs") )
	    { argidx++; su.fs_.getMultiFromString( argv[argidx] ); }
	else if ( mArgIs("swapbytes") )
	    { argidx++; su.fp_.byteswap_ = toInt( argv[argidx] ); }
	else if ( mArgIs("fg") )
	    dofork = false;
	else
	    { od_cout() << "Ignoring option: " << argv[argidx] << od_endl; }

	argidx++;
    }

    if ( argc <= argidx )
    {
	od_cout() << "Usage: " << argv[0]
		  << "\n\t[--ns #samples]""\n\t[--nrtrcs #traces]"
		     "\n\t[--fmt segy_format_number]"
		     "\n\t[--filenrs start`stop`step[`nrzeropad]]"
		     "\n\t[--swapbytes 0_1_or_2]"
		     "\n\tfilename\n"
		  << "Note: filename must be with FULL path." << od_endl;
	return ExitProgram( 1 );
    }

#ifdef __debug__
    od_cout() << argv[0] << " started with args:";
    for ( int idx=1; idx<argc; idx++ )
	od_cout() << ' ' << argv[idx];
    od_cout() << od_endl;
#endif

    if ( dofork )
	ForkProcess();

    BufferString fnm( argv[argidx] );
    fnm.replace( "+x+", "*" );
    su.fs_.setFileName( fnm );

    uiMain app( argc, argv );

#ifdef __win__
    if ( File::isLink( su.fs_.fileName(0) ) )
	su.fs_.setFileName( File::linkEnd( su.fs_.fileName(0) ) );
#endif

    uiSEGYExamine* sgyex = new uiSEGYExamine( 0, su );
    app.setTopLevel( sgyex );
    sgyex->show();

    const int ret = app.exec();
    delete sgyex;
    return ExitProgram( ret );
}
