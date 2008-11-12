#ifndef uisegydefdlg_h
#define uisegydefdlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uisegydefdlg.h,v 1.9 2008-11-12 14:28:19 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "seistype.h"
class uiSEGYFileSpec;
class uiSEGYFilePars;
class uiComboBox;
class uiCheckBox;
class uiGenInput;
class IOObj;


/*!\brief Initial dialog for SEG-Y I/O. */

class uiSEGYDefDlg : public uiDialog
{
public:

    struct Setup : public uiDialog::Setup
    {
					Setup();

	mDefSetupMemb(Seis::GeomType,	defgeom)
	TypeSet<Seis::GeomType>		geoms_;
					//!< empty=get uiSEGYRead default
    };

			uiSEGYDefDlg(uiParent*,const Setup&,IOPar&);
			~uiSEGYDefDlg();

    void		use(const IOObj*,bool force);
    void		usePar(const IOPar&);

    Seis::GeomType	geomType() const;
    int			nrTrcExamine() const;
    void		fillPar(IOPar&) const;

    Notifier<uiSEGYDefDlg> readParsReq;

protected:

    Setup		setup_;
    Seis::GeomType	geomtype_;
    IOPar&		pars_;

    uiSEGYFileSpec*	filespecfld_;
    uiSEGYFilePars*	fileparsfld_;
    uiGenInput*		nrtrcexfld_;
    uiComboBox*		geomfld_;
    uiCheckBox*		savenrtrcsbox_;

    void		initFlds(CallBacker*);
    void		readParsCB(CallBacker*);
    void		geomChg(CallBacker*);
    bool		acceptOK(CallBacker*);

    void		useSpecificPars(const IOPar&);

};


#endif
