/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Nov 2002
 * FUNCTION : Generate file to include in make.Vars
-*/

static const char* rcsID = "$Id$";

#include "prog.h"
#include "od_iostream.h"
#include "bufstringset.h"
#include "filepath.h"
#include "timefun.h"

class Dep
{
public:
			Dep( const char* m )
			    : name(m)	{}
    bool		operator ==( const char* s ) const
			    { return name == s; }

    BufferString	name;
    BufferStringSet	mods;
};


int main( int argc, char** argv )
{
    int arg1 = 1; bool html = false;
    if ( argc >= 3 && !strcmp(argv[1],"--html") )
	{ html = true; arg1 = 2; }
    if ( argc-arg1+1 < 3 )
    {
	od_cout() << "Usage: " << argv[0]
	     << " [--html] input_ModDeps_file output_file" << od_endl;
	ExitProgram( 1 );
    }

    od_istream instrm( argv[arg1] );
    if ( !instrm.isOK() )
    {
	od_cout() << argv[0] << ": Cannot open input stream" << od_endl;
	return ExitProgram( 1 );
    }

    od_istream outstrm( argv[arg1] );
    if ( !outstrm.isOK() )
    {
	od_cout() << argv[0] << ": Cannot open output stream" << od_endl;
	return ExitProgram( 1 );
    }

    BufferString line;
    char wordbuf[256];
    ObjectSet<Dep> deps;
    while ( instrm.isOK() )
    {
	instrm.getLine( line );
	char* bufptr = linebuf;
	mTrimBlanks(bufptr);
	if ( ! *bufptr || *bufptr == '#' )
	    continue;

	char* nextptr = (char*)getNextWord(line.buf(),wordbuf);
	if ( ! wordbuf[0] ) continue;
	od_int64 l = strLength( wordbuf );
	if ( wordbuf[l-1] == ':' ) wordbuf[l-1] = '\0';
	if ( ! wordbuf[0] ) continue;

	*nextptr++ = '\0';
	mSkipBlanks(nextptr);

	Dep* newdep = new Dep( wordbuf ) ;
	BufferStringSet filedeps;
	while ( nextptr && *nextptr )
	{
	    mSkipBlanks(nextptr);
	    nextptr = (char*)getNextWord(nextptr,wordbuf);
	    if ( !wordbuf[0] ) break;

	    if ( wordbuf[1] != '.' || (wordbuf[0] != 'S' && wordbuf[0] != 'D') )
		{ od_cout() << "Cannot handle dep=" << wordbuf << od_endl;
		    ExitProgram(1); }

	    filedeps.add( wordbuf );
	}


	BufferStringSet depmods;
	for ( int idx=filedeps.size()-1; idx>=0; idx-- )
	{
	    const char* filedep = filedeps.get(idx).buf();
	    const char* modnm = filedep + 2;
	    if ( *filedep == 'S' )
	    {
		depmods.add( modnm );
	        continue;
	    }

	    Dep* depdep = find( deps, modnm );
	    if ( !depdep )
		{ od_cout() << "Cannot find dep=" << modnm << od_endl;
				ExitProgram(1); }

	    for ( int idep=depdep->mods.size()-1; idep>=0; idep-- )
	    {
		const char* depdepmod = depdep->mods.get(idep).buf();
		if ( !depmods.isPresent(depdepmod) )
		    depmods.add( depdepmod );
	    }
	}
	if ( depmods.size() < 1 )
	    { delete newdep; continue; }
	deps += newdep;

	for ( int idx=depmods.size()-1; idx>=0; idx-- )
	    newdep->mods.add( depmods.get( idx ) );
    }
    instrm.close();

    if ( html )
    {
	outstrm << "<html>\n<!-- Generated by " << FilePath(argv[0]).fileName()
	        << " (" << Time::getDateTimeString()
	        << ") -->\n\n\n<body bgcolor=\"#dddddd\">"
		    "<title>OpendTect source documentation</title>\n\n"
		    "<center><h3>Open"
		<< GetProjectVersionName()
		<< " modules</h3></center>\n\n"
		   "<TABLE align=\"center\" summary=\"Modules\""
			   "WIDTH=\"75%\" BORDER=\"1\">"
		<< od_endl;

	BufferStringSet allmods;
	for ( int idx=0; idx<deps.size(); idx++ )
	    allmods.add( deps[idx]->name );
	allmods.sort();

	for ( int idep=0; idep<allmods.size(); idep++ )
	{
	    const BufferString& nm = allmods.get(idep);
	    outstrm << "<TR align=center>\n";

	    outstrm << " <TD><b>" << nm << "</b></TD>\n <TD><a href=\""
	    << nm << "/index.html\">Main Page</a></TD>\n <TD><a href=\""
	    << nm << "/classes.html\">Class Index</a></TD>\n <TD><a href=\""
	    << nm<<"/hierarchy.html\">Class Hierarchy</a></TD>\n <TD><a href=\""
	    << nm << "/annotated.html\">Compound List</a></TD>\n</TR>\n\n";
	}

	outstrm << "\n</TABLE>\n\n<body>\n<html>" << od_endl;
    }

    else
    {
	for ( int idx=0; idx<deps.size(); idx++ )
	{
	    Dep& dep = *deps[idx];

	    outstrm << 'L' << dep.name << " :=";
	    for ( int idep=0; idep<dep.mods.size(); idep++ )
		outstrm << " -l" << (const char*)(*dep.mods[idep]);
	    outstrm << od_endl;
	    outstrm << 'I' << dep.name << " :=";
	    for ( int idep=0; idep<dep.mods.size(); idep++ )
		outstrm << ' ' << (const char*)(*dep.mods[idep]);
	    outstrm << od_endl;

	}
    }


    sdout.close();
    return ExitProgram( 0 );
}
