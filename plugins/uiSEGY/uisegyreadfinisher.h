#ifndef uisegyreadfinisher_h
#define uisegyreadfinisher_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2015
 RCS:           $Id: $
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uidialog.h"


/*!\brief Finishes reading process of 'any SEG-Y file'. */

mExpClass(uiSEGY) uiSEGYReadFinisher : public uiDialog
{ mODTextTranslationClass(uiSEGYReadFinisher);
public:

			uiSEGYReadFinisher(uiParent*,const FullSpec&,
					   const char* usrspec);
			~uiSEGYReadFinisher();

    FullSpec&		fullSpec()		{ return fs_; }

protected:

    FullSpec		fs_;

    void		initWin(CallBacker*);
    bool		acceptOK(CallBacker*);

    static uiString	getWinTile(const FullSpec&);
    static uiString	getDlgTitle(const char*);

};


#endif
