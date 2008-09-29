/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvey.cc,v 1.92 2008-09-29 13:23:48 cvsbert Exp $
________________________________________________________________________

-*/

#include "uisurvey.h"

#include "survinfo.h"
#include "uibutton.h"
#include "uicanvas.h"
#include "uiconvpos.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uisurvinfoed.h"
#include "uisurvmap.h"
#include "uisetdatadir.h"
#include "uitextedit.h"
#include "uifileinput.h"
#include "mousecursor.h"
#include "uimain.h"
#include "uifont.h"
#include "iodrawtool.h"
#include "dirlist.h"
#include "ioman.h"
#include "ctxtioobj.h"
#include "filegen.h"
#include "filepath.h"
#include "oddirs.h"
#include "iostrm.h"
#include "strmprov.h"
#include "envvars.h"
#include "cubesampling.h"
#include "odver.h"
#include "pixmap.h"

#include <iostream>
#include <math.h>

#define mMapWidth	300
#define mMapHeight	300
extern "C" const char* GetSurveyName();
extern "C" const char* GetSurveyFileName();
extern "C" void SetSurveyName(const char*);
extern bool IOMAN_no_survchg_triggers;

static ObjectSet<uiSurvey::Util>& getUtils()
{
    static ObjectSet<uiSurvey::Util>* utils = 0;
    if ( !utils )
    {
	utils = new ObjectSet<uiSurvey::Util>;
	*utils += new uiSurvey::Util( "xy2ic.png", "Convert (X,Y) to/from I/C",
				      CallBack() );
    }
    return *utils;
}


void uiSurvey::add( const uiSurvey::Util& util )
{
    getUtils() += new uiSurvey::Util( util );
}


static BufferString getTrueDir( const char* dn )
{
    BufferString dirnm = dn;
    FilePath fp;
    while ( File_isLink(dirnm) )
    {
	BufferString newdirnm = File_linkTarget(dirnm);
	fp.set( newdirnm );
	if ( !fp.isAbsolute() )
	{
	    FilePath dirnmfp( dirnm );
	    dirnmfp.setFileName( 0 );
	    fp.setPath( dirnmfp.fullPath() );
	}
	dirnm = fp.fullPath();
    }
    return dirnm;
}


static bool copySurv( const char* from, const char* todirnm, int mb )
{
    FilePath fp( GetBaseDataDir() ); fp.add( todirnm );
    const BufferString todir( fp.fullPath() );
    if ( File_exists(todir) )
    {
	BufferString msg( "A survey '" );
	msg += todirnm;
	msg += "' already exists.\nYou will have to remove it first";
        uiMSG().error( msg );
        return false;
    }

    BufferString msg;
    if ( mb > 0 )
	{ msg += mb; msg += " MB of data"; }
    else
	msg += "An unknown amount";
    msg += " of data needs to be copied.\nDuring the copy, OpendTect will "
	    "freeze.\nDepending on the data transfer rate, this can take "
	    "a long time!\n\nDo you wish to continue?";
    if ( !uiMSG().askGoOn( msg ) )
	return false;

    const BufferString fromdir = getTrueDir( FilePath(from).fullPath() );

    MouseCursorChanger cc( MouseCursor::Wait );
    if ( !File_copy( fromdir, todir, mFile_Recursive ) )
    {
        uiMSG().error( "Cannot copy the survey data" );
        return false;
    }

    File_makeWritable( todir, mFile_Recursive, mC_True );
    return true;
}


uiSurvey::uiSurvey( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Survey selection",
	       "Select and setup survey","0.3.1"))
    , initialdatadir_(GetBaseDataDir())
    , initialsurvey_(GetSurveyName())
    , survinfo_(0)
    , survmap_(0)
{
    SurveyInfo::produceWarnings( false );
    const int lbwidth = 250;
    const int noteshght = 5;
    const int totwdth = lbwidth + mMapWidth + 10;

    const char* ptr = GetBaseDataDir();
    if ( !ptr ) return;

    mkDirList();

    uiGroup* rightgrp = new uiGroup( this, "Survey selection right" );

    survmap_ = new uiSurveyMap( rightgrp );
    survmap_->setStretch( 0, 0 );
    survmap_->setWidth( mMapWidth );
    survmap_->setHeight( mMapHeight );

    uiGroup* leftgrp = new uiGroup( this, "Survey selection left" );
    listbox_ = new uiListBox( leftgrp, dirlist_, "Surveys" );
    listbox_->setCurrentItem( GetSurveyName() );
    listbox_->selectionChanged.notify( mCB(this,uiSurvey,selChange) );
    listbox_->doubleClicked.notify( mCB(this,uiSurvey,accept) );
    listbox_->setPrefWidth( lbwidth );
    listbox_->setStretch( 2, 2 );
    leftgrp->attach( leftOf, rightgrp );

    newbut_ = new uiPushButton( leftgrp, "&New",
	    			mCB(this,uiSurvey,newButPushed), false );
    newbut_->attach( rightOf, listbox_ );
    newbut_->setPrefWidthInChar( 12 );
    rmbut_ = new uiPushButton( leftgrp, "&Remove",
	    		       mCB(this,uiSurvey,rmButPushed), false );
    rmbut_->attach( alignedBelow, newbut_ );
    rmbut_->setPrefWidthInChar( 12 );
    editbut_ = new uiPushButton( leftgrp, "&Edit",
	    			 mCB(this,uiSurvey,editButPushed), false );
    editbut_->attach( alignedBelow, rmbut_ );
    editbut_->setPrefWidthInChar( 12 );
    copybut_ = new uiPushButton( leftgrp, "C&opy",
	    			 mCB(this,uiSurvey,copyButPushed), false );
    copybut_->attach( alignedBelow, editbut_ );
    copybut_->setPrefWidthInChar( 12 );

    ObjectSet<uiSurvey::Util>& utils = getUtils();
    uiGroup* utilbutgrp = new uiGroup( rightgrp, "Surv Util buttons" );
    const CallBack cb( mCB(this,uiSurvey,utilButPush) );
    for ( int idx=0; idx<utils.size(); idx++ )
    {
	const uiSurvey::Util& util = *utils[idx];
	uiToolButton* but = new uiToolButton( utilbutgrp, util.tooltip_,
						ioPixmap(util.pixmap_), cb );
	but->setToolTip( util.tooltip_ );
	utilbuts_ += but;
	if ( idx > 0 )
	    utilbuts_[idx]->attach( rightOf, utilbuts_[idx-1] );
    }
    utilbutgrp->attach( centeredBelow, survmap_ );

    datarootbut_ = new uiPushButton( leftgrp, "&Set Data Root",
	    			mCB(this,uiSurvey,dataRootPushed), false );
    datarootbut_->attach( centeredBelow, listbox_ );

    uiSeparator* horsep1 = new uiSeparator( this );
    horsep1->setPrefWidth( totwdth );
    horsep1->attach( stretchedBelow, rightgrp, -2 );
    horsep1->attach( ensureBelow, leftgrp );

    uiGroup* infoleft = new uiGroup( this, "Survey info left" );
    uiGroup* inforight = new uiGroup( this, "Survey info right" );
    infoleft->attach( alignedBelow, leftgrp );
    infoleft->attach( ensureBelow, horsep1 );
    inforight->attach( alignedBelow, rightgrp );
    inforight->attach( ensureBelow, horsep1 );

    infoleft->setFont( uiFontList::get(FontData::key(FontData::ControlSmall)) );
    inforight->setFont( uiFontList::get(FontData::key(FontData::ControlSmall)));

    inllbl_ = new uiLabel( infoleft, "" ); 
    crllbl_ = new uiLabel( infoleft, "" );
    zlbl_ = new uiLabel( inforight, "" ); 
    binlbl_ = new uiLabel( inforight, "" );
#if 0
    inllbl_->setHSzPol( uiObject::widevar );
    crllbl_->setHSzPol( uiObject::widevar );
    zlbl_->setHSzPol( uiObject::widevar );
    binlbl_->setHSzPol( uiObject::widevar );
#else
    inllbl_->setPrefWidthInChar( 40 );
    crllbl_->setPrefWidthInChar( 40 );
    zlbl_->setPrefWidthInChar( 40 );
    binlbl_->setPrefWidthInChar( 40 );
#endif

    crllbl_->attach( alignedBelow, inllbl_ );
    binlbl_->attach( alignedBelow, zlbl_ );
   
    uiSeparator* horsep2 = new uiSeparator( this );
    horsep2->attach( stretchedBelow, infoleft, -2 );
    horsep2->setPrefWidth( totwdth );

    uiLabel* notelbl = new uiLabel( this, "Notes:" );
    notelbl->attach( alignedBelow, horsep2 );
    notes_ = new uiTextEdit( this, "Notes" );
    notes_->attach( alignedBelow, notelbl);
    notes_->setPrefHeightInChar( noteshght );
    notes_->setPrefWidth( totwdth );
   
    getSurvInfo(); 
    mkInfo();
    setOkText( "&Ok (Select)" );

    finaliseDone.notify( mCB(this,uiSurvey,doDrawMap) );
}


uiSurvey::~uiSurvey()
{
    delete survinfo_;
    delete survmap_;
}


void uiSurvey::newButPushed( CallBacker* )
{
    if ( !survmap_ ) return;
    BufferString oldnm = listbox_->getText();
  
    FilePath fp( GetSoftwareDir() );
    fp.add( "data" ).add( "BasicSurvey" );
    delete survinfo_;
    survinfo_ = SurveyInfo::read( fp.fullPath() );
    survinfo_->dirname = "";
    mkInfo();
    if ( !survInfoDialog() )
	updateInfo(0);

    rmbut_->setSensitive(true);
    editbut_->setSensitive(true);
    for ( int idx=0; idx<utilbuts_.size(); idx++ )
	utilbuts_[idx]->setSensitive(true);
}


void uiSurvey::editButPushed( CallBacker* )
{
    if ( !survInfoDialog() )
	updateInfo(0);
}


class uiSurveyGetCopyDir : public uiDialog
{
public:

uiSurveyGetCopyDir( uiParent* p, const char* cursurv )
	: uiDialog(p,uiDialog::Setup("Import survey from location",
		   "Copy surveys from any data root","0.3.1"))
{
    BufferString curfnm;
    if ( cursurv && *cursurv )
    {
	FilePath fp( GetBaseDataDir() ); fp.add( cursurv );
	curfnm = fp.fullPath();
    }
    inpfld = new uiFileInput( this,
	    	"Opendtect survey directory to copy",
		uiFileInput::Setup(curfnm).directories(true));
    inpfld->setDefaultSelectionDir( GetBaseDataDir() );
    inpfld->valuechanged.notify( mCB(this,uiSurveyGetCopyDir,inpSel) );

    newdirnmfld = new uiGenInput( this, "New survey directory name", "" );
    newdirnmfld->attach( alignedBelow, inpfld );
}


void inpSel( CallBacker* )
{
    fname = inpfld->fileName();
    FilePath fp( fname );
    newdirnmfld->setText( fp.fileName() );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool acceptOK( CallBacker* )
{
    fname = inpfld->fileName();
    if ( !File_exists(fname) )
	mErrRet( "Selected directory does not exist" );
    if ( !File_isDirectory(fname) )
	mErrRet( "Please select a valid directory" );
    FilePath fp( fname );
    fp.add( ".survey" );
    if ( !File_exists( fp.fullPath() ) )
	mErrRet( "This is not an OpendTect survey directory" );

    newdirnm = newdirnmfld->text();
    if ( newdirnm.isEmpty() )
	{ inpSel(0); newdirnm = newdirnmfld->text(); }
    cleanupString( newdirnm.buf(), mC_False, mC_False, mC_True );

    return true;
}

    BufferString	fname;
    BufferString	newdirnm;
    uiFileInput*	inpfld;
    uiGenInput*		newdirnmfld;

};


void uiSurvey::copyButPushed( CallBacker* )
{
    uiSurveyGetCopyDir dlg( this, listbox_->getText() );
    if ( !dlg.go() )
	return;

    if ( !copySurv( dlg.fname, dlg.newdirnm, -1 ) )
	return;

    updateSvyList();
    listbox_->setCurrentItem( dlg.newdirnm );
    updateSvyFile();
}


void uiSurvey::dataRootPushed( CallBacker* )
{
    uiSetDataDir dlg( this );
    if ( dlg.go() )
    {
	mkDirList();
	updateSvyList();
    }
}


void uiSurvey::mkDirList()
{
    dirlist_.deepErase();
    getSurveyList( dirlist_ );
}


bool uiSurvey::survInfoDialog()
{
    uiSurveyInfoEditor dlg( this, *survinfo_ );
    dlg.survparchanged.notify( mCB(this,uiSurvey,updateInfo) );
    if ( !dlg.go() )
	return false;

    updateSvyList();
    listbox_->setCurrentItem( dlg.dirName() );
//    updateSvyFile();
    return true;
}


void uiSurvey::rmButPushed( CallBacker* )
{
    BufferString selnm( listbox_->getText() );
    const BufferString seldirnm = FilePath(GetBaseDataDir())
					    .add(selnm).fullPath();
    const BufferString truedirnm = getTrueDir( seldirnm );

    BufferString msg( "This will remove the entire survey:\n\t" );
    msg += selnm;
    msg += "\nFull path: "; msg += truedirnm;
    msg += "\nAre you sure you wish to continue?";
    if ( !uiMSG().askGoOn( msg ) ) return;


    MouseCursorManager::setOverride( MouseCursor::Wait );
    bool rmres = File_remove( truedirnm, mFile_Recursive );
    MouseCursorManager::restoreOverride();
    if ( !rmres )
    {
        msg = truedirnm; msg += "\nnot removed properly";
        return;
    }

    if ( seldirnm != truedirnm ) // must have been a link
	File_remove( seldirnm, mFile_NotRecursive );

    updateSvyList();
    const char* ptr = GetSurveyName();
    if ( ptr && selnm == ptr )
    {
        BufferString newsel( listbox_->getText() );
        writeSurveyName( newsel );
	if ( button(CANCEL) ) button(CANCEL)->setSensitive( false );
    }
}


void uiSurvey::utilButPush( CallBacker* cb )
{
    mDynamicCastGet(uiToolButton*,tb,cb)
    if ( !tb ) { pErrMsg("Huh"); return; }

    const int butidx = utilbuts_.indexOf( tb );
    if ( butidx < 0 ) { pErrMsg("Huh"); return; }

    if ( butidx == 0 )
    {
	uiConvertPos dlg( this, *survinfo_ );
	dlg.go();
    }
    else
    {
	Util* util = getUtils()[butidx];
	util->cb_.doCall( this );
    }
}


void uiSurvey::updateSvyList()
{
    mkDirList();
    if ( dirlist_.isEmpty() ) updateInfo(0);
    listbox_->empty();
    listbox_->addItems( dirlist_ );
}


bool uiSurvey::updateSvyFile()
{
    BufferString seltxt( listbox_->getText() );
    if ( seltxt.isEmpty() ) return true;

    if ( !writeSurveyName( seltxt ) )
    {
        ErrMsg( "Cannot update the 'survey' file in $HOME/.od/" );
        return false;
    }
    if ( !File_exists( FilePath(GetBaseDataDir()).add(seltxt).fullPath() ) )
    {
        ErrMsg( "Survey directory does not exist anymore" );
        return false;
    }

    newSurvey();
    SetSurveyName( seltxt );

    return true;
}


bool uiSurvey::writeSurveyName( const char* nm )
{
    const char* ptr = GetSurveyFileName();
    if ( !ptr )
    {
        ErrMsg( "Error in survey system. Please check $HOME/.od/" );
        return false;
    }

    StreamData sd = StreamProvider( ptr ).makeOStream();
    if ( !sd.usable() )
    {
        BufferString errmsg = "Cannot write to ";
        errmsg += ptr;
        ErrMsg( errmsg );
        return false;
    }

    *sd.ostrm << nm;

    sd.close();
    return true;
}


void uiSurvey::mkInfo()
{
    const SurveyInfo& si = *survinfo_;
    BufferString inlinfo( "In-line range: " );
    BufferString crlinfo( "Cross-line range: " );
    BufferString zinfo( "Z range " );
    zinfo += si.getZUnit(); zinfo += ": ";
    BufferString bininfo( "Bin size (m/line): " );

    if ( si.sampling(false).hrg.totalNr() )
    {
	inlinfo += si.sampling(false).hrg.start.inl;
	inlinfo += " - "; inlinfo += si.sampling(false).hrg.stop.inl;
	inlinfo += "  step: "; inlinfo += si.inlStep();
	
	crlinfo += si.sampling(false).hrg.start.crl;
	crlinfo += " - "; crlinfo += si.sampling(false).hrg.stop.crl;
	crlinfo += "  step: "; crlinfo += si.crlStep();

	float inldist = si.inlDistance();
	float crldist = si.crlDistance();

	#define mkString(dist) \
	nr = (int)(dist*100+.5); bininfo += nr/100; \
	rest = nr%100; bininfo += rest < 10 ? ".0" : "."; bininfo += rest; \

	int nr, rest;    
	bininfo += "inl: "; mkString(inldist);
	bininfo += "  crl: "; mkString(crldist);
    }

    #define mkZString(nr) \
    zinfo += istime ? mNINT(1000*nr) : nr;

    const bool istime = si.zIsTime(); 
    mkZString( si.zRange(false).start );
    zinfo += " - "; mkZString( si.zRange(false).stop );
    zinfo += "  step: "; mkZString( si.zRange(false).step );

    inllbl_->setText( inlinfo );
    crllbl_->setText( crlinfo );
    binlbl_->setText( bininfo );
    zlbl_->setText( zinfo );
    notes_->setText( si.comment() );

    bool anysvy = dirlist_.size();
    rmbut_->setSensitive( anysvy );
    editbut_->setSensitive( anysvy );
    for ( int idx=0; idx<utilbuts_.size(); idx++ )
	utilbuts_[idx]->setSensitive( anysvy );
}


void uiSurvey::selChange()
{
    writeComments();
    updateInfo(0);
}


void uiSurvey::updateInfo( CallBacker* cb )
{
    mDynamicCastGet(uiSurveyInfoEditor*,dlg,cb);
    if ( !dlg )
	getSurvInfo();

    mkInfo();
    survmap_->removeItems();
    survmap_->drawMap( survinfo_ );
    //mapcanvas_->update();
}


void uiSurvey::writeComments()
{
    BufferString txt = notes_->text();
    if ( txt == survinfo_->comment() ) return;

    survinfo_->setComment( txt );
    if ( !survinfo_->write( GetBaseDataDir() ) )
        ErrMsg( "Failed to write survey info.\nNo changes committed." );
}


void uiSurvey::doDrawMap( CallBacker* )
{
    survmap_->setWidth( mMapWidth );
    survmap_->setHeight( mMapHeight );
    survmap_->drawMap( survinfo_ );
}


bool uiSurvey::rejectOK( CallBacker* )
{
    if ( initialdatadir_ != GetBaseDataDir() )
    {
	if ( !uiSetDataDir::setRootDataDir( initialdatadir_ ) )
	{
	    uiMSG().error( "As we cannot reset to the old Data Root,\n"
		    	   "You *have to* select a survey now!" );
	    return false;
	}
    }

    IOMAN_no_survchg_triggers = true;
    IOMan::setSurvey( initialsurvey_ );
    SurveyInfo::theinst_ = SurveyInfo::read(
	FilePath(initialdatadir_).add(initialsurvey_).fullPath() );
    IOMAN_no_survchg_triggers = false;

    SurveyInfo::produceWarnings( true );
    return true;
}


bool uiSurvey::acceptOK( CallBacker* )
{
    writeComments();
    SurveyInfo::produceWarnings( true );
    if ( !updateSvyFile() || !IOMan::newSurvey() )
    { SurveyInfo::produceWarnings( false ); return false; }

    newSurvey();
    updateViewsGlobal();
    return true;
}


void uiSurvey::updateViewsGlobal()
{
    BufferString capt( "OpendTect V" );
    capt += GetFullODVersion();
    capt += "/"; capt += __plfsubdir__;

    const char* usr = GetSoftwareUser();
    if ( usr && *usr )
	{ capt += " ["; capt += usr; capt += "]"; }

    if ( !SI().name().isEmpty() )
    {
	capt += ": ";
	capt += SI().name();
    }

    uiMain::setTopLevelCaption( capt );
}


void uiSurvey::getSurvInfo()
{
    BufferString fname = FilePath( GetBaseDataDir() )
	    		.add( listbox_->getText() ).fullPath();
    delete survinfo_;
    survinfo_ = SurveyInfo::read( fname );
}


void uiSurvey::newSurvey()
{
    delete SurveyInfo::theinst_;
    SurveyInfo::theinst_ = 0;
}


void uiSurvey::getSurveyList( BufferStringSet& list )
{
    BufferString basedir = GetBaseDataDir();
    DirList dl( basedir, DirList::DirsOnly );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const char* dirnm = dl.get( idx );
	if ( matchString("_New_Survey_",dirnm) )
	    continue;

	FilePath fp( basedir );
	fp.add( dirnm ).add( ".survey" );
	BufferString survfnm = fp.fullPath();
	if ( File_exists(survfnm) )
	    list.add( dirnm );
    }

    list.sort();
}


bool uiSurvey::survTypeOKForUser( bool is2d )
{
    const bool dowarn = (is2d && !SI().has2D()) || (!is2d && !SI().has3D());
    if ( !dowarn ) return true;

    BufferString warnmsg( "Your survey is set up as '" );
    warnmsg += is2d ? "3-D only'.\nTo be able to actually use 2-D"
		    : "2-D only'.\nTo be able to actually use 3-D";
    warnmsg += " data\nyou will have to change the survey setup.";
    warnmsg += "\n\nDo you wish to continue?";

    return uiMSG().askGoOn( warnmsg );
}
