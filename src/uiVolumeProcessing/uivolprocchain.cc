/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: uivolprocchain.cc,v 1.18 2009-08-27 09:58:39 cvsbert Exp $";

#include "uivolprocchain.h"

#include "ctxtioobj.h"
#include "datainpspec.h"
#include "ioman.h"
#include "ioobj.h"
#include "settings.h"
#include "volprocchain.h"
#include "volproctrans.h"
#include "uibutton.h"
#include "uiioobjsel.h"
#include "uigeninput.h"
#include "uiicons.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uitoolbar.h"
#include "uiseparator.h"


namespace VolProc
{


const char* uiChain::sKeySettingKey()
{ return "dTect.ProcessVolumeBuilderOnOK"; }   

mImplFactory2Param( uiStepDialog, uiParent*, Step*, uiChain::factory );


uiStepDialog::uiStepDialog( uiParent* p, const char* stepnm, Step* step )
    : uiDialog( p, uiDialog::Setup("Edit step",stepnm,"103.6.1") )
    , step_(step)
{
}


void uiStepDialog::addNameFld( uiObject* alignobj )
{
    uiSeparator* sep = 0;
    if ( alignobj )
    {
	sep = new uiSeparator( this, "namefld sep" );
	sep->attach( stretchedBelow, alignobj );
    }
    namefld_ = new uiGenInput( this, "Name for this step", step_->userName() );
    if ( alignobj )
    {
	namefld_->attach( ensureBelow, sep );
	namefld_->attach( alignedBelow, alignobj );
    }
}


void uiStepDialog::addNameFld( uiGroup* aligngrp )
{
    uiObject* alignobj = aligngrp ? aligngrp->attachObj() : 0;
    addNameFld( alignobj );
}


bool uiStepDialog::acceptOK( CallBacker* )
{
    const BufferString nm( namefld_->text() );
    if ( nm.isEmpty() )
    {
	uiMSG().error( "Please enter a name for this step" );
	return false;
    }

    step_->setUserName( nm.buf() );

    return true;
}


const ioPixmap& uiChain::getPixmap()
{
    static ioPixmap res("volproc.png");
    return res;
}


uiChain::uiChain( uiParent* p, Chain& chn, bool withprocessnow )
    : uiDialog( p, uiDialog::Setup("Volume Builder: Setup",0,"103.6.0")
	    .menubar(true) )
    , chain_(chn)
{
    uiToolBar* tb = new uiToolBar( this, "Load/Save toolbar", uiToolBar::Right);
    tb->addButton( Icons::openObject(), mCB(this,uiChain,readPush),
	    	   "Read stored setup", false );
    tb->addButton( Icons::saveAs(), mCB(this,uiChain,savePush),
	    	   "Save setup now", false );

    uiGroup* flowgrp = new uiGroup( this, "Flow group" );

    uiLabel* availablelabel = new uiLabel( flowgrp, "Available steps" );
    factorylist_ = new uiListBox( flowgrp, PS().getNames(true) );
    factorylist_->selectionChanged.notify(
	    mCB(this,uiChain,factoryClickCB) );
    factorylist_->attach( ensureBelow, availablelabel );

    addstepbutton_ = new uiToolButton( flowgrp, "Add button",
					mCB(this,uiChain,addStepPush) );
    ((uiToolButton*)addstepbutton_)->setArrowType( uiToolButton::RightArrow );
    addstepbutton_->setToolTip( "Add step" );
    addstepbutton_->attach( centeredRightOf, factorylist_ );

    steplist_ = new uiListBox( flowgrp );
    steplist_->attach( rightTo, factorylist_ );
    steplist_->attach( ensureRightOf, addstepbutton_ );
    steplist_->selectionChanged.notify(
	    mCB(this,uiChain,stepClickCB) );
    steplist_->doubleClicked.notify(
	    mCB(this,uiChain,stepDoubleClickCB) );

    uiLabel* label = new uiLabel( flowgrp, "Used steps" );
    label->attach( alignedAbove, steplist_ );
    label->attach( rightTo, availablelabel );

    moveupbutton_ = new uiToolButton( flowgrp, "Up button",
					mCB(this,uiChain,moveUpCB) );
    ((uiToolButton*)moveupbutton_)->setArrowType( uiToolButton::UpArrow );
    moveupbutton_->setToolTip( "Move step up" );
    moveupbutton_->attach( rightOf, steplist_ );

    movedownbutton_ = new uiToolButton( flowgrp, "Up button",
					mCB(this,uiChain,moveDownCB) );
    ((uiToolButton*)movedownbutton_)->setArrowType( uiToolButton::DownArrow );
    movedownbutton_->setToolTip( "Move step down" );
    movedownbutton_->attach( alignedBelow, moveupbutton_ );

    propertiesbutton_ = new uiPushButton( flowgrp, "S&ettings",
			    mCB(this,uiChain,propertiesCB), false );
    propertiesbutton_->attach( alignedBelow, movedownbutton_ );

    removestepbutton_ = new uiToolButton( flowgrp, "Remove",
	    				  Icons::removeObject(), 
					  mCB(this,uiChain,removeStepPush) );
    movedownbutton_->setToolTip( "Remove step from flow" );
    removestepbutton_->attach( alignedBelow, propertiesbutton_ );

    flowgrp->setHAlignObj( steplist_ );

    IOObjContext ctxt = VolProcessingTranslatorGroup::ioContext();
    ctxt.forread = false;

    objfld_ = new uiIOObjSel( this, ctxt, "On OK, store As" );
    objfld_->setInput( chain_.storageID() );
    objfld_->setConfirmOverwrite( false );
    objfld_->attach( alignedBelow, flowgrp );

    if ( withprocessnow )
    {
	enableSaveButton( "Process on OK" );
	bool enabled = false;
	Settings::common().getYN( sKeySettingKey(), enabled );
	setSaveButtonChecked( enabled );
    }

    updateList();
    updateButtons();
}


uiChain::~uiChain()
{
}


void uiChain::updObj( const IOObj& ioobj )
{
    chain_.setStorageID( ioobj.key() );
    objfld_->setInput( ioobj.key() );
}


const MultiID& uiChain::storageID() const
{
    return chain_.storageID();
}


bool uiChain::acceptOK(CallBacker*)
{
    const IOObj* ioobj = objfld_->ioobj( true );
    if ( !ioobj )
    {
	uiMSG().error("Please enter a name for the setup");
	return false;
    }

    chain_.setStorageID( ioobj->key() );
    if ( hasSaveButton() )
    {
	Settings::common().setYN( sKeySettingKey(), saveButtonChecked() );
	Settings::common().write();
    }

    return doSave();
}


bool uiChain::doSave()
{
    const IOObj* ioobj = objfld_->ioobj( true );
    if ( !ioobj )
	return doSaveAs();

    BufferString errmsg;
    if ( VolProcessingTranslator::store( chain_, ioobj, errmsg ) )
	return true;

     uiMSG().error( errmsg );
     return false;
}


bool uiChain::doSaveAs()
{
    IOObjContext ctxt = VolProcessingTranslatorGroup::ioContext();
    uiIOObjSelDlg dlg( this, ctxt, "Volume Builder Setup" );
    if ( !dlg.go() || !dlg.nrSel() )
	 return false;

     BufferString errmsg;
     if ( VolProcessingTranslator::store( chain_, dlg.ioObj(), errmsg ) )
     {
	 updObj( *dlg.ioObj() );
	 return true;
     }

     uiMSG().error( errmsg );
     return false;
}


void uiChain::updateList()
{
    NotifyStopper stopper( steplist_->selectionChanged );
    int idx=0;
    for ( ; idx<chain_.nrSteps(); idx ++ )
    {
	const char* key = chain_.getStep(idx)->type();
	const char* username = chain_.getStep(idx)->userName();
	const int keyidx = PS().getNames(false).indexOf( key );
	const char* displayname = username
	    ? username
	    : PS().getNames(true)[keyidx]->buf();

	if ( idx>=steplist_->size() )
	    steplist_->addItem( displayname, false);
	else
	    steplist_->setItemText( idx, displayname );
    }

    for ( ; idx<steplist_->size(); )
	steplist_->removeItem( idx );
}


void uiChain::updateButtons()
{
    const bool factoryselected = factorylist_->nextSelected(-1)!=-1;
    const int stepsel = steplist_->nextSelected(-1);

    addstepbutton_->setSensitive( factoryselected );
    removestepbutton_->setSensitive( stepsel!=-1 );

    moveupbutton_->setSensitive( stepsel>0 );
    movedownbutton_->setSensitive( stepsel!=-1 &&
	    stepsel!=steplist_->size()-1 );

    propertiesbutton_->setSensitive( stepsel!=-1 );
}


bool uiChain::showPropDialog( int idx )
{
    Step* step = chain_.getStep( idx );
    if ( !step ) return false;

    PtrMan<uiStepDialog> dlg = factory().create( step->type(), this, step );
    if ( !dlg )
    {
	dlg = new uiStepDialog( this, "Name", step );
	uiObject* uio = 0; dlg->addNameFld( uio );
    }

    bool ret = dlg->go();
    if ( ret )
	updateList();

    return ret;
}


void uiChain::readPush( CallBacker* )
{
    IOObjContext ctxt = VolProcessingTranslatorGroup::ioContext();
    ctxt.forread = true;
    uiIOObjSelDlg dlg( this, ctxt );
     dlg.selGrp()->setConfirmOverwrite( false );
     if ( !dlg.go() || !dlg.nrSel() )
	 return;

     BufferString errmsg;
     if ( VolProcessingTranslator::retrieve( chain_, dlg.ioObj(), errmsg ) )
     {
	 updObj( *dlg.ioObj() );
	 updateList();
	 return;
     }

     updateList();
     uiMSG().error(errmsg);
}


void uiChain::savePush(CallBacker* cb)
{
    doSaveAs();
}


void uiChain::factoryClickCB(CallBacker*)
{
    if ( factorylist_->nextSelected(-1) == -1 )
	return;

    steplist_->selectAll(false);
    updateButtons();
}


void uiChain::stepClickCB(CallBacker*)
{
    if ( steplist_->nextSelected(-1) == -1 )
	return;

    factorylist_->selectAll(false);
    updateButtons();
}


void uiChain::stepDoubleClickCB(CallBacker*)
{
    factorylist_->selectAll(false);
    updateButtons();

    showPropDialog( steplist_->currentItem() );
}


void uiChain::addStepPush(CallBacker*)
{
    const int sel = factorylist_->nextSelected(-1);
    if ( sel == -1 )
	return;

    const char* steptype = PS().getNames(false)[sel]->buf();
    Step* step = PS().create( steptype, chain_ );
    if ( !step ) return;

    chain_.addStep( step );
    updateList();
    steplist_->selectAll( false );
    int curitm = steplist_->size() - 1;
    steplist_->setCurrentItem( curitm );
    if ( !showPropDialog(curitm) )
    {
	chain_.removeStep( curitm );
	updateList();
	curitm--;
	if ( curitm >= 0 )
	    steplist_->setCurrentItem( curitm );
    }
    updateButtons();
}


void uiChain::removeStepPush(CallBacker*)
{
    int curitm = steplist_->nextSelected(-1);
    if ( curitm < 0 )
	return;

    chain_.removeStep( curitm );
    updateList();
    steplist_->selectAll(false);
    if ( curitm >= steplist_->size() )
	curitm--;
    if ( curitm >= 0 )
	steplist_->setCurrentItem( curitm );

    updateButtons();
}


void uiChain::moveUpCB(CallBacker*)
{
    const int idx = steplist_->nextSelected(-1);

    if ( idx<1 ) return;

    chain_.swapSteps( idx, idx-1 );
    updateList();

    steplist_->setSelected( idx-1, true );
    updateButtons();
}


void uiChain::moveDownCB(CallBacker*)
{
    const int idx = steplist_->nextSelected(-1);
    if ( idx<0 || idx>=chain_.nrSteps() )
	return;

    chain_.swapSteps( idx, idx+1 );
    updateList();
    steplist_->setSelected( idx+1, true );
    updateButtons();
}


void uiChain::propertiesCB(CallBacker*)
{
    const int idx = steplist_->nextSelected(-1);
    if ( idx<0 ) return;

    showPropDialog( idx );
}

}; //namespace
