#ifndef tutlogtools_h
#define tutlogtools_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : R.K. Singh
 * DATE     : June 2007
 * ID       : $Id: tutlogtools.h,v 1.1 2012/03/26 14:33:17 cvsdgb Exp $
-*/

#include "commondefs.h"

namespace Well { class Log; }

namespace Tut
{

mClass LogTools
{
public:

    			LogTools(const Well::Log& input,Well::Log& output);
			   		    

    bool		runSmooth(int gate);

protected:

    const Well::Log&	inplog_;
    Well::Log&		outplog_;

};

} // namespace

#endif
