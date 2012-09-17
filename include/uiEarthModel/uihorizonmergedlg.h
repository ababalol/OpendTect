#ifndef uihorizonmergedlg_h
#define uihorizonmergedlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2011
 RCS:		$Id: uihorizonmergedlg.h,v 1.1 2011/05/16 12:04:15 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiGenInput;
class uiHorizon3DSel;
class uiSurfaceWrite;

mClass uiHorizonMergeDlg : public uiDialog
{
public:
			uiHorizonMergeDlg(uiParent*,bool is2d);
			~uiHorizonMergeDlg();

protected:

    bool		acceptOK(CallBacker*);

    uiHorizon3DSel*	horselfld_;
    uiGenInput*		duplicatefld_;
    uiSurfaceWrite*	outfld_;
};

#endif

