/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Dec 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiodmenumgr.cc,v 1.238 2012-01-10 22:43:58 cvsnanne Exp $";

#include "uiodmenumgr.h"
#include "uitoolbutton.h"

#include "uicrdevenv.h"
#include "uifiledlg.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uivolprocchain.h"
#include "uiodapplmgr.h"
#include "uiodfaulttoolman.h"
#include "uiodhelpmenumgr.h"
#include "uiodscenemgr.h"
#include "uiodstdmenu.h"
#include "uisettings.h"
#include "ui3dviewer.h"
#include "uitextfile.h"
#include "uitextedit.h"
#include "uitoolbar.h"
#include "uivispartserv.h"
#include "visemobjdisplay.h"
#include "uimsg.h"

#include "dirlist.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "oddirs.h"
#include "odmemory.h"
#include "settings.h"
#include "strmprov.h"
#include "survinfo.h"
#include "thread.h"

static const char* sKeyIconSetNm = "Icon set name";


uiODMenuMgr::uiODMenuMgr( uiODMain* a )
    : appl_(*a)
    , dTectTBChanged(this)
    , dTectMnuChanged(this)
    , helpmgr_(0)
    , inviewmode_(false)
{
    surveymnu_ = new uiPopupMenu( &appl_, "&Survey" );
    analmnu_ = new uiPopupMenu( &appl_, "&Analysis" );
    procmnu_ = new uiPopupMenu( &appl_, "&Processing" );
    scenemnu_ = new uiPopupMenu( &appl_, "S&cenes" );
    viewmnu_ = new uiPopupMenu( &appl_, "&View" );
    utilmnu_ = new uiPopupMenu( &appl_, "&Utilities" );
    helpmnu_ = new uiPopupMenu( &appl_, "&Help" );

    dtecttb_ = new uiToolBar( &appl_, "OpendTect tools", uiToolBar::Top );
    cointb_ = new uiToolBar( &appl_, "Graphical tools", uiToolBar::Left );
    mantb_ = new uiToolBar( &appl_, "Manage data", uiToolBar::Right );

    faulttoolman_ = new uiODFaultToolMan( appl_ );

    appl_.applMgr().visServer()->createToolBars();

    IOM().surveyChanged.notify( mCB(this,uiODMenuMgr,updateDTectToolBar) );
    IOM().surveyChanged.notify( mCB(this,uiODMenuMgr,updateDTectMnus) );
    appl_.applMgr().visServer()->selectionmodechange.notify(
				mCB(this,uiODMenuMgr,selectionMode) );
}


uiODMenuMgr::~uiODMenuMgr()
{
    delete dtecttb_;
    delete cointb_;
    delete mantb_;
    delete helpmgr_;
    delete faulttoolman_;
}


void uiODMenuMgr::initSceneMgrDepObjs( uiODApplMgr* appman,
				       uiODSceneMgr* sceneman )
{
    uiMenuBar* menubar = appl_.menuBar();
    fillSurveyMenu();	menubar->insertItem( surveymnu_ );
    fillAnalMenu();	menubar->insertItem( analmnu_ );
    fillProcMenu();	menubar->insertItem( procmnu_ );
    fillSceneMenu();	menubar->insertItem( scenemnu_ );
    fillViewMenu();	menubar->insertItem( viewmnu_ );
    fillUtilMenu();	menubar->insertItem( utilmnu_ );
    menubar->insertSeparator();
    helpmgr_ = new uiODHelpMenuMgr( this );
    menubar->insertItem( helpmnu_ );

    fillDtectTB( appman );
    fillCoinTB( sceneman );
    fillManTB();
}


uiPopupMenu* uiODMenuMgr::getBaseMnu( uiODApplMgr::ActType at )
{
    return at == uiODApplMgr::Imp ? impmnu_ :
	  (at == uiODApplMgr::Exp ? expmnu_ : manmnu_);
}


uiPopupMenu* uiODMenuMgr::getMnu( bool imp, uiODApplMgr::ObjType ot )
{
    return imp ? impmnus_[(int)ot] : expmnus_[(int)ot];
}


void uiODMenuMgr::updateStereoMenu()
{
    ui3DViewer::StereoType type = 
			(ui3DViewer::StereoType)sceneMgr().getStereoType();
    stereooffitm_->setChecked( type == ui3DViewer::None );
    stereoredcyanitm_->setChecked( type == ui3DViewer::RedCyan );
    stereoquadbufitm_->setChecked( type == ui3DViewer::QuadBuffer );
    stereooffsetitm_->setEnabled( type != ui3DViewer::None );
}


void uiODMenuMgr::updateViewMode( bool isview )
{
    if ( inviewmode_ == isview )
	return;
    toggViewMode( 0 );
}


void uiODMenuMgr::updateAxisMode( bool shwaxis )
{ cointb_->turnOn( axisid_, shwaxis ); }

bool uiODMenuMgr::isSoloModeOn() const
{ return cointb_->isOn( soloid_ ); }

void uiODMenuMgr::enableMenuBar( bool yn )
{ appl_.menuBar()->setSensitive( yn ); }

void uiODMenuMgr::enableActButton( bool yn )
{
    if ( yn )
	{ cointb_->setSensitive( actviewid_, true ); return; }

    if ( !inviewmode_ )
	toggViewMode(0);
    cointb_->setSensitive( actviewid_, false );
}


#define mInsertItem(menu,txt,id) \
    menu->insertItem( \
	new uiMenuItem(txt,mCB(this,uiODMenuMgr,handleClick)), id )

#define mInsertPixmapItem(menu,txt,id,pmfnm) { \
    menu->insertItem( \
	new uiMenuItem(txt,mCB(this,uiODMenuMgr,handleClick),pmfnm), id ); }

#define mCleanUpImpExpSets(set) \
{ \
    while ( !set.isEmpty() ) \
    { \
	uiPopupMenu* pmnu = set.remove(0); \
	if ( pmnu ) delete pmnu; \
    } \
}

void uiODMenuMgr::fillSurveyMenu()
{
    mInsertPixmapItem( surveymnu_, "&Select/Setup ...", mManSurveyMnuItm,
	    	       "survey.png" )

    uiPopupMenu* sessionitm = new uiPopupMenu( &appl_, "S&ession" );
    mInsertItem( sessionitm, "&Save ...", mSessSaveMnuItm );
    mInsertItem( sessionitm, "&Restore ...", mSessRestMnuItm );
    mInsertItem( sessionitm, "&Auto ...", mSessAutoMnuItm );
    surveymnu_->insertItem( sessionitm );
    surveymnu_->insertSeparator();

    impmnu_ = new uiPopupMenu( &appl_, "&Import" );
    fillImportMenu();
    surveymnu_->insertItem( impmnu_ );

    expmnu_ = new uiPopupMenu( &appl_, "Ex&port" );
    fillExportMenu();
    surveymnu_->insertItem( expmnu_ );

    manmnu_ = new uiPopupMenu( &appl_, "&Manage");
    fillManMenu();
    surveymnu_->insertItem( manmnu_ );

    preloadmnu_ = new uiPopupMenu( &appl_, "&Pre-load");
    mInsertItem( preloadmnu_, "&Seismics ...", mPreLoadSeisMnuItm );
    mInsertItem( preloadmnu_, "&Horizons ...", mPreLoadHorMnuItm );
    surveymnu_->insertItem( preloadmnu_ );

    surveymnu_->insertSeparator();
    mInsertItem( surveymnu_, "E&xit", mExitMnuItm );
}



void uiODMenuMgr::fillImportMenu()
{
    impmnu_->clear();
    uiPopupMenu* impseis = new uiPopupMenu( &appl_, "&Seismics" );
    uiPopupMenu* imphor = new uiPopupMenu( &appl_, "&Horizons" );
    uiPopupMenu* impfault = new uiPopupMenu( &appl_, "&Faults" );
    uiPopupMenu* impfaultstick = new uiPopupMenu( &appl_, "F&aultStickSets" );
    uiPopupMenu* impwell = new uiPopupMenu( &appl_, "&Wells" );
    uiPopupMenu* imppick = new uiPopupMenu( &appl_, "&PickSets" );
    uiPopupMenu* impwvlt = new uiPopupMenu( &appl_, "&Wavelets" );
    uiPopupMenu* impmute = new uiPopupMenu( &appl_, "&Mute Functions" );
    uiPopupMenu* impcpd = new uiPopupMenu( &appl_, "&Cross-plot data" );
    uiPopupMenu* impvelfn = new uiPopupMenu( &appl_, "&Velocity Functions" );
    uiPopupMenu* imppdf =
	new uiPopupMenu( &appl_, "Probability &Density Functions" );
    impmnu_->insertItem( impseis );
    impmnu_->insertItem( imphor );
    impmnu_->insertItem( impfault );
    impmnu_->insertItem( impfaultstick );
    impmnu_->insertItem( impwell );
    impmnu_->insertItem( imppick );
    impmnu_->insertItem( impwvlt );
    impmnu_->insertItem( impmute );
    impmnu_->insertItem( impvelfn );
    impmnu_->insertItem( impcpd );
    impmnu_->insertItem( imppdf );

    mInsertItem( imppick, "&Ascii ...", mImpPickAsciiMnuItm );
    mInsertItem( impwvlt, "&Ascii ...", mImpWvltAsciiMnuItm );
    mInsertItem( impmute, "&Ascii ...", mImpMuteDefAsciiMnuItm );
    mInsertItem( impcpd, "&Ascii ...", mImpPVDSAsciiMnuItm );
    mInsertItem( impvelfn, "&Ascii ...", mImpVelocityAsciiMnuItm );
    mInsertItem( imppdf, "RokDoc &Ascii ...", mImpPDFAsciiMnuItm );

    mInsertItem( impseis, "SEG-&Y ...", mImpSeisSEGYMnuItm );
    mInsertItem( impseis, "SEG-Y s&canned ...", mImpSeisSEGYDirectMnuItm );
    uiPopupMenu* impseissimple = new uiPopupMenu( &appl_, "&Simple file" );
    mInsertItem( impseissimple, "&3D ...", mImpSeisSimple3DMnuItm );
    mInsertItem( impseissimple, "&2D ...", mImpSeisSimple2DMnuItm );
    mInsertItem( impseissimple, "&Pre-Stack 3D ...", mImpSeisSimplePS3DMnuItm );
    mInsertItem( impseissimple, "Pre-S&tack 2D ...", mImpSeisSimplePS2DMnuItm );
    impseis->insertItem( impseissimple );
    uiPopupMenu* impcbvsseis = new uiPopupMenu( &appl_, "&CBVS" );
    mInsertItem( impcbvsseis, "&From file ...", mImpSeisCBVSMnuItm );
    mInsertItem( impcbvsseis, "&From other survey ...", 
					   mImpSeisCBVSOtherSurvMnuItm );
    impseis->insertItem( impcbvsseis );

    uiPopupMenu* imphorasc = new uiPopupMenu( &appl_, "&Ascii" );
    mInsertItem( imphorasc, "&Geometry 3D ...", mImpHorAsciiMnuItm );
    mInsertItem( imphorasc, "&Attributes 3D ...", mImpHorAsciiAttribMnuItm );
    mInsertItem( imphorasc, "&Geometry 2D ...", mImpHor2DAsciiMnuItm );
    imphor->insertItem( imphorasc );

    mInsertItem( impfault, "&Ascii 3D ...", mImpFaultMnuItm );
    mInsertItem( impfaultstick, "&Ascii 3D ...", mImpFaultSSAscii3DMnuItm );
    mInsertItem( impfaultstick, "&Ascii 2D ...", mImpFaultSSAscii2DMnuItm );

    uiPopupMenu* impwellasc = new uiPopupMenu( &appl_, "&Ascii" );
    mInsertItem( impwellasc, "&Track ...", mImpWellAsciiTrackMnuItm );
    mInsertItem( impwellasc, "&Logs ...", mImpWellAsciiLogsMnuItm );
    mInsertItem( impwellasc, "&Markers ...", mImpWellAsciiMarkersMnuItm );
    impwell->insertItem( impwellasc );
    mInsertItem( impwell, "&VSP (SEG-Y) ...", mImpWellSEGYVSPMnuItm );
    mInsertItem( impwell, "Simple &Multi-Well ...", mImpWellSimpleMnuItm );

    impmnus_.erase();
    impmnus_.allowNull();
    impmnus_ += impseis; impmnus_ += imphor; impmnus_ += impfault;
    impmnus_ += impwell; impmnus_ += 0; impmnus_ += 0;
    impmnus_ += imppick; impmnus_ += 0; impmnus_ += 0;
    impmnus_ += impwvlt; impmnus_ += impmute; impmnus_ += impvelfn;
    impmnus_ += imppdf;
}


void uiODMenuMgr::fillExportMenu()
{
    expmnu_->clear();
    uiPopupMenu* expseis = new uiPopupMenu( &appl_, "&Seismics" );
    uiPopupMenu* exphor = new uiPopupMenu( &appl_, "&Horizons" );
    uiPopupMenu* expflt = new uiPopupMenu( &appl_, "&Faults" );
    uiPopupMenu* expfltss = new uiPopupMenu( &appl_, "F&aultStickSets" );
    uiPopupMenu* exppick = new uiPopupMenu( &appl_, "&PickSets" );
    uiPopupMenu* expwvlt = new uiPopupMenu( &appl_, "&Wavelets" );
    uiPopupMenu* expmute = new uiPopupMenu( &appl_, "&Mute Functions" );
    uiPopupMenu* exppdf =
	new uiPopupMenu( &appl_, "Probability &Density Functions" );
    expmnu_->insertItem( expseis );
    expmnu_->insertItem( exphor );
    expmnu_->insertItem( expflt );
    expmnu_->insertItem( expfltss );
    expmnu_->insertItem( exppick );
    expmnu_->insertItem( expwvlt );
    expmnu_->insertItem( expmute );
    expmnu_->insertItem( exppdf );

    uiPopupMenu* expseissgy = new uiPopupMenu( &appl_, "&SEG-Y" );
    mInsertItem( expseissgy, "&3D ...", mExpSeisSEGY3DMnuItm );
    mInsertItem( expseissgy, "&2D ...", mExpSeisSEGY2DMnuItm );
    mInsertItem( expseissgy, "&Pre-Stack 3D ...", mExpSeisSEGYPS3DMnuItm );
    expseis->insertItem( expseissgy );
    uiPopupMenu* expseissimple = new uiPopupMenu( &appl_, "S&imple file" );
    mInsertItem( expseissimple, "&3D ...", mExpSeisSimple3DMnuItm );
    mInsertItem( expseissimple, "&2D ...", mExpSeisSimple2DMnuItm );
    mInsertItem( expseissimple, "&Pre-Stack 3D ...", mExpSeisSimplePS3DMnuItm );
    expseis->insertItem( expseissimple );

    mInsertItem( exphor, "Ascii &3D...", mExpHorAscii3DMnuItm );
    mInsertItem( exphor, "Ascii &2D...", mExpHorAscii2DMnuItm );
    mInsertItem( expflt, "&Ascii ...", mExpFltAsciiMnuItm );
    mInsertItem( expfltss, "&Ascii ...", mExpFltSSAsciiMnuItm );
    mInsertItem( exppick, "&Ascii ...", mExpPickAsciiMnuItm );
    mInsertItem( expwvlt, "&Ascii ...", mExpWvltAsciiMnuItm );
    mInsertItem( expmute, "&Ascii ...", mExpMuteDefAsciiMnuItm );
    mInsertItem( exppdf, "RokDoc &Ascii ...", mExpPDFAsciiMnuItm );

    expmnus_.erase();
    expmnus_.allowNull();
    expmnus_ += expseis; expmnus_ += exphor; expmnus_+= expflt;
    expmnus_+=0; expmnus_+= 0; expmnus_+= 0;
    expmnus_+=exppick; expmnus_+= 0; expmnus_+= 0;
    expmnus_+=expwvlt; expmnus_+= expmute; expmnus_+= 0; expmnus_ += exppdf;
}


void uiODMenuMgr::fillManMenu()
{
    manmnu_->clear();
    mInsertPixmapItem( manmnu_, "&AttributeSets ...", mManAttrMnuItm,
	    	 "man_attrs.png" );
    mInsertPixmapItem( manmnu_, "&Faults ...", mManFaultMnuItm, "man_flt.png" )
    mInsertPixmapItem( manmnu_, "&FaultStickSets ...", mManFaultStickMnuItm,
	    			"man_fltss.png" );
    if ( SI().survDataType() == SurveyInfo::No2D )
	mInsertPixmapItem( manmnu_, "&Horizons ...", mManHor3DMnuItm,
			   "man_hor.png" )
    else
    {
	uiPopupMenu* mnu = new uiPopupMenu( &appl_, "&Horizons", "man_hor.png");
	mInsertItem( mnu, "&2D ...", mManHor2DMnuItm );
	mInsertItem( mnu, "&3D ...", mManHor3DMnuItm );
	manmnu_->insertItem( mnu );
    }

    mInsertPixmapItem( manmnu_, "&PickSets/Polygons ...", mManPickMnuItm,
		       "man_picks.png" );
    mInsertPixmapItem( manmnu_, "&Body ...", mManBodyMnuItm,"man_body.png" );
    mInsertPixmapItem( manmnu_, "Probability &Density Functions ...",
		 mManPDFMnuItm, "man_prdfs.png" );
    mInsertPixmapItem( manmnu_, "2D Geometry ..",
		 mManGeomItm, "man2dgeom.png" );
    mInsertPixmapItem( manmnu_, "Cross Plot data..",
		 mManCrossPlotItm, "manxplot.png" );
    create2D3DMnu( manmnu_, "&Seismics", mManSeis2DMnuItm, mManSeis3DMnuItm,
		   "man_seis.png" );

    mInsertPixmapItem( manmnu_, "Strati&graphy ...", mManStratMnuItm,
	    	       "man_strat.png" )
    mInsertPixmapItem( manmnu_, "Wa&velets ...", mManWvltMnuItm, "man_wvlt.png")
    mInsertPixmapItem( manmnu_, "&Wells ...", mManWellMnuItm, "man_wll.png" )
}


void uiODMenuMgr::create2D3DMnu( uiPopupMenu* itm, const char* title,
				 int id2d, int id3d, const char* pmfnm )
{
    if ( SI().has2D() && SI().has3D() )
    {
	uiPopupMenu* mnu = new uiPopupMenu( &appl_, title, pmfnm );
	mInsertItem( mnu, "&2D ...", id2d );
	mInsertItem( mnu, "&3D ...", id3d );
	itm->insertItem( mnu );
    }
    else
    {
	const BufferString titledots( title, " ..." );
	if ( SI().has2D() )
	    mInsertPixmapItem( itm, titledots, id2d, pmfnm )
	else if ( SI().has3D() )
	    mInsertPixmapItem( itm, titledots, id3d, pmfnm )
    }
}


void uiODMenuMgr::fillProcMenu()
{
    procmnu_->clear();

    uiPopupMenu* csoitm = new uiPopupMenu( &appl_, "&Create Seismic Output" );
    create2D3DMnu( csoitm, "&Attribute", mSeisOut2DMnuItm, mSeisOut3DMnuItm, 0);
    if ( SI().has3D() )
    {
	csoitm->insertItem(
	    new uiMenuItem("Volume &Builder ...",
			mCB(&applMgr(),uiODApplMgr,createVolProcOutput)) );
	csoitm->insertItem(
	    new uiMenuItem("&Time - depth conversion ...",
			mCB(&applMgr(),uiODApplMgr,processTime2Depth)) );
	csoitm->insertItem(
	    new uiMenuItem("&Velocity conversion ...",
			mCB(&applMgr(),uiODApplMgr,processVelConv)) );
	csoitm->insertItem(
	    new uiMenuItem("&Pre Stack processing ...",
			mCB(&applMgr(),uiODApplMgr,processPreStack)) );
	csoitm->insertItem(
	    new uiMenuItem("Angle mute function ...",
			mCB(&applMgr(),uiODApplMgr,genAngleMuteFunction) ));
	csoitm->insertItem(
	    new uiMenuItem("Bayesian &Classification ...",
			mCB(&applMgr(),uiODApplMgr,bayesClass3D), "bayes.png"));
    }
    create2D3DMnu( csoitm, "&Between horizons", mCompBetweenHor2DMnuItm,
	    	   mCompBetweenHor3DMnuItm, "betweenhors.png" );
    create2D3DMnu( csoitm, "&Along horizon", mCompAlongHor2DMnuItm,
	    	   mCompAlongHor3DMnuItm, "alonghor.png" );
    mInsertItem( csoitm, "&Re-Start ...", mReStartMnuItm );
    uiPopupMenu* scnitm = new uiPopupMenu( &appl_, "SEG-&Y Scanned Re-sort" );
    csoitm->insertItem( new uiMenuItem("SEG-&Y Scanned Re-sort ...",
		    mCB(&applMgr(),uiODApplMgr,resortSEGY)) );

    procmnu_->insertItem( csoitm );

    uiPopupMenu* grditm = new uiPopupMenu( &appl_, "Create Grid Output");
    create2D3DMnu( grditm, "&Grid", mCreateSurf2DMnuItm, mCreateSurf3DMnuItm,
	    	   "ongrid.png" );
    procmnu_->insertItem( grditm );

}


void uiODMenuMgr::fillAnalMenu()
{
    analmnu_->clear();
    SurveyInfo::Pol2D survtype = SI().survDataType();
    const char* attrpm = "attributes.png";
    if ( survtype == SurveyInfo::Both2DAnd3D )
    {
	uiPopupMenu* aitm = new uiPopupMenu( &appl_, "&Attributes", attrpm );
	mInsertItem( aitm, "&2D ...", mEdit2DAttrMnuItm );
	mInsertItem( aitm, "&3D ...", mEdit3DAttrMnuItm );

	analmnu_->insertItem( aitm );
	analmnu_->insertSeparator();
    }
    else
    {
       	mInsertPixmapItem( analmnu_, "&Attributes ...", mEditAttrMnuItm, attrpm)
	analmnu_->insertSeparator();
    }

    if ( survtype!=SurveyInfo::Only2D )
    {
	analmnu_->insertItem( new uiMenuItem( "Volume Builder ...",
				mCB(&applMgr(),uiODApplMgr,doVolProcCB),
				VolProc::uiChain::pixmapFileName() ) );
    }

    uiPopupMenu* crsplot = new uiPopupMenu( &appl_, "&Cross-plot", "xplot.png");
    mInsertItem( crsplot, "&Well logs <--> Attributes ...", mXplotMnuItm );
    mInsertItem( crsplot, "&Attributes <--> Attributes ...", mAXplotMnuItm );
    mInsertItem( crsplot, "&Open Crossplot ...", mOpenXplotMnuItm );
    analmnu_->insertItem( crsplot );

    if (  SI().zIsTime() )
	analmnu_->insertItem( new uiMenuItem( "&Tie Well to Seismic ...",
	    mCB(&applMgr(),uiODApplMgr,tieWellToSeismic), "well_tie.png" ) );
}


void uiODMenuMgr::fillSceneMenu()
{
    mInsertItem( scenemnu_, "&New", mAddSceneMnuItm );

    addtimedepthsceneitm_ = new uiMenuItem( "Dummy",
	    				    mCB(this,uiODMenuMgr,handleClick) );
    scenemnu_->insertItem( addtimedepthsceneitm_, mAddTmeDepthMnuItm );

    mInsertItem( scenemnu_, "&Cascade", mCascadeMnuItm );
    uiPopupMenu* tileitm = new uiPopupMenu( &appl_, "&Tile" );
    scenemnu_->insertItem( tileitm );

    mInsertItem( tileitm, "&Auto", mTileAutoMnuItm );
    mInsertItem( tileitm, "&Horizontal", mTileHorMnuItm );
    mInsertItem( tileitm, "&Vertical", mTileVerMnuItm );
    scenemnu_->insertSeparator();

    updateSceneMenu();
}



void uiODMenuMgr::updateSceneMenu()
{
    BufferStringSet scenenms;
    int activescene = 0;
    sceneMgr().getSceneNames( scenenms, activescene );

    for ( int idx=0; idx<=scenenms.size(); idx++ )
    {
	const int id = mSceneSelMnuItm + idx;
	uiMenuItem* itm = scenemnu_->find( id );

	if ( idx >= scenenms.size() )
	{
	    if ( itm )
		scenemnu_->removeItem( id );
	    continue;
	}

	if ( !itm )
	{
	    itm = new uiMenuItem( "", mCB(this,uiODMenuMgr,handleClick) );
	    scenemnu_->insertItem( itm, id );
	    itm->setCheckable( true );
	}

	itm->setName( scenenms.get(idx) );
	itm->setText( scenenms.get(idx) );
	itm->setChecked( idx==activescene );
    }

    BufferString itmtxt( "New [" );
    itmtxt += SI().zIsTime() ? "&Depth]" : "T&ime]";
    addtimedepthsceneitm_->setText( itmtxt );
}


void uiODMenuMgr::fillViewMenu()
{
    if ( !viewmnu_ ) return;

    viewmnu_->clear();
    mInsertItem( viewmnu_, "&Base Map ...", mBaseMapMnuItm );
    mInsertItem( viewmnu_, "&Work area ...", mWorkAreaMnuItm );
    mInsertItem( viewmnu_, "&Z-scale ...", mZScaleMnuItm );
    uiPopupMenu* stereoitm = new uiPopupMenu( &appl_, "&Stereo viewing" );
    viewmnu_->insertItem( stereoitm );

#define mInsertStereoItem(itm,txt,docheck,id) \
    itm = new uiMenuItem( txt, mCB(this,uiODMenuMgr,handleClick) ); \
    stereoitm->insertItem( itm, id ); \
    itm->setCheckable( true ); \
    itm->setChecked( docheck );

    mInsertStereoItem( stereooffitm_, "&Off", true, mStereoOffMnuItm )
    mInsertStereoItem( stereoredcyanitm_, "&Red/Cyan", false,
	    		mStereoRCMnuItm )
    mInsertStereoItem( stereoquadbufitm_, "&Quad buffer", false,
	    		mStereoQuadMnuItm )

    stereooffsetitm_ = new uiMenuItem( "&Stereo offset ...",
				mCB(this,uiODMenuMgr,handleClick) );
    stereoitm->insertItem( stereooffsetitm_, mStereoOffsetMnuItm );
    stereooffsetitm_->setEnabled( false );

    mkViewIconsMnu();
    viewmnu_->insertSeparator();

    uiPopupMenu& toolbarsmnu = appl_.getToolbarsMenu();
    toolbarsmnu.item().setName("&Toolbars");
    viewmnu_->insertItem( &toolbarsmnu );
}


void uiODMenuMgr::addIconMnuItems( const DirList& dl, uiPopupMenu* iconsmnu,
				   BufferStringSet& nms )
{
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const BufferString nm( dl.get( idx ).buf() + 6 );
	if ( nm.isEmpty() || nms.isPresent(nm) )
	    continue;

	BufferString mnunm( "&" ); mnunm += nm;
	mInsertItem( iconsmnu, mnunm, mViewIconsMnuItm+nms.size() );
	nms.add( nm );
    }
}


void uiODMenuMgr::mkViewIconsMnu()
{
    DirList dlsett( GetSettingsDir(), DirList::DirsOnly, "icons.*" );
    DirList dlsite( mGetApplSetupDataDir(), DirList::DirsOnly, "icons.*" );
    DirList dlrel( mGetSWDirDataDir(), DirList::DirsOnly, "icons.*" );
    if ( dlsett.size() + dlsite.size() + dlrel.size() < 2 )
	return;

    uiPopupMenu* iconsmnu = new uiPopupMenu( &appl_, "&Icons" );
    viewmnu_->insertItem( iconsmnu );
    mInsertItem( iconsmnu, "&Default", mViewIconsMnuItm+0 );
    BufferStringSet nms; nms.add( "Default" );
    addIconMnuItems( dlsett, iconsmnu, nms );
    addIconMnuItems( dlsite, iconsmnu, nms );
    addIconMnuItems( dlrel, iconsmnu, nms );
}


mBasicExtern const char* logMsgFileName();

void uiODMenuMgr::fillUtilMenu()
{
    settmnu_ = new uiPopupMenu( &appl_, "&Settings" );
    utilmnu_->insertItem( settmnu_ );
    mInsertItem( settmnu_, "&Fonts ...", mSettFontsMnuItm );
    mInsertItem( settmnu_, "&Look and feel ...", mSettLkNFlMnuItm );
    mInsertItem( settmnu_, "&Mouse controls ...", mSettMouseMnuItm );
    mInsertItem( settmnu_, "&Keyboard shortcuts ...", mSettShortcutsMnuItm );
    uiPopupMenu* advmnu = new uiPopupMenu( &appl_, "&Advanced" );
    mInsertItem( advmnu, "&Personal settings ...", mSettGeneral );
    mInsertItem( advmnu, "&Survey defaults ...", mSettSurvey );
    settmnu_->insertItem( advmnu );

    toolsmnu_ = new uiPopupMenu( &appl_, "&Tools" );
    utilmnu_->insertItem( toolsmnu_ );

    mInsertItem( toolsmnu_, "&Batch programs ...", mBatchProgMnuItm );
    mInsertItem( toolsmnu_, "&Position conversion ...", mPosconvMnuItm );
    mInsertItem( toolsmnu_, "&Create Devel. Env. ...", mCrDevEnvMnuItm );
    mInsertItem( utilmnu_, "&Plugins ...", mPluginsMnuItm );

    if ( File::isWritable(GetSoftwareDir(1)) )
	mInsertItem( utilmnu_, "&Installation Manager ...", mInstMgrMnuItem );

    const char* lmfnm = logMsgFileName();
    if ( lmfnm && *lmfnm )
	mInsertItem( utilmnu_, "Show &log file ...", mShwLogFileMnuItm );
#ifdef __debug__
    const bool enabdpdump = true;
#else
    const bool enabdpdump = GetEnvVarYN( "OD_ENABLE_DATAPACK_DUMP" );
#endif
    if ( enabdpdump )
    {
	mInsertItem( toolsmnu_, "Data pack dump ...", mDumpDataPacksMnuItm );
	if ( OD::canDumpMemInfo() )
	{
	    mInsertItem( toolsmnu_, "Display memory info ...",
			 mDisplayMemoryMnuItm );
	}
    }

}


#define mAddTB(tb,fnm,txt,togg,fn) \
    tb->addButton( fnm, txt, mCB(appman,uiODApplMgr,fn), togg )

void uiODMenuMgr::fillDtectTB( uiODApplMgr* appman )
{
    mAddTB(dtecttb_,"survey.png","Survey setup",false,manSurvCB);

    SurveyInfo::Pol2D survtype = SI().survDataType();
    if ( survtype == SurveyInfo::Only2D )
    {
	mAddTB(dtecttb_,"attributes.png","Edit attributes",false,editAttr2DCB);
	mAddTB(dtecttb_,"out_2dlines.png","Create seismic output",
	       false,seisOut2DCB);
    }
    else if ( survtype == SurveyInfo::No2D )
    {
	mAddTB( dtecttb_,"attributes.png","Edit attributes",false,editAttr3DCB);
	mAddTB( dtecttb_,"out_vol.png","Create seismic output",false,
		seisOut3DCB);
	mAddTB( dtecttb_,VolProc::uiChain::pixmapFileName(),
		"Volume Builder",false,doVolProc);
    }
    else
    {
	mAddTB(dtecttb_,"attributes_2d.png","Edit 2D attributes",false,
	       editAttr2DCB);
	mAddTB(dtecttb_,"attributes_3d.png","Edit 3D attributes",false,
	       editAttr3DCB);
	mAddTB(dtecttb_,"out_2dlines.png","Create 2D seismic output",false,
	       seisOut2DCB);
	mAddTB(dtecttb_,"out_vol.png","Create 3D seismic output",false,
	       seisOut3DCB);
	mAddTB( dtecttb_,VolProc::uiChain::pixmapFileName(),
		"Volume Builder",false,doVolProc);
    }
    mAddTB(dtecttb_,"xplot_wells.png","Crossplot Attribute vs Well data",
	   false,doWellXPlot);
    mAddTB(dtecttb_,"xplot_attribs.png","Crossplot Attribute vs Attribute data",
	   false,doAttribXPlot);

    dTectTBChanged.trigger();
}


#undef mAddTB
#define mAddTB(tb,fnm,txt,togg,fn) \
    tb->addButton( fnm, txt, mCB(this,uiODMenuMgr,fn), togg )

#define mAddPopUp( nm, txt1, txt2, itm1, itm2, mnuid ) { \
    uiPopupMenu* popmnu = new uiPopupMenu( &appl_, nm ); \
    popmnu->insertItem( new uiMenuItem(txt1, \
		       mCB(this,uiODMenuMgr,handleClick)), itm1 ); \
    popmnu->insertItem( new uiMenuItem(txt2, \
		       mCB(this,uiODMenuMgr,handleClick)), itm2 ); \
    mantb_ ->setButtonMenu( mnuid, popmnu ); }

void uiODMenuMgr::fillManTB()
{
    const int seisid =
	mAddTB(mantb_,"man_seis.png","Manage Seismic data",false,manSeis);
    const int horid =
	mAddTB(mantb_,"man_hor.png","Manage Horizons",false,manHor);
    const int fltid =
	mAddTB(mantb_,"man_flt.png","Manage Faults",false,manFlt);
    mAddTB(mantb_,"man_wll.png","Manage well data",false,manWll);
    mAddTB(mantb_,"man_picks.png","Manage PickSets/Polygons",false,manPick);
    //mAddTB(mantb_,"man_body.png","Manage body",false,manBody);
    mAddTB(mantb_,"man_wvlt.png","Manage Wavelets",false,manWvlt);
    mAddTB(mantb_,"man_strat.png","Manage Stratigraphy",false,manStrat);
 
    if ( SI().survDataType() == SurveyInfo::Both2DAnd3D )
	mAddPopUp( "Seismics Menu", "2D Seismics", "3D Seismics",
		   mManSeis2DMnuItm, mManSeis3DMnuItm, seisid );
 
    if ( SI().survDataType() != SurveyInfo::No2D )
	mAddPopUp( "Horizon Menu", "2D Horizons", "3D Horizons",
		   mManHor2DMnuItm, mManHor3DMnuItm, horid );

    mAddPopUp( "Fault Menu", "Faults", "FaultStickSets",
	       mManFaultMnuItm, mManFaultStickMnuItm, fltid );
}


static bool sIsPolySelect = true;

#undef mAddTB
#define mAddTB(tb,fnm,txt,togg,fn) \
    tb->addButton( fnm, txt, mCB(scenemgr,uiODSceneMgr,fn), togg )

#define mAddMnuItm(mnu,txt,fn,fnm,idx) {\
    uiMenuItem* itm = new uiMenuItem( txt, mCB(this,uiODMenuMgr,fn) ); \
    mnu->insertItem( itm, idx ); itm->setPixmap( fnm ); }

void uiODMenuMgr::fillCoinTB( uiODSceneMgr* scenemgr )
{
    actviewid_ = cointb_->addButton( "altpick.png", "Switch view mode",
	    		mCB(this,uiODMenuMgr,toggViewMode), false );
    mAddTB(cointb_,"home.png","To home position",false,toHomePos);
    mAddTB(cointb_,"set_home.png","Save home position",false,saveHomePos);
    mAddTB(cointb_,"view_all.png","View all",false,viewAll);
    cameraid_ = mAddTB(cointb_,"perspective.png",
	    	       "Switch to orthographic camera",false,switchCameraType);
    
    curviewmode_ = ui3DViewer::Inl;
    bool separateviewbuttons = false;
    Settings::common().getYN( "dTect.SeparateViewButtons", separateviewbuttons);
    if ( !separateviewbuttons )
    {
	viewselectid_ = cointb_->addButton( "cube_inl.png","View Inline",
				mCB(this,uiODMenuMgr,handleViewClick), false );

	uiPopupMenu* vwmnu = new uiPopupMenu( &appl_, "View Menu" );
	mAddMnuItm( vwmnu, "View Inline", handleViewClick, "cube_inl.png",0);
	mAddMnuItm( vwmnu, "View Crossline", handleViewClick, "cube_crl.png",1);
	mAddMnuItm( vwmnu, "View Z", handleViewClick, "cube_z.png",2);
	mAddMnuItm( vwmnu, "View North", handleViewClick, "view_N.png",3);
	mAddMnuItm( vwmnu, "View North - Z", handleViewClick, "view_NZ.png",4);
	cointb_->setButtonMenu( viewselectid_, vwmnu );
	viewinlid_ = viewcrlid_ = viewzid_ = viewnid_ = viewnzid_ = -1;
    }
    else
    {
#define mAddVB(img,txt) cointb_->addButton( img, txt, \
			mCB(this,uiODMenuMgr,handleViewClick), false );
	viewinlid_ = mAddVB( "cube_inl.png", "View Inline" );
	viewcrlid_ = mAddVB( "cube_crl.png", "View Crossline" );
	viewzid_ = mAddVB( "cube_z.png", "View Z" );
	viewnid_ = mAddVB( "view_N.png", "View North" );
	viewnzid_ = mAddVB( "view_NZ.png", "View North Z" );
	viewselectid_ = -1;
    }

    mAddTB( cointb_, "dir-light.png", "Set directional light", false, 
	    doDirectionalLight);
    
    axisid_ = mAddTB(cointb_,"axis.png","Display orientation axis",
	    	     true,showRotAxis);
    coltabid_ = cointb_->addButton( "colorbar.png", "Display color bar",
			    mCB(this,uiODMenuMgr,dispColorBar), true );
    mAddTB(cointb_,"snapshot.png","Take snapshot",false,mkSnapshot);
    polyselectid_ = cointb_->addButton( "polygonselect.png",
	"Polygon Selection mode", mCB(this,uiODMenuMgr,selectionMode), true );
    uiPopupMenu* mnu = new uiPopupMenu( &appl_, "Menu" );
    mAddMnuItm( mnu, "Polygon", handleToolClick, "polygonselect.png", 0 );
    mAddMnuItm( mnu, "Rectangle", handleToolClick, "rectangleselect.png", 1 );
    cointb_->setButtonMenu( polyselectid_, mnu );

    removeselectionid_ = cointb_->addButton( "trashcan.png", "Remove selection",
		    mCB(this,uiODMenuMgr,removeSelection), false );

    soloid_ = mAddTB(cointb_,"solo.png","Display current element only",
		     true,soloMode);
}


void uiODMenuMgr::handleViewClick( CallBacker* cb )
{
    mDynamicCastGet(uiMenuItem*,itm,cb)
    mDynamicCastGet(uiToolButton*,tb,cb)

    if ( viewselectid_ < 0 )
    {
	if ( !tb ) return;
	const int clickid = tb->id();
	if ( clickid == viewinlid_ ) curviewmode_ = ui3DViewer::Inl;
	if ( clickid == viewcrlid_ ) curviewmode_ = ui3DViewer::Crl;
	if ( clickid == viewzid_ ) curviewmode_ = ui3DViewer::Z;
	if ( clickid == viewnid_ ) curviewmode_ = ui3DViewer::Y;
	if ( clickid == viewnzid_ ) curviewmode_ = ui3DViewer::YZ;
    }

    if ( tb )
    {
	sceneMgr().setViewSelectMode( curviewmode_ );
	return;
    }

    if ( !itm ) return;
   
    int itmid = itm->id();
    BufferString pm( "cube_inl.png" );
    BufferString tt( "View Inline" );
    curviewmode_ = ui3DViewer::Inl;
    switch( itmid )
    {
	case 1: pm = "cube_crl.png"; tt = "View Crossline";
		curviewmode_ = ui3DViewer::Crl; break;
	case 2: pm = "cube_z.png"; tt = "View Z";
		curviewmode_ = ui3DViewer::Z; break;
	case 3: pm = "view_N.png"; tt = "View North"; 
		curviewmode_ = ui3DViewer::Y; break;
	case 4: pm = "view_NZ.png"; tt = "View North Z"; 
		curviewmode_ = ui3DViewer::YZ; break; 
    }

    cointb_->setPixmap( viewselectid_, pm );
    cointb_->setToolTip( viewselectid_, tt );
    sceneMgr().setViewSelectMode( curviewmode_ );
}


void uiODMenuMgr::handleToolClick( CallBacker* cb )
{
    mDynamicCastGet(uiMenuItem*,itm,cb)
    if ( !itm ) return;

    sIsPolySelect = itm->id()==0;
    selectionMode( 0 );
}


void uiODMenuMgr::selectionMode( CallBacker* cb )
{
    uiVisPartServer* visserv = appl_.applMgr().visServer();
    if ( cb == visserv )
    {
	cointb_->turnOn( polyselectid_, visserv->isSelectionModeOn() );
	sIsPolySelect = visserv->getSelectionMode()==uiVisPartServer::Polygon;
    }
    else
    {
	uiVisPartServer::SelectionMode mode = sIsPolySelect ?
			 uiVisPartServer::Polygon : uiVisPartServer::Rectangle;
	visserv->turnSelectionModeOn( cointb_->isOn(polyselectid_) );
	visserv->setSelectionMode( mode );
    }

    cointb_->setPixmap( polyselectid_, sIsPolySelect ?
	    		"polygonselect.png" : "rectangleselect.png" );
    cointb_->setToolTip( polyselectid_, sIsPolySelect ?
			"Polygon Selection mode" : "Rectangle Selection mode" );
}


void uiODMenuMgr::removeSelection( CallBacker* )
{
    uiVisPartServer& visserv = *appl_.applMgr().visServer();
    visserv.removeSelection();
}


void uiODMenuMgr::dispColorBar( CallBacker* )
{
    appl_.applMgr().visServer()->displaySceneColorbar(
	    cointb_->isOn(coltabid_) );
}


void uiODMenuMgr::setCameraPixmap( bool perspective )
{
    cointb_->setToolTip(cameraid_,perspective ? "Switch to orthographic camera"
					      : "Switch to perspective camera");
    cointb_->setPixmap( cameraid_, perspective ? "perspective.png"
	    				       : "orthographic.png" );
}


#define mDoOp(ot,at,op) \
	applMgr().doOperation(uiODApplMgr::at,uiODApplMgr::ot,op)

void uiODMenuMgr::handleClick( CallBacker* cb )
{
    mDynamicCastGet(uiMenuItem*,itm,cb)
    if ( !itm ) return; // Huh?

    const int id = itm->id();
    switch( id )
    {
    case mManSurveyMnuItm:		applMgr().manageSurvey(); break;
    case mSessSaveMnuItm:		appl_.saveSession(); break;
    case mSessRestMnuItm:		appl_.restoreSession(); break;
    case mSessAutoMnuItm:		appl_.autoSession(); break;
    case mImpSeisCBVSMnuItm:		mDoOp(Imp,Seis,0); break;
    case mImpSeisSEGYMnuItm:		mDoOp(Imp,Seis,1); break;
    case mImpSeisSEGYDirectMnuItm:	mDoOp(Imp,Seis,2); break;
    case mImpSeisSimple3DMnuItm:	mDoOp(Imp,Seis,5); break;
    case mImpSeisSimple2DMnuItm:	mDoOp(Imp,Seis,6); break;
    case mImpSeisSimplePS3DMnuItm:	mDoOp(Imp,Seis,7); break;
    case mImpSeisSimplePS2DMnuItm:	mDoOp(Imp,Seis,8); break;
    case mImpSeisCBVSOtherSurvMnuItm:	mDoOp(Imp,Seis,9); break;
    case mExpSeisSEGY3DMnuItm:		mDoOp(Exp,Seis,1); break;
    case mExpSeisSEGY2DMnuItm:		mDoOp(Exp,Seis,2); break;
    case mExpSeisSEGYPS3DMnuItm:	mDoOp(Exp,Seis,3); break;
    case mExpSeisSEGYPS2DMnuItm:	mDoOp(Exp,Seis,4); break;
    case mExpSeisSimple3DMnuItm:	mDoOp(Exp,Seis,5); break;
    case mExpSeisSimple2DMnuItm:	mDoOp(Exp,Seis,6); break;
    case mExpSeisSimplePS3DMnuItm:	mDoOp(Exp,Seis,7); break;
    case mExpSeisSimplePS2DMnuItm:	mDoOp(Exp,Seis,8); break;
    case mImpHorAsciiMnuItm:		mDoOp(Imp,Hor,0); break;
    case mImpHorAsciiAttribMnuItm:	mDoOp(Imp,Hor,1); break;
    case mImpHor2DAsciiMnuItm:		mDoOp(Imp,Hor,2); break;
    case mExpHorAscii3DMnuItm:		mDoOp(Exp,Hor,0); break;
    case mExpHorAscii2DMnuItm:		mDoOp(Exp,Hor,1); break;
    case mExpFltAsciiMnuItm:		mDoOp(Exp,Flt,0); break;
    case mExpFltSSAsciiMnuItm:		mDoOp(Exp,Flt,1); break;			
    case mImpWellAsciiTrackMnuItm:	mDoOp(Imp,Wll,0); break;
    case mImpWellAsciiLogsMnuItm:	mDoOp(Imp,Wll,1); break;
    case mImpWellAsciiMarkersMnuItm:	mDoOp(Imp,Wll,2); break;
    case mImpWellSEGYVSPMnuItm:		mDoOp(Imp,Wll,3); break;
    case mImpWellSimpleMnuItm:		mDoOp(Imp,Wll,4); break;
    case mImpPickAsciiMnuItm:		mDoOp(Imp,Pick,0); break;
    case mExpPickAsciiMnuItm:		mDoOp(Exp,Pick,0); break;
    case mExpWvltAsciiMnuItm:		mDoOp(Exp,Wvlt,0); break;
    case mImpWvltAsciiMnuItm:		mDoOp(Imp,Wvlt,0); break;
    case mImpFaultMnuItm:		mDoOp(Imp,Flt,0); break;
    case mImpFaultSSAscii3DMnuItm:	mDoOp(Imp,Flt,1); break;
    case mImpFaultSSAscii2DMnuItm:	mDoOp(Imp,Flt,2); break;
    case mImpMuteDefAsciiMnuItm:	mDoOp(Imp,MDef,0); break;
    case mExpMuteDefAsciiMnuItm:	mDoOp(Exp,MDef,0); break;
    case mImpPVDSAsciiMnuItm:		mDoOp(Imp,PVDS,0); break;
    case mImpVelocityAsciiMnuItm:	mDoOp(Imp,Vel,0); break;
    case mImpPDFAsciiMnuItm:		mDoOp(Imp,PDF,0); break;
    case mExpPDFAsciiMnuItm:		mDoOp(Exp,PDF,0); break;
    case mManSeis3DMnuItm:		mDoOp(Man,Seis,2); break;
    case mManSeis2DMnuItm:		mDoOp(Man,Seis,1); break;
    case mManHor3DMnuItm:		mDoOp(Man,Hor,2); break;
    case mManHor2DMnuItm:		mDoOp(Man,Hor,1); break;
    case mManFaultStickMnuItm:		mDoOp(Man,Flt,1); break;
    case mManFaultMnuItm:		mDoOp(Man,Flt,2); break;
    case mManBodyMnuItm:		mDoOp(Man,Body,0); break;		
    case mManWellMnuItm:		mDoOp(Man,Wll,0); break;
    case mManPickMnuItm:		mDoOp(Man,Pick,0); break;
    case mManWvltMnuItm:		mDoOp(Man,Wvlt,0); break;
    case mManAttrMnuItm:		mDoOp(Man,Attr,0); break;
    case mManNLAMnuItm:			mDoOp(Man,NLA,0); break;
    case mManSessMnuItm:		mDoOp(Man,Sess,0); break;
    case mManStratMnuItm:		mDoOp(Man,Strat,0); break;
    case mManPDFMnuItm:			mDoOp(Man,PDF,0); break;
    case mManGeomItm:			mDoOp(Man,Geom,0); break;
    case mManCrossPlotItm:		mDoOp(Man,PVDS,0); break;

    case mPreLoadSeisMnuItm:	applMgr().manPreLoad(uiODApplMgr::Seis); break;
    case mPreLoadHorMnuItm:	applMgr().manPreLoad(uiODApplMgr::Hor); break;
    case mExitMnuItm: 		appl_.exit(); break;
    case mEditAttrMnuItm: 	applMgr().editAttribSet(); break;
    case mEdit2DAttrMnuItm: 	applMgr().editAttribSet(true); break;
    case mEdit3DAttrMnuItm: 	applMgr().editAttribSet(false); break;
    case mSeisOutMnuItm:	applMgr().createVol( SI().has2D() ); break;
    case mSeisOut2DMnuItm: 	applMgr().createVol(true); break;
    case mSeisOut3DMnuItm: 	applMgr().createVol(false); break;
    case mCreateSurf2DMnuItm: 	applMgr().createHorOutput(0,true); break;
    case mCreateSurf3DMnuItm: 	applMgr().createHorOutput(0,false); break;
    case mCompAlongHor2DMnuItm:	applMgr().createHorOutput(1,true); break;
    case mCompAlongHor3DMnuItm:	applMgr().createHorOutput(1,false); break;
    case mCompBetweenHor2DMnuItm: applMgr().createHorOutput(2,true); break;
    case mCompBetweenHor3DMnuItm: applMgr().createHorOutput(2,false); break;
    case mReStartMnuItm: 	applMgr().reStartProc(); break;
    case mXplotMnuItm:		applMgr().doWellXPlot(); break;
    case mAXplotMnuItm:		applMgr().doAttribXPlot(); break;
    case mOpenXplotMnuItm:	applMgr().openCrossPlot(); break;
    case mAddSceneMnuItm:	sceneMgr().tile(); // leave this, or --> crash!
				sceneMgr().addScene(true); break;
    case mAddTmeDepthMnuItm:	applMgr().addTimeDepthScene(); break;
    case mCascadeMnuItm: 	sceneMgr().cascade(); break;
    case mTileAutoMnuItm: 	sceneMgr().tile(); break;
    case mTileHorMnuItm: 	sceneMgr().tileHorizontal(); break;
    case mTileVerMnuItm: 	sceneMgr().tileVertical(); break;
    case mBaseMapMnuItm:	applMgr().showBaseMap(); break;
    case mWorkAreaMnuItm: 	applMgr().setWorkingArea(); break;
    case mZScaleMnuItm: 	applMgr().setZStretch(); break;
    case mBatchProgMnuItm: 	applMgr().batchProgs(); break;
    case mPluginsMnuItm: 	applMgr().pluginMan(); break;
    case mPosconvMnuItm:	applMgr().posConversion(); break;	
    case mInstMgrMnuItem:	applMgr().startInstMgr(); break;	
    case mCrDevEnvMnuItm: 	uiCrDevEnv::crDevEnv(&appl_); break;
    case mShwLogFileMnuItm: 	showLogFile(); break;
    case mSettFontsMnuItm: 	applMgr().setFonts(); break;
    case mSettMouseMnuItm: 	sceneMgr().setKeyBindings(); break;

    case mSettLkNFlMnuItm: {
	uiLooknFeelSettings dlg( &appl_, "Set Look and Feel Settings" );
	if ( dlg.go() && dlg.isChanged() )
	    uiMSG().message("Your new settings will become active\nthe next "
		    	    "time OpendTect is started.");
    } break;

    case mDumpDataPacksMnuItm: {
	uiFileDialog dlg( &appl_, false, "/tmp/dpacks.txt",
			  "*.txt", "Data pack dump" );
	dlg.setAllowAllExts( true );
	if ( dlg.go() )
	{
	    StreamData sd( StreamProvider(dlg.fileName()).makeOStream() );
	    if ( sd.usable() ) DataPackMgr::dumpDPMs( *sd.ostrm );
	}
    } break;
    case mDisplayMemoryMnuItm: {
	IOPar iopar;
	OD::dumpMemInfo( iopar );
	BufferString text;
	iopar.dumpPretty( text );
	uiDialog dlg( &appl_, uiDialog::Setup("Memory information", mNoDlgTitle,
					      mNoHelpID ) );
	uiTextBrowser* browser = new uiTextBrowser( &dlg );
	browser->setText( text.buf() );
	dlg.setCancelText( 0 );
	dlg.go();
    } break;

    case mSettGeneral: {
	uiSettings dlg( &appl_, "Set Personal Settings" );
	dlg.go();
    } break;
    case mSettSurvey: {
	uiSettings dlg( &appl_, "Set Survey default Settings",
				uiSettings::sKeySurveyDefs() );
	dlg.go();
    } break;

    case mSettShortcutsMnuItm:	applMgr().manageShortcuts(); break;

    case mStereoOffsetMnuItm: 	applMgr().setStereoOffset(); break;
    case mStereoOffMnuItm: 
    case mStereoRCMnuItm : 
    case mStereoQuadMnuItm :
    {
	const int type = id == mStereoRCMnuItm ? 1 
	    				: (id == mStereoQuadMnuItm ? 2 : 0 );
	sceneMgr().setStereoType( type );
	updateStereoMenu();
    } break;

    default:
    {
	if ( id>=mSceneSelMnuItm && id<=mSceneSelMnuItm +100 )
	{
	    const char* scenenm = itm->name();
	    sceneMgr().setActiveScene( scenenm );
	    itm->setChecked( true );
	}

	if ( id >= mViewIconsMnuItm && id < mViewIconsMnuItm+100 )
	{
	    Settings::common().set( sKeyIconSetNm, itm->name().buf() + 1 );
	    for ( int idx=0; idx<uiToolBar::toolBars().size(); idx++ )
		uiToolBar::toolBars()[idx]->reLoadPixMaps();
	    Settings::common().write();
	}
	if ( id > mHelpMnu )
	    helpmgr_->handle( id, itm->name() );

    } break;

    }
}


int uiODMenuMgr::ask2D3D( const char* txt, int res2d, int res3d, int rescncl )
{
    int res = rescncl;
    if ( !SI().has2D() )
	res = res3d;
    else if ( !SI().has3D() )
	res = res2d;
    else
    {
	const int msg = uiMSG().askGoOnAfter( txt, "Cancel", "2D", "3D" );
	res = msg == 2 ? rescncl : ( msg == 0 ? res2d : res3d );
    }

    return res;
}


void uiODMenuMgr::manHor( CallBacker* )
{
    const int opt = ask2D3D( "Manage 2D or 3D Horizons", 1, 2, 0 );
    if ( opt == 0 ) return;

    mDoOp(Man,Hor,opt);
}


void uiODMenuMgr::manSeis( CallBacker* )
{
    const int opt = ask2D3D( "Manage 2D or 3D Seismics", 1, 2, 0 );
    if ( opt == 0 ) return;

    mDoOp(Man,Seis,opt);
}

#define mDefManCBFn(typ) \
    void uiODMenuMgr::man##typ( CallBacker* ) { mDoOp(Man,typ,0); }

mDefManCBFn(Flt)
mDefManCBFn(Wll)
mDefManCBFn(Pick)
mDefManCBFn(Body)    
mDefManCBFn(Wvlt)
mDefManCBFn(Strat)
mDefManCBFn(PDF)


void uiODMenuMgr::showLogFile()
{
    uiTextFileDlg::Setup su( logMsgFileName() );
    su.modal( true );
    uiTextFileDlg dlg( &appl_, su );
    dlg.go();
}


void uiODMenuMgr::updateDTectToolBar( CallBacker* )
{
    dtecttb_->clear();
    mantb_->clear();
    fillDtectTB( &applMgr() );
    fillManTB();
}


void uiODMenuMgr::toggViewMode( CallBacker* cb )
{
    inviewmode_ = !inviewmode_;
    cointb_->setPixmap( actviewid_,
	    		inviewmode_ ? "altview.png" : "altpick.png" );
    if ( inviewmode_ )
	sceneMgr().viewMode( cb );
    else
	sceneMgr().actMode( cb );
}


void uiODMenuMgr::updateDTectMnus( CallBacker* )
{
    fillImportMenu();
    fillExportMenu();
    fillManMenu();

    updateSceneMenu();

    fillAnalMenu();
    fillProcMenu();
    dTectMnuChanged.trigger();
}
