/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2006
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uid2tmodelgrp.h"
#include "uitblimpexpdatasel.h"

#include "uifileinput.h"
#include "uigeninput.h"
#include "uimsg.h"

#include "ctxtioobj.h"
#include "hiddenparam.h"
#include "strmprov.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "welld2tmodel.h"
#include "wellimpasc.h"
#include "welldata.h"
#include "welltrack.h"

#define mZUnLbl SI().depthsInFeet() ? " (ft/s)" : " (m/s)"


HiddenParam< uiD2TModelGroup,BufferString* > errmsg_(0);
HiddenParam< uiD2TModelGroup,BufferString* > warnmsg_(0);

uiD2TModelGroup::uiD2TModelGroup( uiParent* p, const Setup& su )
    : uiGroup(p,"D2TModel group")
    , velfld_(0)
    , csfld_(0)
    , dataselfld_(0)
    , setup_(su)
    , fd_( *Well::D2TModelAscIO::getDesc(setup_.withunitfld_) )
{
    errmsg_.setParam( this, new BufferString() );
    warnmsg_.setParam( this, new BufferString() );

    filefld_ = new uiFileInput( this, setup_.filefldlbl_,
				uiFileInput::Setup().withexamine(true) );
    if ( setup_.fileoptional_ )
    {
	const BufferString velllbl( "Temporary model velocity", mZUnLbl );
	const float vel = getGUIDefaultVelocity();
	filefld_->setWithCheck( true ); filefld_->setChecked( true );
	filefld_->checked.notify( mCB(this,uiD2TModelGroup,fileFldChecked) );
	velfld_ = new uiGenInput( this, velllbl, FloatInpSpec(vel) );
	velfld_->attach( alignedBelow, filefld_ );
    }

    dataselfld_ = new uiTableImpDataSel( this, fd_, "107.0.3" );
    dataselfld_->attach( alignedBelow, setup_.fileoptional_ ? velfld_
							    : filefld_ );

    if ( setup_.asksetcsmdl_ )
    {
	csfld_ = new uiGenInput( this, "Is this checkshot data?",
				 BoolInpSpec(false) );
	csfld_->attach( alignedBelow, dataselfld_ );
    }

    setHAlignObj( filefld_ );
    postFinalise().notify( mCB(this,uiD2TModelGroup,fileFldChecked) );
}


uiD2TModelGroup::~uiD2TModelGroup()
{
    BufferString* errmsg = errmsg_.getParam(this);
    BufferString* warnmsg = warnmsg_.getParam(this);
    errmsg_.removeParam( this );
    warnmsg_.removeParam( this );
    delete errmsg;
    delete warnmsg;
}


const char* uiD2TModelGroup::errMsg() const
{ return errmsg_.getParam(this)->str(); }


const char* uiD2TModelGroup::warnMsg() const
{ return warnmsg_.getParam(this)->str(); }


void uiD2TModelGroup::setMsg( const BufferString msg, bool warning ) const
{
    const_cast<uiD2TModelGroup*>(this)->setMsgNonconst(msg,warning);
}


void uiD2TModelGroup::setMsgNonconst( const BufferString msg, bool warning )
{
    BufferString* retmsg =  warning ? warnmsg_.getParam(this)
				    : errmsg_.getParam(this);
    if ( retmsg ) *retmsg = msg;
}


void uiD2TModelGroup::fileFldChecked( CallBacker* )
{
    const bool havefile = setup_.fileoptional_ ? filefld_->isChecked() : true;
    if ( csfld_ ) csfld_->display( havefile );
    if ( velfld_ ) velfld_->display( !havefile );
    dataselfld_->display( havefile );
    dataselfld_->updateSummary();
}


const char* uiD2TModelGroup::getD2T( Well::Data& wd, bool cksh ) const
{
    getD2TBool(wd,cksh);
    return errMsg();
}

#define mErrRet(s) \
{ \
    setMsg( s, false ); \
    return false; \
}
bool uiD2TModelGroup::getD2TBool( Well::Data& wd, bool cksh ) const
{
    if ( setup_.fileoptional_ && !filefld_->isChecked() )
    {
	if ( velfld_->isUndef() )
	    mErrRet( "Please enter the velocity for generating the D2T model" )
    }

    if ( wd.track().isEmpty() )
	mErrRet( "Cannot generate D2Time model without track" )

    if ( cksh )
	wd.setCheckShotModel( new Well::D2TModel );
    else
	wd.setD2TModel( new Well::D2TModel );

    Well::D2TModel& d2t = *(cksh ? wd.checkShotModel() : wd.d2TModel());
    if ( !&d2t )
	mErrRet( "D2Time model not set properly" )

    if ( filefld_->isCheckable() && !filefld_->isChecked() )
    {
	float vel = velfld_->getfValue();
	const UnitOfMeasure* zun = UnitOfMeasure::surveyDefDepthUnit();
	if ( SI().zIsTime() && SI().depthsInFeet() && zun )
	    vel = zun->internalValue( vel );

	d2t.makeFromTrack( wd.track(), vel, wd.info().replvel );
    }
    else
    {
	const char* fname = filefld_->fileName();
	StreamData sdi = StreamProvider( fname ).makeIStream();
	if ( !sdi.usable() )
	{
	    sdi.close();
	    mErrRet( "Could not open input file" )
	}

	if ( !dataselfld_->commit() )
	    mErrRet( "Please specify data format" )

	d2t.setName( fname );
	Well::D2TModelAscIO aio( fd_ );
	if ( !aio.get(*sdi.istrm,d2t,wd) )
	{
	    BufferString errmsg;
	    errmsg = "Ascii TD model import failed for well ";
	    errmsg.add( wd.name() ).add( "\n" );
	    if ( aio.errMsg() )
		errmsg.add( aio.errMsg() ).add( "\n" );

	    errmsg.add( "Change your format definition "
			"or edit your data or press cancel." );
	    mErrRet(errmsg)
	}

	if ( aio.warnMsg() )
	    setMsg( aio.warnMsg(), true );

	aio.deleteMsg();
    }

    if ( d2t.size() < 2 )
	mErrRet( "Cannot import time-depth model" )

    d2t.deInterpolate();
    return true;
}


bool uiD2TModelGroup::wantAsCSModel() const
{
    return csfld_ && csfld_->getBoolValue() && filefld_->isChecked();
}


BufferString uiD2TModelGroup::dataSourceName() const
{
    BufferString ret;
    if ( !filefld_->isCheckable() || filefld_->isChecked() )
	ret.set( filefld_->fileName() );
    else
	ret.set( "[V=" ).add( velfld_->getfValue() ).add( mZUnLbl ).add( "]" );

    return ret;
}


float getGUIDefaultVelocity()
{
    return SI().depthsInFeet() ? 8000.f : 2000.f;
}

