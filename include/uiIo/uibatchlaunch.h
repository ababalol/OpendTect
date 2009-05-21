#ifndef uibatchlaunch_h
#define uibatchlaunch_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          Jan 2002
 RCS:           $Id: uibatchlaunch.h,v 1.24 2009-05-21 07:12:37 cvsraman Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "bufstringset.h"

#ifndef __cygwin__
# define HAVE_OUTPUT_OPTIONS
#endif

class InlineSplitJobDescProv;
class IOPar;
class uiGenInput;
class uiFileInput;
class uiPushButton;
class uiLabel;
class uiLabeledComboBox;
class uiLabeledSpinBox;

#ifdef HAVE_OUTPUT_OPTIONS
mClass uiBatchLaunch : public uiDialog
{
public:
			uiBatchLaunch(uiParent*,const IOPar&,
				      const char* hostnm,const char* prognm,
				      bool with_print_pars=false);
			~uiBatchLaunch();

    void		setParFileName(const char*);

protected:

    BufferStringSet	opts_;
    IOPar&		iop_;
    BufferString	hostname_;
    BufferString	progname_;
    BufferString	parfname_;
    BufferString	rshcomm_;
    int			nicelvl_;

    uiFileInput*	filefld_;
    uiLabeledComboBox*	optfld_;
    uiGenInput*		remfld_;
    uiGenInput*		remhostfld_;;
    uiLabeledSpinBox*	nicefld_;

    bool		acceptOK(CallBacker*);
    bool		execRemote() const;
    void		optSel(CallBacker*);
    void		remSel(CallBacker*);
    int			selected();

};
#endif

mClass uiFullBatchDialog : public uiDialog
{
protected:

    mClass Setup
    {
    public:
			Setup(const char* txt)
			    : wintxt_(txt)
			    , procprognm_("")
			    , multiprocprognm_("")
			    , modal_(true)
			    , menubar_(false)
			{}

	mDefSetupMemb(BufferString,wintxt)
	mDefSetupMemb(BufferString,procprognm)
	mDefSetupMemb(BufferString,multiprocprognm)
	mDefSetupMemb(bool,modal)
	mDefSetupMemb(bool,menubar)
    };

    			uiFullBatchDialog(uiParent*,const Setup&);

    const BufferString	procprognm_;
    const BufferString	multiprognm_;
    BufferString	singparfname_;
    BufferString	multiparfname_;
    uiGroup*		uppgrp_;

    virtual bool	prepareProcessing()	= 0;
    virtual bool	fillPar(IOPar&)		= 0;
    void		addStdFields(bool forread=false,
	    			     bool onlysinglemachine=false,
				     bool clusterproc=true);
    			//!< Needs to be called at end of constructor
    void		setParFileNmDef(const char*);

    void		doButPush(CallBacker*);

    void		singTogg(CallBacker*);

    bool		singLaunch(const IOPar&,const char*);
    bool		multiLaunch(const char*);
    bool		clusterLaunch(const char*);
    bool		acceptOK(CallBacker*);

    uiGenInput*		singmachfld_;
    uiFileInput*	parfnamefld_;

    bool		redo_; //!< set to true only for re-start
    bool		hascluster_;

};


mClass uiRestartBatchDialog : public uiFullBatchDialog
{
public:

    			uiRestartBatchDialog(uiParent*,const char* ppn=0,
					     const char* mpn=0);

protected:

    virtual bool	prepareProcessing()	{ return true; }
    virtual bool	fillPar(IOPar&)		{ return true; }
    virtual bool	parBaseName() const	{ return 0; }

};


#endif
