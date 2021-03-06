/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2012
________________________________________________________________________

-*/

#include "uistratlaymodtools.h"

#include "keystrs.h"
#include "propertyref.h"
#include "stratlayermodel.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uispinbox.h"
#include "uitoolbutton.h"
#include "od_helpids.h"


const char* uiStratGenDescTools::sKeyNrModels()
{ return "Nr models"; }

const char* uiStratLayModEditTools::sKeyDisplayedProp()
{ return "Displayed property"; }

const char* uiStratLayModEditTools::sKeyDecimation()
{ return "Decimation"; }

const char* uiStratLayModEditTools::sKeySelectedLevel()
{ return "Selected level"; }

const char* uiStratLayModEditTools::sKeySelectedContent()
{return "Selected content";}

const char* uiStratLayModEditTools::sKeyZoomToggle()
{ return "Allow zoom"; }

const char* uiStratLayModEditTools::sKeyDispLith()
{ return "Display lithology"; }


const char* uiStratLayModEditTools::sKeyShowFlattened()
{ return "Display flattened";}


uiStratGenDescTools::uiStratGenDescTools( uiParent* p )
    : uiGroup(p,"Gen Desc Tools")
    , openReq(this)
    , saveReq(this)
    , propEdReq(this)
    , genReq(this)
    , nrModelsChanged(this)
{
    uiGroup* leftgrp = new uiGroup( this, "Left group" );
    uiToolButton* opentb = new uiToolButton( leftgrp, "open",
				tr("Open stored generation description"),
				mCB(this,uiStratGenDescTools,openCB) );
    savetb_ = new uiToolButton( leftgrp, "save",
				tr("Save generation description"),
				mCB(this,uiStratGenDescTools,saveCB) );
    savetb_->attach( rightOf, opentb );
    uiToolButton* proptb = new uiToolButton( leftgrp, "defprops",
				tr("Select layer properties"),
				mCB(this,uiStratGenDescTools,propEdCB) );
    proptb->attach( rightOf, savetb_ );

    uiGroup* rightgrp = new uiGroup( this, "Right group" );
    const CallBack gocb( mCB(this,uiStratGenDescTools,genCB) );
    nrmodlsfld_ = new uiSpinBox( rightgrp );
    nrmodlsfld_->setInterval( Interval<int>(1,mUdf(int)) );
    nrmodlsfld_->setValue( 25 );
    nrmodlsfld_->setFocusChangeTrigger( false );
    nrmodlsfld_->setStretch( 0, 0 );
    nrmodlsfld_->setToolTip( tr("Number of models to generate") );
    nrmodlsfld_->valueChanging.notify(
	    mCB(this,uiStratGenDescTools,nrModelsChangedCB) );
    uiToolButton* runtb = new uiToolButton( rightgrp, "resume",
				tr("Generate this amount of models"), gocb );
    nrmodlsfld_->attach( leftOf, runtb );
    rightgrp->attach( ensureRightOf, leftgrp );
    rightgrp->setFrame( true );
}


int uiStratGenDescTools::nrModels() const
{
    return nrmodlsfld_->getIntValue();
}


void uiStratGenDescTools::setNrModels( int nrmodels )
{
    NotifyStopper notstop( nrmodlsfld_->valueChanged );
    nrmodlsfld_->setValue( nrmodels );
}


int uiStratGenDescTools::getNrModelsFromPar( const IOPar& par ) const
{
    int nrmodels = -1;
    par.get( sKeyNrModels(), nrmodels );
    return nrmodels;
}


void uiStratGenDescTools::enableSave( bool yn )
{
    savetb_->setSensitive( yn );
}

void uiStratGenDescTools::fillPar( IOPar& par ) const
{
    par.set( sKeyNrModels(), nrModels() );
}


bool uiStratGenDescTools::usePar( const IOPar& par )
{
    int nrmodels;
    if ( par.get( sKeyNrModels(), nrmodels ) )
	nrmodlsfld_->setValue( nrmodels );

    return true;
}


uiMultiStratLevelSel::uiMultiStratLevelSel(
	uiParent* p, uiStratLayModEditTools& tools )
    : uiCompoundParSel(p,tr("Stratigraphic Level"))
    , modtools_(tools)
{
    butPush.notify( mCB(this,uiMultiStratLevelSel,doSelLevelDlg) );
}


void uiMultiStratLevelSel::doSelLevelDlg( CallBacker* )
{
    uiDialog::Setup su( uiStrings::phrSelect(uiStrings::sMarker(mPlural)),
			mNoDlgTitle, mTODOHelpKey );
    uiDialog dlg( parent(), su );

    const uiListBox::Setup setup( OD::ChooseZeroOrMore, tr("Available Markers"),
				  uiListBox::AboveLeft );
    uiListBox* lb = new uiListBox( &dlg, setup );
    lb->setMultiChoice( true );
    lb->addItems( alllevelnames_.getUiStringSet() );
    lb->setChosen( sellevelnames_ );

    if ( !dlg.go() )
	return;

    lb->getChosen( sellevelnames_ );
    modtools_.setLevelNames( sellevelnames_ );
    modtools_.selLevelChg.trigger();

}


uiString uiMultiStratLevelSel::getSummary() const
{
    return toUiString( sellevelnames_.getDispString() );
}


void uiMultiStratLevelSel::setLevelNames( const BufferStringSet& lvlnms )
{
    alllevelnames_ = lvlnms;
    sellevelnames_ = lvlnms;
    updateSummary();
}


uiStratLayModEditTools::uiStratLayModEditTools( uiParent* p )
    : uiGroup(p,"Lay Mod Edit Tools")
    , selPropChg(this)
    , selLevelChg(this)
    , selContentChg(this)
    , dispEachChg(this)
    , dispZoomedChg(this)
    , dispLithChg(this)
    , flattenChg(this)
    , mkSynthChg(this)
    , allownoprop_(false)
{
    uiGroup* leftgrp = new uiGroup( this, "Left group" );
    propfld_ = new uiComboBox( leftgrp, "Display property" );
    propfld_->setToolTip( tr("Displayed property") );
    propfld_->selectionChanged.notify(
				mCB(this,uiStratLayModEditTools,selPropCB) );

    lvlfld_ = new uiMultiStratLevelSel( leftgrp, *this );
    lvlfld_->attach( rightOf, propfld_ );

    contfld_ = new uiComboBox( leftgrp, "Content" );
    contfld_->setToolTip( tr("Marked content") );
    contfld_->attach( rightOf, lvlfld_ );
    contfld_->setHSzPol( uiObject::Small );
    contfld_->selectionChanged.notify(
				mCB(this,uiStratLayModEditTools,selContentCB));

    eachlbl_ = new uiLabel( leftgrp, uiStrings::sEach().toLower() );
    eachlbl_->attach( rightOf, contfld_ );
    eachfld_ = new uiSpinBox( leftgrp, 0, "DispEach" );
    eachfld_->setInterval( 1, 1000 );
    eachfld_->attach( rightOf, eachlbl_ );
    eachfld_->valueChanging.notify(
				mCB(this,uiStratLayModEditTools,dispEachCB) );

    uiGroup* rightgrp = new uiGroup( this, "Right group" );
    mksynthtb_ = new uiToolButton( rightgrp, "autogensynth",
			tr("Automatically create synthetics when on"),
			mCB(this,uiStratLayModEditTools,mkSynthCB) );
    mksynthtb_->setToggleButton( true );
    mksynthtb_->setOn( true );
    const CallBack optcb = mCB(this,uiStratLayModEditTools,flattenMenuCB);
    flattenedtb_ = new uiToolButton( rightgrp, "flattenseis",
			tr("Show flattened when on"),
			mCB(this,uiStratLayModEditTools,showFlatCB) );
    flattenedtb_->setToggleButton( true );
    flattenedtb_->setOn( false );
    uiMenu* flattenlvlmenu = flattenedtb_->addMenu();
    flattenlvlmenu->insertAction(new uiAction(tr("Set Default Level"),
					   optcb,"marker.small"), 0);
    flattenedtb_->attach( leftOf, mksynthtb_ );

    lithtb_ = new uiToolButton( rightgrp, "lithologies",
			tr("Show lithology colors when on"),
			mCB(this,uiStratLayModEditTools,dispLithCB) );
    lithtb_->setToggleButton( true );
    lithtb_->setOn( true );
    lithtb_->attach( leftOf, flattenedtb_ );
    zoomtb_ = new uiToolButton( rightgrp, "toggzooming",
			tr("Do not zoom into models when on"),
			mCB(this,uiStratLayModEditTools,dispZoomedCB) );
    zoomtb_->setToggleButton( true );
    zoomtb_->setOn( false );
    zoomtb_->attach( leftOf, lithtb_ );
    rightgrp->attach( rightTo, leftgrp );
    rightgrp->attach( rightBorder );
}


void uiStratLayModEditTools::showFlatCB( CallBacker* )
{
    if ( choosenlvlnms_.isEmpty() )
    {
	pErrMsg("No levels chosen" );
	return;
    }

    if ( flatlvlnm_.isEmpty() )
	flattenMenuCB(0);

    flattenChg.trigger();
}


const char* uiStratLayModEditTools::getSelLevelFromDlg(
	uiParent* parent, const uiDialog::Setup& dlgsu,
	const uiStringSet& alllvlnms, const char* sellevlnm )
{
    uiDialog dlg( parent, dlgsu );

    const uiListBox::Setup setup( OD::ChooseOnlyOne, tr("Selected Markers"),
				  uiListBox::AboveMid );
    uiLabeledComboBox* lcb =
	new uiLabeledComboBox( &dlg, alllvlnms, tr("Selected Markers") );
    if ( sellevlnm )
	lcb->box()->setText( sellevlnm );

    if ( !dlg.go() )
	return 0;

    mDeclStaticString( ret );
    ret.set( lcb->box()->text() );
    return ret.str();
}


void uiStratLayModEditTools::flattenMenuCB( CallBacker* )
{
    uiDialog::Setup su( uiStrings::phrSelect(tr("Marker for Flattening")),
			mNoDlgTitle, mTODOHelpKey );
    flatlvlnm_ =
	getSelLevelFromDlg( parent(), su, choosenlvlnms_.getUiStringSet(),
			    flatlvlnm_.buf() );
    if ( flatlvlnm_.isEmpty() )
	return;

    flattenedtb_->setOn( true );
    flattenChg.trigger();
}


void uiStratLayModEditTools::setNoDispEachFld()
{
    eachlbl_->display( false ); eachfld_->display( false );
    eachfld_ = 0;
}


static void setFldNms( uiComboBox* cb, const BufferStringSet& nms, bool wnone,
			bool wall, int def )
{
    const BufferString selnm( cb->text() );
    cb->setEmpty();
    if ( wnone )
	cb->addItem( toUiString("---") );
    if ( nms.isEmpty() ) return;

    cb->addItems( nms.getUiStringSet() );
    if ( wall )
	cb->addItem( uiStrings::sAll() );

    if ( wnone ) def++;
    if ( !selnm.isEmpty() )
    {
	def = cb->indexOf( selnm );
	if ( def < 0 )
	    def = 0;
    }
    if ( def >= cb->size() ) def = cb->size() - 1;
    if ( def >= 0 )
	cb->setCurrentItem( def );
}


void uiStratLayModEditTools::setProps( const BufferStringSet& nms )
{
    setFldNms( propfld_, nms, allownoprop_, false, 0 );
}


void uiStratLayModEditTools::setLevelNames( const BufferStringSet& nms )
{
    choosenlvlnms_ = nms;
    lvlfld_->setLevelNames( nms );
    //setFldNms( lvlfld_, nms, true, false, 0 );
}


void uiStratLayModEditTools::setContentNames( const BufferStringSet& nms )
{
    setFldNms( contfld_, nms, true, true, -1 );
}


const char* uiStratLayModEditTools::selProp() const
{
    return propfld_->isEmpty() ? 0 : propfld_->text();
}


int uiStratLayModEditTools::selPropIdx() const
{
    if ( propfld_->isEmpty() )
	return -1;

    const int selidx = propfld_->getIntValue();
    if ( selidx < 0 || (allownoprop_ && selidx == 0) )
	return -1;

    int propidx = selidx;
    if ( !allownoprop_ )
	propidx++;
    return propidx;
}


Strat::Level::ID uiStratLayModEditTools::flattenSelLevelID() const
{
    return lvlfld_ ? Strat::LVLS().getByName( flatlvlnm_ ).id()
		   : Strat::Level::ID::getInvalid();
}


const char* uiStratLayModEditTools::selContent() const
{
    return contfld_->isEmpty() ? 0 : contfld_->text();
}


Strat::Level uiStratLayModEditTools::getFlattenStratLevel() const
{
    const Strat::Level::ID lvlid = flattenSelLevelID();
    return Strat::LVLS().get( lvlid );
}


Color uiStratLayModEditTools::selLevelColor() const
{
    const Strat::Level lvl = getFlattenStratLevel();
    return lvl.color();
}


TypeSet<Strat::Level> uiStratLayModEditTools::getAllSelStratLevels() const
{
    TypeSet<Strat::Level> lvlset;
    for ( int idx=0; idx<choosenlvlnms_.size(); idx++ )
	lvlset += Strat::LVLS().getByName( choosenlvlnms_.get(idx) );
    return lvlset;
}


int uiStratLayModEditTools::dispEach() const
{
    return eachfld_ ? eachfld_->getIntValue() : 1;
}


bool uiStratLayModEditTools::dispZoomed() const
{
    return !zoomtb_->isOn();
}


bool uiStratLayModEditTools::dispLith() const
{
    return lithtb_->isOn();
}


bool uiStratLayModEditTools::showFlattened() const
{
    return flattenedtb_->isOn() &&
	   Strat::LVLS().getByName(flatlvlnm_).id().isValid();
}


bool uiStratLayModEditTools::mkSynthetics() const
{
    return mksynthtb_->isOn();
}


void uiStratLayModEditTools::setSelProp( const char* sel )
{
    propfld_->setText( sel );
}


void uiStratLayModEditTools::setSelContent( const char* sel )
{
    contfld_->setText( sel );
}


void uiStratLayModEditTools::setDispEach( int nr )
{
    if ( eachfld_ ) eachfld_->setValue( nr );
}


void uiStratLayModEditTools::setDispZoomed( bool yn )
{
    zoomtb_->setOn( !yn );
}


void uiStratLayModEditTools::setDispLith( bool yn )
{
    lithtb_->setOn( yn );
}


void uiStratLayModEditTools::setFlatTBSensitive( bool yn )
{
    flattenedtb_->setSensitive( yn );
}


void uiStratLayModEditTools::setShowFlattened( bool yn )
{
    flattenedtb_->setOn( yn );
}


void uiStratLayModEditTools::setMkSynthetics( bool yn )
{
    mksynthtb_->setOn( yn );
}

#define mGetProp( func, key ) \
if ( func ) \
par.set( key, func )

void uiStratLayModEditTools::fillPar( IOPar& par ) const
{
    mGetProp( selProp(), sKeyDisplayedProp() );
    par.set( sKeyDecimation(), dispEach() );

    mGetProp( flatlvlnm_, sKeySelectedLevel() );
    mGetProp( selContent(), sKeySelectedContent() );

    par.setYN( sKeyZoomToggle(), dispZoomed() );
    par.setYN( sKeyDispLith(), dispLith() );
    par.setYN( sKeyShowFlattened(), showFlattened() );
}

#define mSetProp( fld, key ) \
{ \
    FixedString selprop = par[key()]; \
    if ( selprop && fld->isPresent( selprop ) ) \
    { \
	fld->setCurrentItem( selprop ); \
	fld->selectionChanged.trigger(); \
    } \
}

#define mSetYN( func, key, cb ) \
{ \
    bool yn; \
    if ( par.getYN( key(), yn ) ) \
    { \
	func( yn ); \
	cb( 0 ); \
    } \
}


bool uiStratLayModEditTools::usePar( const IOPar& par )
{
    NotifyStopper stopselprop( selPropChg );
    NotifyStopper stoplvlchg( selLevelChg );
    NotifyStopper stopcontchg( selContentChg );
    NotifyStopper stopeachchg( dispEachChg );
    NotifyStopper stopzoomchg( dispZoomedChg );
    NotifyStopper stoplithchg( dispLithChg );
    NotifyStopper stopflatchg( flattenChg );
    NotifyStopper stopsynthchg( mkSynthChg );

    mSetProp( propfld_, sKeyDisplayedProp );
    par.get( sKeySelectedLevel(), flatlvlnm_ );
    mSetProp( contfld_, sKeySelectedContent );

    int decimation;
    if ( par.get( sKeyDecimation(), decimation ) )
    {
	setDispEach( decimation );
	dispEachCB( 0 );
    }

    mSetYN( setDispZoomed, sKeyZoomToggle, dispZoomedCB );
    mSetYN( setDispLith, sKeyDispLith, dispLithCB );
    mSetYN( setShowFlattened, sKeyShowFlattened, showFlatCB );

    return true;
}


//-----------------------------------------------------------------------------

#define mCreatePropSelFld( propnm, txt, prop, prevbox ) \
    uiLabeledComboBox* lblbox##propnm = new uiLabeledComboBox( this, txt ); \
    propnm##fld_ = lblbox##propnm->box(); \
    const PropertyRefSelection subsel##propnm = proprefsel.subselect( prop );\
    for ( int idx=0; idx<subsel##propnm.size(); idx++ )\
	if ( subsel##propnm[idx] )\
	    propnm##fld_->addItem( toUiString(subsel##propnm[idx]->name()) );\
    if ( prevbox )\
	lblbox##propnm->attach( alignedBelow, prevbox );


uiStratLayModFRPropSelector::uiStratLayModFRPropSelector( uiParent* p,
					const PropertyRefSelection& proprefsel,
				const uiStratLayModFRPropSelector::Setup& set )
	: uiDialog(p,uiDialog::Setup(tr("Property Selector"),
				     tr("There are multiple properties "
				        "referenced with the same type. \n"
				        "Please specify which one to use as: "),
		     mODHelpKey(mStratSynthLayerModFRPPropSelectorHelpID) ) )
	, vsfld_(0)
	, porosityfld_(0)
	, initialsatfld_(0)
	, finalsatfld_(0)
{
    mCreatePropSelFld(den, tr("Reference for Density"), PropertyRef::Den, 0);
    mCreatePropSelFld(vp, tr("Reference for Vp"), PropertyRef::Vel, lblboxden);
    uiLabeledComboBox* prevfld = lblboxvp;
    if ( set.withswave_ )
    {
	mCreatePropSelFld(vs, tr("Reference for Vs"), PropertyRef::Vel,prevfld);
	prevfld = lblboxvs;
	if ( proprefsel.find(PropertyRef::standardSVelStr()) >=0 ||
	     proprefsel.find(PropertyRef::standardSVelAliasStr()) >= 0 )
	    setVSProp( PropertyRef::standardSVelStr() );
    }

    if ( set.withinitsat_ )
    {
	mCreatePropSelFld( initialsat, tr("Reference for Initial Saturation"),
			   PropertyRef::Volum, prevfld );
	prevfld = lblboxinitialsat;
	if ( proprefsel.find("Water Saturation") >=0 )
	    setInitialSatProp( "Water Saturation" );
    }

    if ( set.withfinalsat_ )
    {
	mCreatePropSelFld( finalsat, tr("Reference for Final Saturation"),
			   PropertyRef::Volum, prevfld );
	prevfld = lblboxfinalsat;
	if ( proprefsel.find("Water Saturation") >=0 )
	    setFinalSatProp( "Water Saturation" );
    }

    if ( set.withpor_ )
    {
	mCreatePropSelFld( porosity, tr("Reference for Porosity"),
			   PropertyRef::Volum, prevfld );
	if ( proprefsel.find("Porosity") >= 0 )
	    setPorProp( "Porosity" );
    }

    const bool haspwave =
	    proprefsel.find(PropertyRef::standardPVelStr()) >=0 ||
	    proprefsel.find(PropertyRef::standardPVelAliasStr()) >= 0;
    if ( haspwave )
	setVPProp( PropertyRef::standardPVelStr() );
    else
	errmsg_ = tr( "No reference to P wave velocity found" );
}


bool uiStratLayModFRPropSelector::isOK() const
{
    if ( !errmsg_.isEmpty() )
	return false;

    if ( denfld_->isEmpty() || vpfld_->isEmpty() ||
	 (vsfld_ && vsfld_->isEmpty()) )
	return false;

    if ( vsfld_ &&
	 BufferString(vpfld_->text()) == BufferString(vsfld_->text()) )
    {
	uiStratLayModFRPropSelector& modsel =
			const_cast<uiStratLayModFRPropSelector&>( *this );
	modsel.errmsg_ = tr( "Selected property for P wave velocity and "
			     "S wave velocity should be different." );
    }

    if ( (porosityfld_ && porosityfld_->isEmpty()) ||
	 (initialsatfld_ && initialsatfld_->isEmpty()) ||
	 (finalsatfld_ && finalsatfld_->isEmpty()) )
	return false;

    return errmsg_.isEmpty();
}


#define mSetPropFromNm(propfunc,propnm) \
    void uiStratLayModFRPropSelector::set##propfunc##Prop( const char* nm ) \
    { \
	if ( !propnm##fld_ ) \
	    return; \
	\
	if ( propnm##fld_->isPresent(nm) ) \
	    propnm##fld_->setCurrentItem( nm ); \
	else \
	    propnm##fld_->setCurrentItem(0); \
    } \

mSetPropFromNm(Den,den);
mSetPropFromNm(VP,vp);
mSetPropFromNm(VS,vs);
mSetPropFromNm(Por,porosity);
mSetPropFromNm(InitialSat,initialsat);
mSetPropFromNm(FinalSat,finalsat);


bool uiStratLayModFRPropSelector::needsDisplay() const
{
    if ( denfld_->size() > 1 )
	return true;

    if ( vpfld_->size()>1 && (vsfld_ && vsfld_->size()>1) &&
	 BufferString(getSelVPName()) == BufferString(getSelVSName()) )
	return true;

    if ( (porosityfld_ && porosityfld_->size() > 1) ||
	 (initialsatfld_ && initialsatfld_->size() > 1) ||
	 (finalsatfld_ && finalsatfld_->size() > 1) )
	return true;

    return !isOK();
}


bool uiStratLayModFRPropSelector::acceptOK()
{
    return isOK();
}


const char* uiStratLayModFRPropSelector::getSelVPName() const
{ return vpfld_->text(); }

const char* uiStratLayModFRPropSelector::getSelVSName() const
{ return vsfld_ ? vsfld_->text() : 0; }

const char* uiStratLayModFRPropSelector::getSelDenName() const
{ return denfld_->text(); }

const char* uiStratLayModFRPropSelector::getSelPorName() const
{ return porosityfld_ ? porosityfld_->text() : 0; }

const char* uiStratLayModFRPropSelector::getSelInitialSatName() const
{ return initialsatfld_ ? initialsatfld_->text() : 0; }

const char* uiStratLayModFRPropSelector::getSelFinalSatName() const
{ return finalsatfld_ ? finalsatfld_->text() : 0; }
