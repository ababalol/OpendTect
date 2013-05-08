/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiprestackanglemute.h"

#include "prestackanglecomputer.h"
#include "prestackanglemute.h"
#include "raytrace1d.h"
#include "survinfo.h"
#include "windowfunction.h"
#include "uibutton.h"
#include "uiioobjsel.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiprestackprocessor.h"
#include "uiraytrace1d.h"
#include "uiseparator.h"
#include "uiveldesc.h"


namespace PreStack
{

uiAngleCompGrp::uiAngleCompGrp( uiParent* p, PreStack::AngleCompParams& pars, 
				bool dooffset, bool isformute )
    : uiGroup(p,"Angle Mute Group")
    , params_(pars)
    , isformute_(isformute)
    , anglelbl_(0)
{
    IOObjContext ctxt = uiVelSel::ioContext(); 
    velfuncsel_ = new uiVelSel( this, ctxt, uiSeisSel::Setup(Seis::Vol) );
    velfuncsel_->setLabelText( "Velocity input volume" );
    if ( !params_.velvolmid_.isUdf() )
       velfuncsel_->setInput( params_.velvolmid_ ); 

    if ( isformute_ )
    {
	anglefld_ = new uiGenInput( this, "Mute cutoff angle (degree)",
				     FloatInpSpec(false) );
	anglefld_->attach( alignedBelow, velfuncsel_ );
	anglefld_->setValue( params_.mutecutoff_ );
    }
    else
    {
	anglefld_ = new uiGenInput( this, "Angle range",
				    FloatInpIntervalSpec(params_.anglerange_) );
	anglefld_->attach( alignedBelow, velfuncsel_ );
	anglelbl_ = new uiLabel( this, "degrees" );
	anglelbl_->attach( rightOf, anglefld_ );
    }

    advpushbut_ = new uiPushButton( this, "Advanced Parameters", true );
    advpushbut_->activated.notify( mCB(this, uiAngleCompGrp, advPushButCB) );
    advpushbut_->attach( alignedBelow, anglefld_ );

    advpardlg_ = new uiAngleCompAdvParsDlg(this, params_, dooffset, isformute);
    setHAlignObj( velfuncsel_ );
}


void uiAngleCompGrp::updateFromParams()
{
    velfuncsel_->setInput( params_.velvolmid_ );
    if ( isformute_ )
	anglefld_->setValue( params_.mutecutoff_ );
    else
	anglefld_->setValue( params_.anglerange_ );
}


bool uiAngleCompGrp::acceptOK()
{
    if ( !velfuncsel_->ioobj() )
	return false;

    params_.velvolmid_ = velfuncsel_->key(true);
    Interval<float> normalanglevalrange( 0, 90 );
    if ( isformute_ )
    {
	params_.mutecutoff_ = anglefld_->getfValue();
	if ( !normalanglevalrange.includes(params_.mutecutoff_,false) )
	{
	    uiMSG().error( 
		    "Please select the mute cutoff between 0 and 90 degree" );
	    return false;
	}
    }
    else
    {
	params_.anglerange_ = anglefld_->getFInterval();
	if ( !normalanglevalrange.includes(params_.anglerange_,false) )
	{
	    uiMSG().error("Please provide angle range between 0 and 90 degree");
	    return false;
	}
    }

    return true;
}


void uiAngleCompGrp::advPushButCB( CallBacker* )
{
    advpardlg_->updateFromParams();
    advpardlg_->go(); 
}


uiAngleCompAdvParsDlg::uiAngleCompAdvParsDlg( uiParent* p, 
					      PreStack::AngleCompParams& pars,
					      bool offset, bool isformute )
    : uiDialog(p, uiDialog::Setup("Advanced Parameter",
				  "Advanced angle parametrs", mNoHelpID))
    , params_(pars)
    , isformute_(isformute)
    , smoothtypefld_(0)
    , smoothwindowfld_(0)
    , smoothwinparamfld_(0)
    , smoothwinlengthfld_(0)
    , freqf3fld_(0)
    , freqf4fld_(0)
{
    uiRayTracer1D::Setup rsu; 
    rsu.dooffsets_ = offset; 
    rsu.doreflectivity_ = false;
    raytracerfld_ = new uiRayTracerSel( this, rsu );

    if ( isformute_ )
	return;

    smoothtypefld_ = new uiGenInput( this, "Smoothing Type",
	StringListInpSpec(PreStack::AngleComputer::smoothingTypeNames()) );
    smoothtypefld_->attach( alignedBelow, raytracerfld_ );
    smoothtypefld_->valuechanged.notify( mCB(this,uiAngleCompAdvParsDlg,
					     smoothTypeSel) );

    const BufferStringSet& windowfunctions = WINFUNCS().getNames();
    smoothwindowfld_ = new uiGenInput( this, "Smoothing Window", 
				       StringListInpSpec(windowfunctions) );
    smoothwindowfld_->attach( alignedBelow, smoothtypefld_ );

    smoothwinparamfld_ = new uiGenInput( this, "Smoothing Param",
					 FloatInpSpec() );
    smoothwinparamfld_->attach( alignedBelow, smoothwindowfld_ );

    smoothwinlengthfld_ = new uiGenInput( this, "Window width (ms)/(m)/(ft)", 
					FloatInpSpec() );
    smoothwinlengthfld_->attach( alignedBelow, smoothwinparamfld_ );

    freqf3fld_ = new uiGenInput( this, "Freq F3", FloatInpSpec() );
    freqf3fld_->attach( alignedBelow, smoothtypefld_ );

    freqf4fld_ = new uiGenInput( this, "Freq F4", FloatInpSpec() );
    freqf4fld_->attach( alignedBelow, freqf3fld_ );

    postFinalise().notify( mCB(this,uiAngleCompAdvParsDlg,smoothTypeSel) );
}


bool uiAngleCompAdvParsDlg::acceptOK( CallBacker* )
{
    raytracerfld_->fillPar( params_.raypar_ );
    if ( isformute_ )
	return true;

    IOPar& iopar = params_.smoothingpar_;
    const bool istimeavg = smoothtypefld_->getIntValue() == 0;
    iopar.set( PreStack::AngleComputer::sKeySmoothType(),
	       smoothtypefld_->getIntValue() );
    if ( istimeavg )
    {
	iopar.set( PreStack::AngleComputer::sKeyWinFunc(),
		   smoothwindowfld_->text() );
	iopar.set( PreStack::AngleComputer::sKeyWinParam(), 
		   smoothwinparamfld_->getfValue() );
	iopar.set( PreStack::AngleComputer::sKeyWinLen(), 
		   smoothwinlengthfld_->getfValue() );
    }
    else
    {
	iopar.set( PreStack::AngleComputer::sKeyFreqF3(), 
		   freqf3fld_->getfValue() );
	iopar.set( PreStack::AngleComputer::sKeyFreqF4(),
		   freqf4fld_->getfValue() );
    }

    return true;
}

void uiAngleCompAdvParsDlg::updateFromParams()
{
    raytracerfld_->usePar( params_.raypar_ );
    if ( isformute_ )
	return;

    const IOPar& iopar = params_.smoothingpar_;
    int smoothtype = 0;
    iopar.get( PreStack::AngleComputer::sKeySmoothType(), smoothtype );
    smoothtypefld_->setValue( smoothtype );
    BufferString windowname;
    iopar.get( PreStack::AngleComputer::sKeyWinFunc(), windowname );
    smoothwindowfld_->setText( windowname );
    float windowparam;
    iopar.get( PreStack::AngleComputer::sKeyWinParam(), windowparam );
    smoothwinparamfld_->setValue( windowparam );
    float windowlength;
    iopar.get( PreStack::AngleComputer::sKeyWinLen(), windowlength );
    smoothwinlengthfld_->setValue( windowlength );
    
    float freqf3;
    iopar.get( PreStack::AngleComputer::sKeyFreqF3(), freqf3 );
    freqf3fld_->setValue( freqf3 );
    float freqf4;
    iopar.get( PreStack::AngleComputer::sKeyFreqF4(), freqf4 );
    freqf4fld_->setValue( freqf4 );

    smoothTypeSel(0);
}


void uiAngleCompAdvParsDlg::smoothTypeSel( CallBacker* )
{
    const bool istimeavg = smoothtypefld_->getIntValue() == 0;
    smoothwindowfld_->display( istimeavg );
    smoothwinparamfld_->display( istimeavg );
    smoothwinlengthfld_->display( istimeavg );
    freqf3fld_->display( !istimeavg );
    freqf4fld_->display( !istimeavg );
}


void uiAngleMute::initClass()
{
    uiPSPD().addCreator( create, AngleMute::sFactoryKeyword() );
}


uiDialog* uiAngleMute::create( uiParent* p, Processor* sgp )
{
    mDynamicCastGet( AngleMute*, sgmute, sgp );
    if ( !sgmute ) return 0;

    return new uiAngleMute( p, sgmute );
}


uiAngleMute::uiAngleMute( uiParent* p, AngleMute* rt )
    : uiDialog( p, uiDialog::Setup("AngleMute setup",mNoDlgTitle,"103.2.20") )
    , processor_( rt )		      
{
    anglecompgrp_ = new uiAngleCompGrp( this, processor_->params() );

    uiSeparator* sep = new uiSeparator( this, "Sep" );
    sep->attach( stretchedBelow, anglecompgrp_ );

    topfld_ = new uiGenInput( this, "Mute type",
	    BoolInpSpec(!processor_->params().tail_,"Outer","Inner") );
    topfld_->attach( ensureBelow, sep );
    topfld_->attach( centeredBelow, anglecompgrp_ );

    taperlenfld_ = new uiGenInput( this, "Taper length (samples)",
	    FloatInpSpec(processor_->params().taperlen_) );
    taperlenfld_->attach( alignedBelow, topfld_ );
}


bool uiAngleMute::acceptOK(CallBacker*)
{
    if ( !anglecompgrp_->acceptOK() )
	return false;

    processor_->params().taperlen_ = taperlenfld_->getfValue();
    processor_->params().tail_ = !topfld_->getBoolValue();

    return true;
}


}; //namespace
