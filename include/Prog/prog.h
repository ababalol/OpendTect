#ifndef prog_h
#define prog_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		5-12-1995
 RCS:		$Id: prog.h,v 1.20 2012-05-01 07:57:40 cvsbert Exp $
________________________________________________________________________

 Include this file in any executable program you make. The file is actually
 pretty empty ....

-*/

#include "plugins.h"
#include "debug.h"

#ifdef __msvc__
# ifndef _CONSOLE
#  include "winmain.h"
# endif
#endif


#ifdef __cpp__
extern "C" {
#endif
    void		forkProcess();
    			/*!< Doesn't do anything on M$ Windows */
#ifdef __cpp__
}
#endif

#endif
