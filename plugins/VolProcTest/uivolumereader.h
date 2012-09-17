#ifndef uivolumereader_h
#define uivolumereader_h

/*+

_________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu 
 Date:		03-28-2007
 RCS:		$Id: uivolumereader.h,v 1.2 2009/07/22 16:01:27 cvsbert Exp $
_________________________________________________________________________

-*/

#include "uidialog.h"

class IOObjContext;
class uiIOObjSel;
class CtxtIOObj;

namespace VolProc
{

class VolumeReader;
class ProcessingStep;

class uiReader : public uiDialog
{

public:
    static void		initClass();
    			
			uiReader(uiParent*,VolumeReader*);
			~uiReader();
protected:
    static uiDialog*	create(uiParent*,ProcessingStep*);
    bool		acceptOK(CallBacker*);

    VolumeReader*	volreader_;

    uiIOObjSel*		uinputselfld_;
    CtxtIOObj*		iocontext_;
};


};
#endif
