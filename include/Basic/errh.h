#ifndef errh_H
#define errh_H

/*
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		19-10-1995
 Contents:	Error handler
 RCS:		$Id: errh.h,v 1.2 2000-06-23 14:09:11 bert Exp $
________________________________________________________________________

*/

#include <callback.h>
#include <bufstring.h>


class ErrMsgClass : public CallBacker
{
public:

			ErrMsgClass( const char* s, bool p )
			: msg(s), prog(p)	{}

    const char*		msg;
    bool		prog;

    static CallBack	TheCB;
    static bool		PrintProgrammerErrs;

};


inline void ErrMsg( const char* msg, bool progr = false )
{
    if ( !ErrMsgClass::TheCB.willCall() )
    {
	if ( !progr || ErrMsgClass::PrintProgrammerErrs )
	    cerr << (progr?"(PE) ":"") << msg << endl;
    }
    else
    {
	ErrMsgClass obj( msg, progr );
	ErrMsgClass::TheCB.doCall( &obj );
    }
}


inline void programmerErrMsg( const char* msg, const char* fname, int linenr )
{
    BufferString str( fname );
    str += "/";
    str += linenr;
    str += ": ";
    str += msg;

    ErrMsg( str, true );
}

#define pErrMsg(msg) programmerErrMsg( msg, __FILE__, __LINE__ )


#ifdef __prog__

CallBack ErrMsgClass::TheCB;

bool ErrMsgClass::PrintProgrammerErrs =
# ifdef __debug__
	true;
# else
	false;
# endif

#endif


#endif
