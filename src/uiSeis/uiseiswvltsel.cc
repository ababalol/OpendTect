/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Dec 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseiswvltsel.cc,v 1.3 2010-12-06 12:29:29 cvsbert Exp $";

#include "uiseiswvltsel.h"
#include "uiseiswvltman.h"
#include "uicombobox.h"
#include "uitoolbutton.h"
#include "wavelet.h"
#include "iodirentry.h"
#include "ctxtioobj.h"
#include "survinfo.h"
#include "ioman.h"


uiSeisWaveletSel::uiSeisWaveletSel( uiParent* p, const char* seltxt )
    : uiGroup(p,"Wavelet selector")
    , newSelection(this)
{
    uiLabeledComboBox* lcb = new uiLabeledComboBox( this,
	    					seltxt ? seltxt : "Wavelet" );
    nmfld_ = lcb->box();

    uiToolButton* tb = new uiToolButton( this, "man_wvlt.png",
	    "Manage wavelets", mCB(this,uiSeisWaveletSel,startMan) );

    tb->attach( rightOf, lcb );
    setHAlignObj( lcb );

    fillBox();
}


uiSeisWaveletSel::~uiSeisWaveletSel()
{
    deepErase( ids_ );
}


void uiSeisWaveletSel::initFlds( CallBacker* )
{
    nmfld_->selectionChanged.notify( mCB(this,uiSeisWaveletSel,selChg) );
}


void uiSeisWaveletSel::setInput( const char* nm )
{
    if ( nm && *nm )
	nmfld_->setText( nm );
}


void uiSeisWaveletSel::setInput( const MultiID& mid )
{
    if ( mid.isEmpty() ) return;

    IOObj* ioobj = IOM().get( mid );
    if ( !ioobj ) return;

    nmfld_->setText( ioobj->name() );
    delete ioobj;
}


const char* uiSeisWaveletSel::getName() const
{
    return nmfld_->text();
}


const MultiID& uiSeisWaveletSel::getID() const
{
    static const MultiID emptyid;
    const int selidx = nmfld_->currentItem();
    return selidx < 0 ? emptyid : *ids_[selidx];
}


void uiSeisWaveletSel::startMan( CallBacker* )
{
    uiSeisWvltMan dlg( this );
    dlg.go();
    fillBox();
    selChg( nmfld_ );
}


void uiSeisWaveletSel::selChg( CallBacker* )
{
    newSelection.trigger();
}


void uiSeisWaveletSel::fillBox()
{
    IOObjContext ctxt( mIOObjContext(Wavelet) );
    IOM().to( ctxt.getSelKey() );
    IODirEntryList dil( IOM().dirPtr(), ctxt );
    nms_.erase(); deepErase( ids_ );
    for ( int idx=0; idx<dil.size(); idx ++ )
    {
	nms_.add( dil[idx]->ioobj->name() );
	ids_ += new MultiID( dil[idx]->ioobj->key() );
    }

    BufferString curwvlt( nmfld_->text() );
    nmfld_->selectionChanged.disable();
    nmfld_->setEmpty(); nmfld_->addItems( nms_ );
    nmfld_->selectionChanged.enable();

    int newidx = nms_.indexOf( curwvlt.buf() );
    if ( curwvlt.isEmpty() || newidx < 0 )
    {
	const char* res = SI().pars().find(
		IOPar::compKey(sKey::Default,ctxt.trgroup->userName()) );
	if ( res && *res )
	{
	    IOObj* ioobj = IOM().get( MultiID(res) );
	    if ( ioobj )
	    {
		curwvlt = ioobj->name();
		newidx = nms_.indexOf( curwvlt.buf() );
		delete ioobj;
	    }
	}
    }

    if ( newidx >= 0 )
	nmfld_->setCurrentItem( newidx );
}
