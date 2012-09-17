#ifndef uiwellt2dconv_h
#define uiwellt2dconv_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2010
 RCS:           $Id: uiwellt2dconv.h,v 1.2 2010/07/08 06:07:30 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uit2dconvsel.h"


mClass uiT2DWellConvSelGroup : public uiT2DConvSelGroup
{
public:

    				uiT2DWellConvSelGroup(uiParent*);

    static void			initClass();
    static uiT2DConvSelGroup*	create( uiParent* p )
    				{ return new uiT2DWellConvSelGroup(p); }

    virtual bool		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

protected:

    uiIOObjSel*			fld_;

};

#endif
