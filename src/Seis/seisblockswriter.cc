/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2017
________________________________________________________________________

-*/

#include "seisblockswriter.h"
#include "seismemblocks.h"
#include "seistrc.h"
#include "seisbuf.h"
#include "posidxpairdataset.h"
#include "paralleltask.h"
#include "executor.h"
#include "uistrings.h"
#include "scaler.h"
#include "datachar.h"
#include "file.h"
#include "keystrs.h"
#include "posinfo.h"
#include "survgeom3d.h"
#include "survinfo.h" // for survey name and zInFeet
#include "ascstream.h"
#include "separstr.h"
#include "arrayndimpl.h"
#include "odmemory.h"


namespace Seis
{
namespace Blocks
{

class StepFinder
{
public:

		StepFinder(Writer&);

    bool	isFinished() const	{ return tbuf_.isEmpty(); }
    void	finish(uiRetVal&);
    void	addTrace(const SeisTrc&,uiRetVal&);

    Writer&		wrr_;
    Pos::IdxPairDataSet positions_;
    SeisTrcBuf		tbuf_;

};

} // namespace Blocks

} // namespace Seis


Seis::Blocks::StepFinder::StepFinder( Writer& wrr )
    : wrr_(wrr)
    , positions_(0,false)
    , tbuf_(false)
{
}


// Algo: first collect at least 2000 traces. Once 3 inls and 3 crls found,
// finish the procedure.

void Seis::Blocks::StepFinder::addTrace( const SeisTrc& trc, uiRetVal& uirv )
{
    const BinID bid( trc.info().binID() );
    if ( positions_.includes(bid) )
	return;
    tbuf_.add( new SeisTrc(trc) );
    positions_.add( bid );
    if ( positions_.totalSize() % 2000 )
	return;
    const int nrinls = positions_.nrFirst();
    if ( nrinls < 3 )
	return;

    bool found3crls = false;
    for ( int iinl=0; iinl<nrinls; iinl++ )
    {
	if ( positions_.nrSecondAtIdx(iinl) > 2 )
	    { found3crls = true; break; }
    }
    if ( !found3crls )
	return;

    finish( uirv );
}


// Algo: if enough inls/crls found - or at the end, finish() will be called.
// If at least one step was seen, the default (SI or provided) is replaced.
// Now add the collected traces to the writer so it can start off making
// the appropriate blocks.
// The writer will know the detection is finished because the tbuf is empty.

void Seis::Blocks::StepFinder::finish( uiRetVal& uirv )
{
    Pos::IdxPairDataSet::SPos spos;
    int inlstate = -1, crlstate = -1;
	// initialising these four because of alarmist code analysers
    int previnl = -1, prevcrl = -1;
    int inlstep = 1, crlstep = 1;
    while ( positions_.next(spos) )
    {
	const BinID bid( positions_.getIdxPair(spos) );
	if ( inlstate < 0 )
	{
	    previnl = bid.inl();
	    prevcrl = bid.crl();
	    inlstate = crlstate = 0;
	}
	const int inldiff = bid.inl() - previnl;
	if ( inldiff )
	{
	    previnl = bid.inl();
	    if ( inlstate < 1 )
	    {
		inlstep = inldiff;
		inlstate = 1;
	    }
	    else if ( inldiff < inlstep )
		inlstep = inldiff;
	    continue;
	}
	const int crldiff = bid.crl() - prevcrl;
	if ( crldiff < 1 )
	    continue;
	prevcrl = bid.crl();
	if ( crlstate < 1 )
	{
	    crlstep = crldiff;
	    crlstate = 1;
	}
	else if ( crldiff < crlstep )
	    crlstep = crldiff;
    }

    if ( inlstate > 0 )
	wrr_.hgeom_.sampling().hsamp_.step_.inl() = inlstep;
    if ( crlstate > 0 )
	wrr_.hgeom_.sampling().hsamp_.step_.crl() = crlstep;

    for ( int idx=0; idx<tbuf_.size(); idx++ )
    {
	SeisTrc* trc = tbuf_.get( idx );
	wrr_.doAdd( *trc, uirv );
	if ( uirv.isError() )
	    break;
    }

    tbuf_.deepErase(); // finished!
}



Seis::Blocks::Writer::Writer( const HGeom* geom )
    : specifiedfprep_(OD::AutoFPRep)
    , nrcomps_(1)
    , isfinished_(false)
    , stepfinder_(0)
    , interp_(new DataInterp(DataCharacteristics()))
    , strm_(0)
{
    if ( geom )
	hgeom_ = *geom;
    else
	hgeom_ = static_cast<const HGeom&>( HGeom::default3D() );
    zgeom_ = hgeom_.sampling().zsamp_;
}


Seis::Blocks::Writer::~Writer()
{
    if ( !isfinished_ )
    {
	Task* task = finisher();
	if ( task )
	{
	    task->execute();
	    delete task;
	}
    }

    delete strm_;
    deepErase( zevalpositions_ );
    delete interp_;
    delete stepfinder_;
}


void Seis::Blocks::Writer::setEmpty()
{
    clearColumns();
    deepErase( zevalpositions_ );
}


void Seis::Blocks::Writer::setBasePath( const File::Path& fp )
{
    if ( fp.fullPath() != basepath_.fullPath() )
    {
	basepath_ = fp;
	needreset_ = true;
    }
}


void Seis::Blocks::Writer::setFullPath( const char* nm )
{
    File::Path fp( nm );
    fp.setExtension( 0 );
    setBasePath( fp );
}


void Seis::Blocks::Writer::setFileNameBase( const char* nm )
{
    File::Path fp( basepath_ );
    fp.setFileName( nm );
    setBasePath( fp );
}


void Seis::Blocks::Writer::setFPRep( OD::FPDataRepType rep )
{
    if ( specifiedfprep_ != rep )
    {
	specifiedfprep_ = rep;
	needreset_ = true;
    }
}


void Seis::Blocks::Writer::setCubeName( const char* nm )
{
    cubename_ = nm;
}


void Seis::Blocks::Writer::setZDomain( const ZDomain::Def& def )
{
    hgeom_.setZDomain( def );
}


void Seis::Blocks::Writer::addComponentName( const char* nm )
{
    compnms_.add( nm );
}


void Seis::Blocks::Writer::addAuxInfo( const char* key, const IOPar& iop )
{
    BufferString auxkey( key );
    if ( auxkey.isEmpty() )
	{ pErrMsg( "Refusing to add section without key" ); return; }

    IOPar* toadd = new IOPar( iop );
    toadd->setName( BufferString(sKeySectionPre(),auxkey) );
    auxiops_ += toadd;
}


void Seis::Blocks::Writer::setScaler( const LinScaler* newscaler )
{
    if ( (!scaler_ && !newscaler) )
	return;

    delete scaler_;
    scaler_ = newscaler ? newscaler->clone() : 0;
    needreset_ = true;
}


void Seis::Blocks::Writer::resetZ()
{
    const float eps = Seis::cDefSampleSnapDist();
    deepErase( zevalpositions_ );
    const int nrglobzidxs = Block::globIdx4Z(zgeom_,zgeom_.stop,dims_.z()) + 1;

    for ( IdxType globzidx=0; globzidx<nrglobzidxs; globzidx++ )
    {
	ZEvalPosSet* posset = new ZEvalPosSet;
	for ( IdxType loczidx=0; loczidx<dims_.z(); loczidx++ )
	{
	    const float z = Block::z4Idxs( zgeom_, dims_.z(),
					   globzidx, loczidx );
	    if ( z > zgeom_.start-eps && z < zgeom_.stop+eps )
		*posset += ZEvalPos( loczidx, z );
	}
	if ( posset->isEmpty() )
	    delete posset;
	else
	    zevalpositions_ += posset;
    }

    if ( zevalpositions_.isEmpty() )
    {
	pErrMsg("Huh");
	ZEvalPosSet* posset = new ZEvalPosSet;
	*posset += ZEvalPos( 0, zgeom_.start );
	 zevalpositions_ += posset;
    }
}


uiRetVal Seis::Blocks::Writer::add( const SeisTrc& trc )
{
    uiRetVal uirv;
    Threads::Locker locker( accesslock_ );

    if ( needreset_ )
    {
	needreset_ = false;
	isfinished_ = false;
	setEmpty();
	if ( trc.isEmpty() )
	{
	    uirv.add( tr("No data in input trace") );
	    return uirv;
	}

	fprep_ = specifiedfprep_;
	if ( fprep_ == OD::AutoFPRep )
	    fprep_ = trc.data().getInterpreter()->dataChar().userType();
	delete interp_;
	interp_ = DataInterp::create( DataCharacteristics(fprep_), true );

	zgeom_.start = trc.startPos();
	zgeom_.stop = trc.endPos();
	zgeom_.step = trc.stepPos();
	nrcomps_ = trc.nrComponents();
	resetZ();

	delete stepfinder_;
	stepfinder_ = new StepFinder( *const_cast<Writer*>(this) );
    }

    if ( stepfinder_ )
    {
	stepfinder_->addTrace( trc, uirv );
	if ( stepfinder_->isFinished() )
	    { delete stepfinder_; stepfinder_ = 0; }
	return uirv;
    }

    doAdd( trc, uirv );
    return uirv;
}


void Seis::Blocks::Writer::doAdd( const SeisTrc& trc, uiRetVal& uirv )
{
    const BinID bid = trc.info().binID();
    const HGlobIdx globidx( Block::globIdx4Inl(hgeom_,bid.inl(),dims_.inl()),
			    Block::globIdx4Crl(hgeom_,bid.crl(),dims_.crl()) );

    MemBlockColumn* column = getColumn( globidx );
    if ( !column )
    {
	uirv.set( tr("Memory needed for writing process unavailable.") );
	setEmpty();
	return;
    }
    else if ( isCompleted(*column) )
	return; // this check is absolutely necessary to for MT writing

    const HLocIdx locidx( Block::locIdx4Inl(hgeom_,bid.inl(),dims_.inl()),
			  Block::locIdx4Crl(hgeom_,bid.crl(),dims_.crl()) );

    for ( int icomp=0; icomp<nrcomps_; icomp++ )
    {
	MemBlockColumn::BlockSet& blockset = *column->blocksets_[icomp];
	for ( int iblock=0; iblock<blockset.size(); iblock++ )
	{
	    const ZEvalPosSet& posset = *zevalpositions_[iblock];
	    MemBlock& block = *blockset[iblock];
	    add2Block( block, posset, locidx, trc, icomp );
	}
    }

    bool& visited = column->visited_[locidx.inl()][locidx.crl()];
    if ( !visited )
    {
	column->nruniquevisits_++;
	visited = true;
    }

    if ( isCompleted(*column) )
	writeColumn( *column, uirv );
}


void Seis::Blocks::Writer::add2Block( MemBlock& block,
			const ZEvalPosSet& zevals, const HLocIdx& hlocidx,
			const SeisTrc& trc, int icomp )
{
    if ( block.isRetired() )
	return; // new visit of already written. Won't do, but no error.

    LocIdx locidx( hlocidx.inl(), hlocidx.crl(), 0 );
    for ( int idx=0; idx<zevals.size(); idx++ )
    {
	const ZEvalPos& evalpos = zevals[idx];
	locidx.z() = evalpos.first;
	float val2set = trc.getValue( evalpos.second, icomp );
	if ( scaler_ )
	    val2set = (float)scaler_->scale( val2set );
	block.setValue( locidx, val2set );
    }
}


Seis::Blocks::MemBlockColumn*
Seis::Blocks::Writer::mkNewColumn( const HGlobIdx& hglobidx )
{
    MemBlockColumn* column = new MemBlockColumn( hglobidx, dims_, nrcomps_ );

    const int nrglobzidxs = zevalpositions_.size();
    for ( IdxType globzidx=0; globzidx<nrglobzidxs; globzidx++ )
    {
	const ZEvalPosSet& evalposs = *zevalpositions_[globzidx];
	GlobIdx globidx( hglobidx.inl(), hglobidx.crl(), globzidx );
	Dimensions dims( dims_ );
	if ( globzidx == nrglobzidxs-1 )
	    dims.z() = (SzType)evalposs.size();

	for ( int icomp=0; icomp<nrcomps_; icomp++ )
	{
	    MemBlock* block = new MemBlock( globidx, dims, *interp_ );
	    if ( block->isRetired() ) // ouch
		{ delete column; return 0; }
	    block->zero();
	    *column->blocksets_[icomp] += block;
	}
    }

    return column;
}


Seis::Blocks::MemBlockColumn*
Seis::Blocks::Writer::getColumn( const HGlobIdx& globidx )
{
    Column* column = findColumn( globidx );
    if ( !column )
    {
	column = mkNewColumn( globidx );
	addColumn( column );
    }
    return (MemBlockColumn*)column;
}


bool Seis::Blocks::Writer::isCompleted( const MemBlockColumn& column ) const
{
    return column.nruniquevisits_ == ((int)dims_.inl()) * dims_.crl();
}


namespace Seis
{
namespace Blocks
{

class ColumnWriter : public Executor
{ mODTextTranslationClass(Seis::Blocks::ColumnWriter)
public:

ColumnWriter( Writer& wrr, MemBlockColumn& colmn )
    : Executor("Column Writer")
    , wrr_(wrr)
    , column_(colmn)
    , strm_(*wrr.strm_)
    , nrblocks_(wrr_.nrColumnBlocks())
    , iblock_(0)
{
    if ( strm_.isBad() )
	setErr( true );
    else
    {
	column_.fileoffset_ = strm_.position();
	column_.getDefArea( start_, dims_ );
	if ( !wrr_.writeColumnHeader(column_,start_,dims_) )
	    setErr();
    }
}

virtual od_int64 nrDone() const { return iblock_; }
virtual od_int64 totalNr() const { return nrblocks_; }
virtual uiString nrDoneText() const { return tr("Blocks written"); }
virtual uiString message() const
{
    if ( uirv_.isError() )
	return uirv_;
    return tr("Writing Traces");
}


int finish()
{
    column_.retire();
    return uirv_.isError() ? ErrorOccurred() : Finished();
}

void setErr( bool initial=false )
{
    uiString uifnm( toUiString(strm_.fileName()) );
    uirv_.set( initial	? uiStrings::phrCannotOpen(uifnm)
			: uiStrings::phrCannotWrite(uifnm) );
    strm_.addErrMsgTo( uirv_ );
}

virtual int nextStep()
{
    if ( uirv_.isError() || iblock_ >= nrblocks_ )
	return finish();

    for ( int icomp=0; icomp<wrr_.nrcomps_; icomp++ )
    {
	MemBlockColumn::BlockSet& blockset = *column_.blocksets_[icomp];
	MemBlock& block = *blockset[iblock_];
	if ( !wrr_.writeBlock( block, start_, dims_ ) )
	    { setErr(); return finish(); }
    }

    iblock_++;
    return MoreToDo();
}

    Writer&		wrr_;
    od_ostream&		strm_;
    MemBlockColumn&	column_;
    HDimensions		dims_;
    HLocIdx		start_;
    const int		nrblocks_;
    int			iblock_;
    uiRetVal		uirv_;

};

} // namespace Blocks
} // namespace Seis


void Seis::Blocks::Writer::writeColumn( MemBlockColumn& column, uiRetVal& uirv )
{
    if ( !strm_ )
	strm_ = new od_ostream( dataFileName() );
    ColumnWriter wrr( *this, column );
    if ( !wrr.execute() )
	uirv = wrr.uirv_;
}


bool Seis::Blocks::Writer::writeColumnHeader( const MemBlockColumn& column,
		    const HLocIdx& start, const HDimensions& dims ) const
{

    const SzType hdrsz = columnHeaderSize( version_ );
    const od_stream_Pos orgstrmpos = strm_->position();

    strm_->addBin( hdrsz );
    strm_->addBin( dims.first ).addBin( dims.second ).addBin( dims_.third );
    strm_->addBin( start.first ).addBin( start.second );
    strm_->addBin( column.globIdx().first ).addBin( column.globIdx().second );

    const int bytes_left_in_hdr = hdrsz - (int)(strm_->position()-orgstrmpos);
    char* buf = new char [bytes_left_in_hdr];
    OD::memZero( buf, bytes_left_in_hdr );
    strm_->addBin( buf, bytes_left_in_hdr );
    delete [] buf;

    return strm_->isOK();
}


bool Seis::Blocks::Writer::writeBlock( MemBlock& block, HLocIdx wrstart,
				       HDimensions wrhdims )
{
    const DataBuffer& dbuf = block.dbuf_;
    const Dimensions& blockdims = block.dims();
    const Dimensions wrdims( wrhdims.inl(), wrhdims.crl(), blockdims.z() );

    if ( wrdims == blockdims )
	strm_->addBin( dbuf.data(), dbuf.totalBytes() );
    else
    {
	const DataBuffer::buf_type* bufdata = dbuf.data();
	const int bytespersample = dbuf.bytesPerElement();
	const int bytesperentirecrl = bytespersample * blockdims.z();
	const int bytesperentireinl = bytesperentirecrl * blockdims.crl();

	const int bytes2write = wrdims.z() * bytespersample;
	const IdxType wrstopinl = wrstart.inl() + wrdims.inl();
	const IdxType wrstopcrl = wrstart.crl() + wrdims.crl();

	const DataBuffer::buf_type* dataptr;
	for ( IdxType iinl=wrstart.inl(); iinl<wrstopinl; iinl++ )
	{
	    dataptr = bufdata + iinl * bytesperentireinl
			      + wrstart.crl() * bytesperentirecrl;
	    for ( IdxType icrl=wrstart.crl(); icrl<wrstopcrl; icrl++ )
	    {
		strm_->addBin( dataptr, bytes2write );
		dataptr += bytesperentirecrl;
	    }
	}
    }

    return strm_->isOK();
}


void Seis::Blocks::Writer::writeInfoFiles( uiRetVal& uirv )
{
    const BufferString ovvwfnm( overviewFileName() );
    if ( File::exists(ovvwfnm) )
	File::remove( ovvwfnm );

    od_ostream infostrm( infoFileName() );
    if ( infostrm.isBad() )
    {
	uirv.add( uiStrings::phrCannotOpen( toUiString(infostrm.fileName()) ) );
	return;
    }

    if ( !writeInfoFileData(infostrm) )
    {
	uirv.add( uiStrings::phrCannotWrite( toUiString(infostrm.fileName()) ));
	return;
    }

    od_ostream ovvwstrm( ovvwfnm );
    if ( ovvwstrm.isBad() )
    {
	ErrMsg( uiStrings::phrCannotOpen(toUiString(ovvwfnm)).getFullString() );
	return;
    }
    if ( !writeOverviewFileData(ovvwstrm) )
	File::remove( ovvwfnm );
}


bool Seis::Blocks::Writer::writeInfoFileData( od_ostream& strm )
{
    ascostream ascostrm( strm );
    if ( !ascostrm.putHeader(sKeyFileType()) )
	return false;

    PosInfo::CubeData cubedata;
    Interval<IdxType> globinlidxrg, globcrlidxrg;
    Interval<double> xrg, yrg;
    scanPositions( cubedata, globinlidxrg, globcrlidxrg, xrg, yrg );

    IOPar iop( sKeyGenSection() );
    iop.set( sKeyFmtVersion(), version_ );
    iop.set( sKeySurveyName(), SI().name() );
    iop.set( sKeyCubeName(), cubename_ );
    DataCharacteristics::putUserTypeToPar( iop, fprep_ );
    if ( scaler_ )
    {
	// write the scaler needed to reconstruct the org values
	LinScaler* invscaler = scaler_->inverse();
	invscaler->put( iop );
	delete invscaler;
    }
    iop.set( sKey::XRange(), xrg );
    iop.set( sKey::YRange(), yrg );
    iop.set( sKey::InlRange(), finalinlrg_ );
    iop.set( sKey::CrlRange(), finalcrlrg_ );
    iop.set( sKey::ZRange(), zgeom_ );
    hgeom_.zDomain().set( iop );
    if ( hgeom_.zDomain().isDepth() && SI().zInFeet() )
	iop.setYN( sKeyDepthInFeet(), true );

    FileMultiString fms;
    for ( int icomp=0; icomp<nrcomps_; icomp++ )
    {
	BufferString nm;
	if ( icomp < compnms_.size() )
	    nm = compnms_.get( icomp );
	else
	    nm.set( "Component " ).add( icomp+1 );
	fms += nm;
    }
    iop.set( sKeyComponents(), fms );
    if ( datatype_ != UnknownData )
	iop.set( sKeyDataType(), nameOf(datatype_) );

    hgeom_.putMapInfo( iop );
    iop.set( sKeyDimensions(), dims_.inl(), dims_.crl(), dims_.z() );
    iop.set( sKeyGlobInlRg(), globinlidxrg );
    iop.set( sKeyGlobCrlRg(), globcrlidxrg );

    iop.putTo( ascostrm );
    if ( !strm.isOK() )
	return false;

    Pos::IdxPairDataSet::SPos spos;
    IOPar offsiop( sKeyOffSection() );
    while ( columns_.next(spos) )
    {
	const MemBlockColumn& column = *(MemBlockColumn*)columns_.getObj(spos);
	if ( column.nruniquevisits_ > 0 )
	{
	    const HGlobIdx& gidx = column.globIdx();
	    BufferString ky;
	    ky.add( gidx.inl() ).add( '.' ).add( gidx.crl() );
	    offsiop.add( ky, column.fileoffset_ );
	}
    }
    offsiop.putTo( ascostrm );
    if ( !strm.isOK() )
	return false;

    for ( int idx=0; idx<auxiops_.size(); idx++ )
    {
	auxiops_[idx]->putTo( ascostrm );
	if ( !strm.isOK() )
	    return false;
    }

    strm << sKeyPosSection() << od_endl;
    return cubedata.write( strm, true );
}


#define mGetNrInlCrlAndRg() \
    const int stepinl = hgeom_.sampling().hsamp_.step_.inl(); \
    const int stepcrl = hgeom_.sampling().hsamp_.step_.crl(); \
    const StepInterval<int> inlrg( finalinlrg_.start, finalinlrg_.stop, \
				   stepinl ); \
    const StepInterval<int> crlrg( finalcrlrg_.start, finalcrlrg_.stop, \
				   stepcrl ); \
    const int nrinl = inlrg.nrSteps() + 1; \
    const int nrcrl = crlrg.nrSteps() + 1

bool Seis::Blocks::Writer::writeOverviewFileData( od_ostream& strm )
{
    ascostream astrm( strm );
    if ( !astrm.putHeader("Cube Overview") )
	return false;

    mGetNrInlCrlAndRg();

    Array2DImpl<float> data( nrinl, nrcrl );
    MemSetter<float> memsetter( data.getData(), mUdf(float),
			 (od_int64)data.info().getTotalSz() );
    memsetter.execute();

    Pos::IdxPairDataSet::SPos spos;
    while ( columns_.next(spos) )
    {
	const MemBlockColumn& column = *(MemBlockColumn*)columns_.getObj(spos);
	const Dimensions bdims( column.dims() );
	const BinID start(
	    Block::startInl4GlobIdx(hgeom_,column.globIdx().inl(),bdims.inl()),
	    Block::startCrl4GlobIdx(hgeom_,column.globIdx().crl(),bdims.crl()));
	const MemColumnSummary& summary = column.summary_;

	for ( IdxType iinl=0; iinl<bdims.inl(); iinl++ )
	{
	    const int inl = start.inl() + inlrg.step * iinl;
	    const int ia2dinl = inlrg.nearestIndex( inl );
	    if ( ia2dinl < 0 || ia2dinl >= nrinl )
		continue;
	    for ( IdxType icrl=0; icrl<bdims.crl(); icrl++ )
	    {
		const int crl = start.crl() + crlrg.step * icrl;
		const int ia2dcrl = crlrg.nearestIndex( crl );
		if ( ia2dcrl < 0 || ia2dcrl >= nrcrl )
		    continue;
		data.set( ia2dinl, ia2dcrl, summary.vals_[iinl][icrl] );
	    }
	}
    }

    return writeFullSummary( astrm, data );
}


bool Seis::Blocks::Writer::writeFullSummary( ascostream& astrm,
				const Array2D<float>& data ) const
{
    mGetNrInlCrlAndRg();
    const Coord origin(
	    hgeom_.transform(BinID(finalinlrg_.start,finalcrlrg_.start)) );
    const Coord endinl0(
	    hgeom_.transform(BinID(finalinlrg_.start,finalcrlrg_.stop)) );
    const Coord endcrl0(
	    hgeom_.transform(BinID(finalinlrg_.stop,finalcrlrg_.start)) );
    const double fullinldist = origin.distTo<double>( endinl0 );
    const double fullcrldist = origin.distTo<double>( endcrl0 );

    IOPar iop;
    iop.set( sKey::InlRange(), inlrg );
    iop.set( sKey::CrlRange(), crlrg );
    hgeom_.putMapInfo( iop );
    iop.putTo( astrm, false );

    const int smallestdim = nrinl > nrcrl ? nrcrl : nrinl;
    TypeSet<int_pair> levelnrblocks;
    int nodesperblock = smallestdim / 2;
    int nrlevels = 0;
    while ( nodesperblock > 1 )
    {
	const float finlnrnodes = ((float)nrinl) / nodesperblock;
	const float fcrlnrnodes = ((float)nrcrl) / nodesperblock;
	const int_pair nrblocks( mNINT32(finlnrnodes), mNINT32(fcrlnrnodes) );
	levelnrblocks += nrblocks;
	const Coord blockdist( fullcrldist/nrblocks.first,
				fullinldist/nrblocks.second );
	const double avglvlblcksz = (blockdist.x_ + blockdist.y_) * .5;
	astrm.put( IOPar::compKey(sKey::Level(),nrlevels), avglvlblcksz );
	nodesperblock /= 2; nrlevels++;
    }
    astrm.newParagraph();

    for ( int ilvl=0; ilvl<nrlevels; ilvl++ )
    {
	writeLevelSummary( astrm.stream(), data, levelnrblocks[ilvl] );
	astrm.newParagraph();
	if ( !astrm.isOK() )
	    return false;
    }

    return true;
}


void Seis::Blocks::Writer::writeLevelSummary( od_ostream& strm,
				const Array2D<float>& data,
				int_pair nrblocks ) const
{
    mGetNrInlCrlAndRg();
    const float_pair fcellsz( ((float)nrinl) / nrblocks.first,
			     ((float)nrcrl) / nrblocks.second );
    const float_pair fhcellsz( fcellsz.first * .5f, fcellsz.second * .5f );
    const int_pair cellsz( mNINT32(fcellsz.first), mNINT32(fcellsz.second) );
    const int_pair hcellsz( mNINT32(fhcellsz.first), mNINT32(fhcellsz.second) );

    for ( int cix=0; ; cix++ )
    {
	const float fcenterx = fhcellsz.first + cix * fcellsz.first;
	int_pair center( mNINT32(fcenterx), 0 );
	if ( center.first >= nrinl )
	    break;

	for ( int ciy=0; ; ciy++ )
	{
	    const float fcentery = fhcellsz.second + ciy * fcellsz.second;
	    center.second = mNINT32( fcentery );
	    if ( center.second > nrcrl )
		break;

	    float sumv = 0.f; int nrv = 0;
	    for ( int ix=center.first-hcellsz.first;
		      ix<=center.first+hcellsz.first; ix++ )
	    {
		if ( ix < 0 )
		    continue;
		if ( ix >= nrinl )
		    break;
		for ( int iy=center.second-hcellsz.second;
			  iy<=center.second+hcellsz.second; iy++ )
		{
		    if ( iy < 0 )
			continue;
		    if ( iy >= nrcrl )
			break;
		    const float v = data.get( ix, iy );
		    if ( !mIsUdf(v) )
			{ sumv += v; nrv++; }
		}
	    }
	    if ( nrv > 0 )
	    {
		const BinID cbid( finalinlrg_.atIndex(center.first,stepinl),
				  finalcrlrg_.atIndex(center.second,stepcrl) );
		const Coord centercoord( hgeom_.transform(cbid) );
		strm << centercoord.x_ << '\t' << centercoord.y_ << '\t'
				<< sumv / nrv << '\n';
	    }
	}
    }
}


void Seis::Blocks::Writer::scanPositions( PosInfo::CubeData& cubedata,
	Interval<IdxType>& globinlrg, Interval<IdxType>& globcrlrg,
	Interval<double>& xrg, Interval<double>& yrg )
{
    Pos::IdxPairDataSet sortedpositions( 0, false );
    Pos::IdxPairDataSet::SPos spos;
    bool first = true;
    while ( columns_.next(spos) )
    {
	const MemBlockColumn& column = *(MemBlockColumn*)columns_.getObj(spos);
	if ( column.nruniquevisits_ < 1 )
	    continue;

	const HGlobIdx& globidx = column.globIdx();
	if ( first )
	{
	    globinlrg.start = globinlrg.stop = globidx.inl();
	    globcrlrg.start = globcrlrg.stop = globidx.crl();
	}
	else
	{
	    globinlrg.include( globidx.inl(), false );
	    globcrlrg.include( globidx.crl(), false );
	}

	for ( IdxType iinl=0; iinl<dims_.inl(); iinl++ )
	{
	    for ( IdxType icrl=0; icrl<dims_.crl(); icrl++ )
	    {
		if ( !column.visited_[iinl][icrl] )
		    continue;
		const int inl = Block::inl4Idxs( hgeom_, dims_.inl(),
						 globidx.inl(), iinl );
		const int crl = Block::crl4Idxs( hgeom_, dims_.crl(),
						 globidx.crl(), icrl );
		const Coord coord = hgeom_.toCoord( inl, crl );
		if ( first )
		{
		    finalinlrg_.start = finalinlrg_.stop = inl;
		    finalcrlrg_.start = finalcrlrg_.stop = crl;
		    xrg.start = xrg.stop = coord.x_;
		    yrg.start = yrg.stop = coord.y_;
		}
		else
		{
		    finalinlrg_.include( inl, false );
		    finalcrlrg_.include( crl, false );
		    xrg.include( coord.x_, false );
		    yrg.include( coord.y_, false );
		}

		sortedpositions.add( BinID(inl,crl) );
	    }
	}

	first = false;
    }

    PosInfo::CubeDataFiller cdf( cubedata );
    spos.reset();
    while ( sortedpositions.next(spos) )
    {
	const Pos::IdxPair ip( sortedpositions.getIdxPair( spos ) );
	cdf.add( BinID(ip.inl(),ip.crl()) );
    }
}


namespace Seis
{
namespace Blocks
{

class WriterFinisher : public Executor
{ mODTextTranslationClass(Seis::Blocks::WriterFinisher)
public:

WriterFinisher( Writer& wrr )
    : Executor("Seis Blocks Writer finisher")
    , wrr_(wrr)
    , colidx_(0)
{
    if ( wrr_.stepfinder_ )
    {
	wrr_.stepfinder_->finish( uirv_ );
	if ( uirv_.isError() )
	    return;
    }

    Pos::IdxPairDataSet::SPos spos;
    while ( wrr_.columns_.next(spos) )
    {
	MemBlockColumn* column = (MemBlockColumn*)wrr_.columns_.getObj( spos );
	if ( column->nruniquevisits_ < 1 )
	    column->retire();
	else if ( !column->isRetired() )
	    towrite_ += column;
    }
}

virtual od_int64 nrDone() const
{
    return colidx_;
}

virtual od_int64 totalNr() const
{
    return towrite_.size();
}

virtual uiString nrDoneText() const
{
   return tr("Column files written");
}

virtual uiString message() const
{
    if ( uirv_.isOK() )
       return tr("Writing edge blocks");
    return uirv_;
}

virtual int nextStep()
{
    if ( uirv_.isError() )
	return ErrorOccurred();

    if ( !towrite_.validIdx(colidx_) )
	return wrapUp();

    wrr_.writeColumn( *towrite_[colidx_], uirv_ );
    if ( uirv_.isError() )
	return ErrorOccurred();

    colidx_++;
    return MoreToDo();
}

int wrapUp()
{
    delete wrr_.strm_; wrr_.strm_ = 0;
    wrr_.writeInfoFiles( uirv_ );
    wrr_.isfinished_ = true;
    return uirv_.isOK() ? Finished() : ErrorOccurred();
}

    int			colidx_;
    uiRetVal		uirv_;
    Writer&		wrr_;
    ObjectSet<MemBlockColumn> towrite_;

};

} // namespace Blocks
} // namespace Seis


Task* Seis::Blocks::Writer::finisher()
{
    WriterFinisher* wf = new WriterFinisher( *this );
    if ( wf->towrite_.isEmpty() )
	{ delete wf; wf = 0; }
    return wf;
}