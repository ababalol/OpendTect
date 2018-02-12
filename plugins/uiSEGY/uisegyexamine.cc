/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uisegyexamine.h"
#include "uisegytrchdrvalplot.h"
#include "uitextedit.h"
#include "uitable.h"
#include "uisplitter.h"
#include "uilabel.h"
#include "uitoolbutton.h"
#include "uiflatviewer.h"
#include "uifileselector.h"
#include "uimsg.h"
#include "uiseistrcbufviewer.h"
#include "uistatsdisplaywin.h"
#include "filepath.h"
#include "ioobj.h"
#include "ptrman.h"
#include "msgh.h"
#include "seistrc.h"
#include "seisbufadapters.h"
#include "segytr.h"
#include "segyhdr.h"
#include "settings.h"
#include "iopar.h"
#include "timer.h"
#include "envvars.h"
#include "separstr.h"
#include "oscommand.h"
#include "od_strstream.h"
#include "oddirs.h"
#include "dbman.h"
#include "od_helpids.h"

const char* uiSEGYExamine::Setup::sKeyNrTrcs = "Examine.Number of traces";


uiSEGYExamine::Setup::Setup( int nrtraces )
    : uiDialog::Setup(tr("SEG-Y Examiner"),mNoDlgTitle,
                      mODHelpKey(mSEGYExamineHelpID) )
    , nrtrcs_(nrtraces<0?getDefNrTrcs():nrtraces)
    , fp_(true)
{
    nrstatusflds( 2 ).modal( false );
}


int uiSEGYExamine::Setup::getDefNrTrcs()
{
    const char* res = Settings::common().find( sKeySettNrTrcExamine );
    if ( !res )
	return 1000;
    return toInt( res );
}


void uiSEGYExamine::Setup::usePar( const IOPar& iop )
{
    fp_.usePar( iop ); fs_.usePar( iop );
    iop.get( sKeyNrTrcs, nrtrcs_ );
}


uiSEGYExamine::uiSEGYExamine( uiParent* p, const uiSEGYExamine::Setup& su )
	: uiDialog(p,su)
	, setup_(su)
	, tbuf_(*new SeisTrcBuf(true))
	, segytransl_(0)
{
    setCtrlStyle( CloseOnly );

    uiGroup* txtgrp = new uiGroup( this, "Txt fld group" );
    uiLabel* lbl = new uiLabel( txtgrp, tr("File header information") );
    uiToolButton* savesettb = new uiToolButton( txtgrp, "save",
					 tr("Save textual header to a file"),
				         mCB(this,uiSEGYExamine,saveHdr) );
    savesettb->attach( rightBorder );
    txtfld_ = new uiTextEdit( txtgrp, "", true );
    txtfld_->setPrefHeightInChar( 14 );
    txtfld_->setPrefWidthInChar( 80 );
    txtfld_->attach( ensureBelow, lbl );

    uiGroup* logrp = new uiGroup( this, "Low group" );
    uiGroup* tblgrp = new uiGroup( logrp, "Table group" );
    lbl = new uiLabel( tblgrp, tr("Trace header information") );
    uiToolButton* seistb = new uiToolButton( tblgrp, "vd", tr("Display traces"),
			     mCB(this,uiSEGYExamine,dispSeis) );

    uiTable::Setup tblsu( SEGY::TrcHeader::hdrDef().size(), setup_.nrtrcs_ );
    tblsu.rowdesc(tr("Header field")).coldesc(uiStrings::sTrace())
				     .selmode(uiTable::SingleRow);
    tbl_ = new uiTable( tblgrp, tblsu, "Trace info" );
    tbl_->setPrefHeightInChar( 14 );
    tbl_->setPrefWidthInChar( 40 );
    tbl_->attach( ensureBelow, lbl );
    for ( int icol=0; icol<setup_.nrtrcs_; icol++ )
    {
	const int tidx = icol + 1;
	uiString tt( tr("Trace header info from %1").arg(tidx) );
	tt.append(" %1 trace").arg(getRankPostFix(tidx));
	tbl_->setColumnLabel( icol, toUiString(tidx) );
	tbl_->setColumnToolTip( icol, tt );
	tbl_->setColumnReadOnly( icol, true );
    }
    tbl_->selectionChanged.notify( mCB(this,uiSEGYExamine,rowClck) );
    uiToolButton* histtb = new uiToolButton( tblgrp, "histogram",
			    tr("Show histogram of sample values"),
			    mCB(this,uiSEGYExamine,dispHist) );
    histtb->attach( rightAlignedAbove, tbl_ );
    seistb->attach( leftOf, histtb );

    hvaldisp_ = new uiSEGYTrcHdrValPlot( this, true );

    uiSplitter* hsplit = new uiSplitter( logrp, "VSplitter", OD::Vertical );
    hsplit->addGroup( tblgrp );
    hsplit->addGroup( hvaldisp_ );

    uiSplitter* vsplit = new uiSplitter( this, "HSplitter", OD::Horizontal );
    vsplit->addGroup( txtgrp );
    vsplit->addGroup( logrp );

    toStatusBar( toUiString(setup_.fs_.dispName()), 1 );
    outInfo( tr("Opening input") );
    segytransl_ = getReader( setup_, txtinfo_ );
    txtfld_->setText( txtinfo_ );

    uiString str( m3Dots(tr("Reading first %1 traces").arg(su.nrtrcs_)) );
    outInfo( str );

    afterPopup.notify( mCB(this,uiSEGYExamine,updateInput) );
}


uiSEGYExamine::~uiSEGYExamine()
{
    delete segytransl_;
    delete &tbuf_;
}


void uiSEGYExamine::rowClck( CallBacker* )
{
    setRow( tbl_->currentRow() );
}


void uiSEGYExamine::saveHdr( CallBacker* )
{
    if ( !segytransl_ )
	return;

    uiFileSelector::Setup fssu;
    fssu.setForWrite().initialselectiondir(
		File::Path(GetDataDir(),sSeismicSubDir()).fullPath() );
    uiFileSelector uifs( this, fssu );
    uifs.caption() = tr("Save SEG-Y Textual Header to");
    if ( !uifs.go() )
	return;

    od_ostream strm( uifs.fileName() );
    if ( !strm.isOK() )
	{ uiMSG().error(tr("Cannot open file for writing")); return; }

    const SEGY::TxtHeader& th = *segytransl_->txtHeader();
    BufferString buf; th.getText( buf );
    strm << buf << od_endl;
}


uiString uiSEGYExamine::sGetWinTitle()
{
    const BufferString fnm( File::Path(setup_.fs_.dispName()).fileName() );

    return tr("First %1 traces from %2").arg( tbuf_.size() ).arg( fnm );
}

void uiSEGYExamine::dispSeis( CallBacker* )
{
    uiSeisTrcBufViewer* vwr = new uiSeisTrcBufViewer( this,
				uiSeisTrcBufViewer::Setup(sGetWinTitle()) );
    vwr->selectDispTypes( true, true );
    vwr->setTrcBuf( tbuf_, Seis::Line, "SEG-Y.Examine", "sample value" );
    vwr->start(); vwr->handleBufChange();
}


void uiSEGYExamine::dispHist( CallBacker* )
{
    SeisTrcBufArray2D a2d( &tbuf_, 0 );
    uiStatsDisplay::Setup su; su.withname( false );
    uiStatsDisplayWin* mw = new uiStatsDisplayWin( this, su, 1, false );
    mw->statsDisplay(0)->setData( &a2d );
    mw->setDeleteOnClose( true );
    mw->setCaption( sGetWinTitle() );
    mw->show();
}


void uiSEGYExamine::updateInput( CallBacker* )
{
    display( true );
    updateInp();
    setName( setup_.fs_.dispName() );
}


void uiSEGYExamine::setRow( int irow )
{
    if ( irow < 0 ) return;

    const int nrcols = tbl_->nrCols();
    TypeSet<float> data;
    for ( int icol=0; icol<nrcols; icol++ )
    {
	const char* txt = tbl_->text( RowCol(irow,icol) );
	if ( !txt || !*txt )
	    break;
	data += Conv::to<float>( txt );
    }

    hvaldisp_->setData( *SEGY::TrcHeader::hdrDef()[irow],
			data.arr(), data.size() );
}


SEGYSeisTrcTranslator* uiSEGYExamine::getReader(
			const uiSEGYExamine::Setup& su, uiString& emsg )
{
    SEGYSeisTrcTranslator* segytr = 0;

    if ( su.fs_.isEmpty() )
	{ emsg = tr( "No input file specified" ); return 0; }
    PtrMan<IOObj> ioobj = su.fs_.getIOObj( true );
    if ( !ioobj )
	{ emsg = uiStrings::phrInternalError("Cannot create DB object");
		return 0; }

    DBM().setEntry( *ioobj );
    su.fp_.fillPar( ioobj->pars() );

    segytr = static_cast<SEGYSeisTrcTranslator*>( ioobj->createTranslator() );
    if ( !segytr )
	{ emsg = uiStrings::phrInternalError("Cannot create Translator");
		return 0; }

    Conn* conn = ioobj->getConn( true );
    if ( !conn || conn->isBad() )
	{ emsg = uiStrings::phrCannotOpen( toUiString(ioobj->mainFileName()) );
		return 0; }

    if ( !segytr->initRead(conn,Seis::PreScan) )
	{ emsg = uiStrings::phrCannotRead( toUiString(ioobj->mainFileName()) );
		return 0; }

    return segytr;
}


int uiSEGYExamine::getRev() const
{
    return getRev( segytransl_ );
}


int uiSEGYExamine::getRev( const SEGYSeisTrcTranslator* transl )
{
    if ( !transl )
	return -1;
    return transl->isRev0() ? 0 : 1;
}


int uiSEGYExamine::getRev( const uiSEGYExamine::Setup& su, uiString& emsg )
{
    PtrMan<SEGYSeisTrcTranslator> rdr = getReader( su, emsg );
    if ( !rdr && emsg.isEmpty() )
	emsg = tr( "Error opening file."
	    "\nPlease check whether the file size is at least 3600 bytes." );
    return getRev( rdr );
}


bool uiSEGYExamine::launch( const uiSEGYExamine::Setup& su )
{
    BufferString cmd( "od_SEGYExaminer --nrtrcs " );
    cmd += su.nrtrcs_;
    if ( su.fp_.ns_ > 0 )
	{ cmd += " --ns "; cmd += su.fp_.ns_; }
    if ( su.fp_.fmt_ > 0 )
	{ cmd += " --fmt "; cmd += su.fp_.fmt_; }
    if ( su.fp_.byteswap_ )
	{ cmd += " --swapbytes "; cmd += su.fp_.byteswap_; }
    if ( su.fs_.isMulti() )
    {
	FileMultiString fms;
	fms += su.fs_.nrs_.start;
	fms += su.fs_.nrs_.stop;
	fms += su.fs_.nrs_.step;
	if ( su.fs_.zeropad_ > 1 )
	    fms += su.fs_.zeropad_;
	cmd += " --filenrs '"; cmd += fms; cmd += "'";
    }

    BufferString fnm( su.fs_.fileName() );
    fnm.replace( "*", "+x+" );
    return ExecODProgram( cmd, fnm );
}


void uiSEGYExamine::updateInp()
{
    if ( !segytransl_ || !tbuf_.isEmpty() )
	return;

    const SEGY::HdrDef& hdef = SEGY::TrcHeader::hdrDef();
    const int nrvals = hdef.size();
    const SEGY::TrcHeader& trhead = segytransl_->trcHeader();

    SeisTrc trc; int nrdone = 0;
    bool stoppedatend = false;
    for ( int itrc=0; itrc<setup_.nrtrcs_; itrc++ )
    {
	if ( !segytransl_->read(trc) )
	    stoppedatend = true;
	if ( nrdone == 0 )
	    handleFirstTrace( trc );
	if ( stoppedatend )
	    break;

	for ( int ival=0; ival<nrvals; ival++ )
	{
	    int val = trhead.entryVal( ival );
	    RowCol rc( ival, itrc );
	    tbl_->setValue( rc, val );
	}

	nrdone++;
	trc.info().setTrcNr( nrdone );
	tbuf_.add( new SeisTrc(trc) );
    }
    tbl_->setNrCols( nrdone > 0 ? nrdone : 1 );

    if ( stoppedatend || nrdone < 1 )
    {
	uiString str( toUiString("\n\n----  ") );
	const bool ismulti = !mIsUdf(setup_.fs_.nrs_.start);
	if ( nrdone < 1 )
	    str.append(tr(" No traces found"));
	else
	{
	    str.append(tr(" Total number of traces present in "));
	    str.append(tr("file",0,ismulti ? mPlural : -1));
	    str.append(toUiString(": ")); str.append(toUiString(nrdone));
	}
	str.append(toUiString("  ----"));
	txtinfo_.append(" ").append(str);
    }
    outInfo( uiString::emptyString() );
    txtfld_->setText( txtinfo_ );
}


void uiSEGYExamine::handleFirstTrace( const SeisTrc& trc )
{
    const SEGY::TxtHeader& txthead = *segytransl_->txtHeader();
    const SEGY::BinHeader& binhead = segytransl_->binHeader();
    od_ostrstream thstrm, bhstrm;
    txthead.dump( thstrm );
    binhead.dump( bhstrm );

    txtinfo_ = toUiString(thstrm.result());
    txtinfo_.append(tr("\n------\n\n"
		"Binary header info (non-zero values displayed only):\n\n"));
    txtinfo_.append(bhstrm.result());

    const SEGY::HdrDef& hdef = SEGY::TrcHeader::hdrDef();
    const int nrvals = hdef.size();
    for ( int ival=0; ival<nrvals; ival++ )
    {
	const SEGY::HdrEntry& he( *hdef[ival] );
	uiString rownm = toUiString("%1 [%2]").arg(((int)he.bytepos_)+1)
							    .arg(he.name());
	tbl_->setRowLabel( ival, rownm );
	tbl_->setRowToolTip( ival, mToUiStringTodo(he.description()) );
    }

    tbl_->resizeRowsToContents();
    tbl_->resizeHeaderToContents( false );
}


bool uiSEGYExamine::rejectOK()
{
    return true;
}


void uiSEGYExamine::outInfo( const uiString txt )
{
    toStatusBar( txt, 0 );}
