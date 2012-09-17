/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituiseis.cc,v 1.4 2011/08/23 14:51:33 cvsbert Exp $";

#include "moddepmgr.h"
#include "uiveldesc.h"
#include "uit2dvelconvselgroup.h"

mDefModInitFn(uiSeis)
{
    mIfNotFirstTime( return );

    uiTime2Depth::initClass();
    uiDepth2Time::initClass();
    uiT2DVelConvSelGroup::initClass();
}
