/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/


#include "uibatchtime2depthsetup.h"

#include "ioobjctxt.h"
#include "dbman.h"
#include "keystrs.h"
#include "process_time2depth.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "zdomain.h"

#include "uibatchjobdispatchersel.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uipossubsel.h"
#include "uiveldesc.h"
#include "od_helpids.h"

uiBatchTime2DepthSetup::uiBatchTime2DepthSetup( uiParent* p )
    : uiDialog( p,Setup(tr("Time to depth volume conversion"),
			tr("Time/Depth conversion"),
                        mODHelpKey(mBatchTime2DepthSetupHelpID) ) )
{
    directionsel_ = new uiGenInput( this, uiStrings::sDirection(),
	    BoolInpSpec(true, tr("Time to Depth"), tr("Depth to Time"), true ));
    directionsel_->valuechanged.notify(
	    mCB(this,uiBatchTime2DepthSetup,dirChangeCB));

    t2dfld_ = new uiZAxisTransformSel( this, false, ZDomain::sKeyTime(),
				   ZDomain::sKeyDepth(), true );
    t2dfld_->attach( alignedBelow, directionsel_ );

    d2tfld_ = new uiZAxisTransformSel( this, false, ZDomain::sKeyDepth(),
				      ZDomain::sKeyTime(), true );
    d2tfld_->attach( alignedBelow, directionsel_ );

    IOObjContext inputtimectxt = SeisTrcTranslatorGroup::ioContext();
    IOObjContext inputdepthctxt = SeisTrcTranslatorGroup::ioContext();
    inputtimectxt.forread_ = inputdepthctxt.forread_ = true;
    if ( SI().zIsTime() )
    {
	inputdepthctxt.toselect_.require_.set( ZDomain::sKey(),
					      ZDomain::sKeyDepth() );
	inputtimectxt.toselect_.dontallow_.set( ZDomain::sKey(),
						ZDomain::sKeyDepth() );
    }
    else
    {
	inputtimectxt.toselect_.require_.set( ZDomain::sKey(),
					     ZDomain::sKeyTime() );
	inputdepthctxt.toselect_.dontallow_.set( ZDomain::sKey(),
						ZDomain::sKeyTime() );
    }

    const uiString depthvol = tr("Depth Volume");
    const uiString timevol = tr("Time Volume");

    uiSeisSel::Setup sssu(Seis::Vol);
    sssu.seltxt( uiStrings::phrInput( timevol ) );
    inputtimesel_ = new uiSeisSel( this, inputtimectxt, sssu );
    inputtimesel_->attach( alignedBelow, t2dfld_ );
    sssu.seltxt( uiStrings::phrInput( depthvol ) );
    inputdepthsel_ = new uiSeisSel( this, inputdepthctxt, sssu );
    inputdepthsel_->attach( alignedBelow, t2dfld_ );

    possubsel_ =  new uiPosSubSel( this, uiPosSubSel::Setup(false,false) );
    possubsel_->attach( alignedBelow, inputtimesel_ );

    IOObjContext outputtimectxt = inputtimectxt;
    outputtimectxt.forread_ = false;
    sssu.seltxt( uiStrings::phrOutput( timevol ) );
    outputtimesel_ = new uiSeisSel( this, outputtimectxt, sssu );
    outputtimesel_->selectionDone.notify(
			mCB(this,uiBatchTime2DepthSetup,objSelCB) );
    outputtimesel_->attach( alignedBelow, possubsel_ );

    IOObjContext outputdepthctxt = inputdepthctxt;
    outputdepthctxt.forread_ = false;
    sssu.seltxt( uiStrings::phrOutput( depthvol ) );
    outputdepthsel_ = new uiSeisSel( this, outputdepthctxt, sssu );
    outputdepthsel_->selectionDone.notify(
			mCB(this,uiBatchTime2DepthSetup,objSelCB) );
    outputdepthsel_->attach( alignedBelow, possubsel_ );

    batchfld_ = new uiBatchJobDispatcherSel( this, false,
					     Batch::JobSpec::T2D );
    batchfld_->attach( alignedBelow, outputdepthsel_ );

    dirChangeCB( 0 );
}


void uiBatchTime2DepthSetup::dirChangeCB( CallBacker* )
{
    const bool istime2depth = directionsel_->getBoolValue();
    t2dfld_->display( istime2depth );
    d2tfld_->display( !istime2depth );
    inputtimesel_->display( istime2depth );
    inputdepthsel_->display( !istime2depth );
    outputtimesel_->display( !istime2depth );
    outputdepthsel_->display( istime2depth );
}


void uiBatchTime2DepthSetup::objSelCB( CallBacker* )
{
}


bool uiBatchTime2DepthSetup::prepareProcessing()
{
    const bool istime2depth = directionsel_->getBoolValue();

    const IOObj* outioobj = 0;
    if ( istime2depth )
    {
	if ( !t2dfld_->acceptOK() )
	    return false;

	outioobj = outputdepthsel_->ioobj();
	if ( !inputtimesel_->ioobj() || !outioobj )
	    return false;
	ZDomain::Depth().set( outioobj->pars() );
    }
    else
    {
	if ( !d2tfld_->acceptOK() )
	    return false;

	outioobj = outputtimesel_->ioobj();
	if ( !inputdepthsel_->ioobj() || !outioobj )
	    return false;
	ZDomain::Time().set( outioobj->pars() );
    }

    DBM().setEntry( *outioobj );
    return true;
}


bool uiBatchTime2DepthSetup::fillPar()
{
    const bool istime2depth = directionsel_->getBoolValue();
    RefMan<ZAxisTransform> trans = istime2depth
	? t2dfld_->getSelection()
	: d2tfld_->getSelection();

    if ( !trans )
	return false;

    IOPar ztranspar;
    trans->fillPar( ztranspar );
    if ( !ztranspar.find( sKey::Name() ) )
    {
	pErrMsg("No name is set");
    }

    const IOObj* input = istime2depth
	? inputtimesel_->ioobj( true )
	: inputdepthsel_->ioobj( true );

    if ( !input )
	return false;

    const IOObj* output = istime2depth
	? outputdepthsel_->ioobj( true )
	: outputtimesel_->ioobj( true );

    if ( !output )
	return false;

    IOPar& par = batchfld_->jobSpec().pars_;
    par.set( ProcessTime2Depth::sKeyInputVolume(),  input->key() );
    possubsel_->fillPar( par );

    StepInterval<float> zrange;
    if ( istime2depth )
	t2dfld_->getTargetSampling( zrange );
    else
	d2tfld_->getTargetSampling( zrange );

    par.set( SurveyInfo::sKeyZRange(), zrange );
    par.set( ProcessTime2Depth::sKeyOutputVolume(), output->key() );
    par.mergeComp( ztranspar, ProcessTime2Depth::sKeyZTransPar() );
    par.setYN( ProcessTime2Depth::sKeyIsTimeToDepth(),
	       directionsel_->getBoolValue() );

    return true;
}


bool uiBatchTime2DepthSetup::acceptOK()
{
    if ( !prepareProcessing() || !fillPar() )
	return false;

    const bool istime2depth = directionsel_->getBoolValue();
    const IOObj* ioobj = istime2depth ? outputdepthsel_->ioobj( true )
				      : outputtimesel_->ioobj( true );
    if ( ioobj )
	batchfld_->setJobName( ioobj->name() );

    return batchfld_->start();
}
