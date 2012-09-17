#ifndef uisegyimpdlg_h
#define uisegyimpdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uisegyimpdlg.h,v 1.11 2009/07/22 16:01:22 cvsbert Exp $
________________________________________________________________________

-*/

#include "uisegyreaddlg.h"
class uiSeisSel;
class CtxtIOObj;
class uiCheckBox;
class uiSeisTransfer;


/*!\brief Dialog to import SEG-Y files after basic setup. */

mClass uiSEGYImpDlg : public uiSEGYReadDlg
{
public :

			uiSEGYImpDlg(uiParent*,const Setup&,IOPar&);
			~uiSEGYImpDlg();

    void		use(const IOObj*,bool force);


protected:

    CtxtIOObj&		ctio_;

    uiSeisTransfer*	transffld_;
    uiSeisSel*		seissel_;
    uiCheckBox*		morebut_;

    virtual bool	doWork(const IOObj&);

    friend class	uiSEGYImpSimilarDlg;
    bool		impFile(const IOObj&,const IOObj&,
	    			const char*,const char*);

};


#endif
