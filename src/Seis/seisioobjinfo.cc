/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : June 2005
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "seisioobjinfo.h"

#include "bufstringset.h"
#include "cbvsreadmgr.h"
#include "conn.h"
#include "dirlist.h"
#include "file.h"
#include "filepath.h"
#include "keystrs.h"
#include "globexpr.h"
#include "iodir.h"
#include "iodirentry.h"
#include "ioman.h"
#include "iopar.h"
#include "iostrm.h"
#include "keystrs.h"
#include "linekey.h"
#include "posinfo2d.h"
#include "ptrman.h"
#include "seis2ddata.h"
#include "seisbuf.h"
#include "seiscbvs.h"
#include "seiscbvs2d.h"
#include "seispsioprov.h"
#include "seisread.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "trckeyzsampling.h"
#include "zdomain.h"


Seis::ObjectSummary::ObjectSummary( const MultiID& mid )
    : ioobjinfo_(*new SeisIOObjInfo(mid))		{ init(); }
Seis::ObjectSummary::ObjectSummary( const IOObj& ioobj )
    : ioobjinfo_(*new SeisIOObjInfo(ioobj))		{ init(); }
Seis::ObjectSummary::ObjectSummary( const IOObj& ioobj, Pos::GeomID geomid )
    : ioobjinfo_(*new SeisIOObjInfo(ioobj))		{ init2D(geomid); }
Seis::ObjectSummary::ObjectSummary( const Seis::ObjectSummary& oth )
    : ioobjinfo_(*new SeisIOObjInfo(oth.ioobjinfo_))	{ init(); }

Seis::ObjectSummary::~ObjectSummary()
{
    delete &ioobjinfo_;
}


Seis::ObjectSummary& Seis::ObjectSummary::operator =(
						const Seis::ObjectSummary& oth )
{
    if ( &oth == this ) return *this;

    const_cast<SeisIOObjInfo&>( ioobjinfo_ ) = oth.ioobjinfo_;
    init();

    return *this;
}


void Seis::ObjectSummary::init()
{
    bad_ = !ioobjinfo_.isOK() || ioobjinfo_.is2D();
    if ( bad_ ) return;

    geomtype_ = ioobjinfo_.geomType();
    TrcKeyZSampling tkzs( false );
    ioobjinfo_.getRanges( tkzs );
    zsamp_ = tkzs.zsamp_;

    SeisTrcReader rdr( ioobjinfo_.ioObj() );
    if ( !rdr.prepareWork(Seis::PreScan) || !rdr.seisTranslator() )
	{ pErrMsg("Translator not SeisTrcTranslator!"); bad_ = true; return; }

    refreshCache( *rdr.seisTranslator() );
}


void Seis::ObjectSummary::init2D( Pos::GeomID geomid )
{
    bad_ = !ioobjinfo_.isOK();
    if ( bad_ ) return;

    geomtype_ = ioobjinfo_.geomType();
    StepInterval<int> trcrg;
    ioobjinfo_.getRanges( geomid, trcrg, zsamp_ );

    SeisTrcReader rdr( ioobjinfo_.ioObj() );
    TrcKeySampling tks( geomid );
    tks.setTrcRange( trcrg );
    Seis::RangeSelData* sd = new Seis::RangeSelData( tks );
    rdr.setSelData( sd );
    if ( !rdr.prepareWork(Seis::PreScan) || !rdr.seis2Dtranslator() )
	{ pErrMsg("Translator not SeisTrcTranslator!"); bad_ = true; return; }

    refreshCache( *rdr.seis2Dtranslator() );
}


void Seis::ObjectSummary::refreshCache( const SeisTrcTranslator& trl )
{
    if ( !trl.inputComponentData().isEmpty() )
	datachar_ = trl.componentInfo().first()->org.datachar;

    trl.getComponentNames( compnms_ );
    nrcomp_ = compnms_.size();
    nrsamppertrc_ = zsamp_.nrSteps()+1;
    nrbytespersamp_ = datachar_.nrBytes();
    nrdatabytespespercomptrc_ = nrbytespersamp_ * nrsamppertrc_;
    nrdatabytespertrc_ = nrdatabytespespercomptrc_ * nrcomp_;
    nrbytestrcheader_ = trl.bytesOverheadPerTrace();
    nrbytespertrc_ = nrbytestrcheader_ + nrdatabytespertrc_;
}


bool Seis::ObjectSummary::hasSameFormatAs( const BinDataDesc& desc ) const
{
    return datachar_ == desc;
}


#define mGoToSeisDir() \
    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Seis)->id_) )

#define mGetDataSet(nm,rv) \
    if ( !isOK() || !is2D() || isPS() ) return rv; \
 \
    PtrMan<Seis2DDataSet> nm \
	= new Seis2DDataSet( *ioobj_ ); \
    if ( nm->nrLines() == 0 ) \
	return rv


SeisIOObjInfo::SeisIOObjInfo( const IOObj* ioobj )
	: ioobj_(ioobj ? ioobj->clone() : 0)		{ setType(); }
SeisIOObjInfo::SeisIOObjInfo( const IOObj& ioobj )
	: ioobj_(ioobj.clone())				{ setType(); }
SeisIOObjInfo::SeisIOObjInfo( const MultiID& id )
	: ioobj_(IOM().get(id))				{ setType(); }


SeisIOObjInfo::SeisIOObjInfo( const char* ioobjnm, Seis::GeomType geomtype )
	: ioobj_(0)
	, geomtype_(geomtype)
{
    mGoToSeisDir();
    switch ( geomtype_ )
    {
	case Seis::Vol:
	ioobj_ = IOM().getLocal( ioobjnm, mTranslGroupName(SeisTrc) );
	break;

	case Seis::VolPS:
	ioobj_ = IOM().getLocal( ioobjnm, mTranslGroupName(SeisPS3D) );
	break;

	case Seis::Line:
	ioobj_ = IOM().getLocal( ioobjnm, mTranslGroupName(SeisTrc2D) );
	break;

	case Seis::LinePS:
	ioobj_ = IOM().getLocal( ioobjnm, mTranslGroupName(SeisPS2D) );
	break;
    }

    setType();
}


SeisIOObjInfo::SeisIOObjInfo( const char* ioobjnm )
	: ioobj_(0)
{
    mGoToSeisDir();
    ioobj_ = IOM().getLocal( ioobjnm, 0 );
    setType();
}


SeisIOObjInfo::SeisIOObjInfo( const SeisIOObjInfo& sii )
	: geomtype_(sii.geomtype_)
	, bad_(sii.bad_)
{
    ioobj_ = sii.ioobj_ ? sii.ioobj_->clone() : 0;
}


SeisIOObjInfo::~SeisIOObjInfo()
{
    delete ioobj_;
}


SeisIOObjInfo& SeisIOObjInfo::operator =( const SeisIOObjInfo& sii )
{
    if ( &sii != this )
    {
	delete ioobj_;
	ioobj_ = sii.ioobj_ ? sii.ioobj_->clone() : 0;
	geomtype_ = sii.geomtype_;
	bad_ = sii.bad_;
    }
    return *this;
}


void SeisIOObjInfo::setType()
{
    bad_ = !ioobj_;
    if ( bad_ ) return;

    const BufferString trgrpnm( ioobj_->group() );
    bool isps = false;
    if ( SeisTrcTranslator::isPS(*ioobj_) )
	isps = true;
    ioobj_->pars().getYN( SeisTrcTranslator::sKeyIsPS(), isps );

    if ( !isps && ioobj_->group()!=mTranslGroupName(SeisTrc) &&
	    ioobj_->group()!=mTranslGroupName(SeisTrc2D) )
	{ bad_ = true; return; }

    const bool is2d = SeisTrcTranslator::is2D( *ioobj_, false );
    geomtype_ = isps ? (is2d ? Seis::LinePS : Seis::VolPS)
		     : (is2d ? Seis::Line : Seis::Vol);
}


SeisIOObjInfo::SpaceInfo::SpaceInfo( int ns, int ntr, int bps )
	: expectednrsamps(ns)
	, expectednrtrcs(ntr)
	, maxbytespsamp(bps)
{
    if ( expectednrsamps < 0 )
	expectednrsamps = SI().zRange(false).nrSteps() + 1;
    if ( expectednrtrcs < 0 )
	expectednrtrcs = mCast( int, SI().sampling(false).hsamp_.totalNr() );
}


#define mChk(ret) if ( bad_ ) return ret

bool SeisIOObjInfo::getDefSpaceInfo( SpaceInfo& spinf ) const
{
    mChk(false);

    if ( Seis::isPS(geomtype_) )
    {
	if ( is2D() )
	    return false;
	else
	{
	    SeisPS3DReader* rdr = SPSIOPF().get3DReader( *ioobj_ );
	    if ( !rdr )
		return false;

	    const PosInfo::CubeData& cd = rdr->posData();
	    spinf.expectednrtrcs = cd.totalSize();
	    delete rdr;
	}
	spinf.expectednrsamps = SI().zRange(false).nrSteps() + 1;
	return true;
    }

    if ( is2D() )
    {
	mGetDataSet(dset,false);
	StepInterval<int> trcrg; StepInterval<float> zrg;
	TypeSet<Pos::GeomID> seen;
	spinf.expectednrtrcs = 0;
	for ( int idx=0; idx<dset->nrLines(); idx++ )
	{
	    const Pos::GeomID geomid = dset->geomID( idx );
	    if ( !seen.isPresent(geomid) )
	    {
		seen.add( geomid );
		dset->getRanges( geomid, trcrg, zrg );
		spinf.expectednrtrcs += trcrg.nrSteps() + 1;
	    }
	}
	spinf.expectednrsamps = zrg.nrSteps() + 1;
	spinf.maxbytespsamp = 4;
	return true;
    }

    TrcKeyZSampling cs;
    if ( !getRanges(cs) )
	return false;

    spinf.expectednrsamps = cs.zsamp_.nrSteps() + 1;
    spinf.expectednrtrcs = mCast( int, cs.hsamp_.totalNr() );
    getBPS( spinf.maxbytespsamp, -1 );
    return true;
}


bool SeisIOObjInfo::isTime() const
{
    const bool siistime = SI().zIsTime();
    mChk(siistime);
    return ZDomain::isTime( ioobj_->pars() );
}


bool SeisIOObjInfo::isDepth() const
{
    const bool siisdepth = !SI().zIsTime();
    mChk(siisdepth);
    return ZDomain::isDepth( ioobj_->pars() );
}


const ZDomain::Def& SeisIOObjInfo::zDomainDef() const
{
    mChk(ZDomain::SI());
    return ZDomain::Def::get( ioobj_->pars() );
}


int SeisIOObjInfo::SpaceInfo::expectedMBs() const
{
    if ( expectednrsamps<0 || expectednrtrcs<0 )
	return -1;
    od_int64 totnrbytes = expectednrsamps;
    totnrbytes *= expectednrtrcs;
    totnrbytes *= maxbytespsamp;
    return (int)( totnrbytes / 1048576 );
}


int SeisIOObjInfo::expectedMBs( const SpaceInfo& si ) const
{
    mChk(-1);

    int nrbytes = si.expectedMBs();
    if ( nrbytes < 0 || isPS() )
	return nrbytes;

    mDynamicCast(SeisTrcTranslator*,PtrMan<SeisTrcTranslator> sttr,
		    ioobj_->createTranslator() );
    if ( !sttr )
	return -1;

    int overhead = sttr->bytesOverheadPerTrace();
    double sz = si.expectednrsamps;
    sz *= si.maxbytespsamp;
    sz = (sz + overhead) * si.expectednrtrcs;

    const double bytes2mb = 9.53674e-7;
    return (int)((sz * bytes2mb) + .5);
}


od_int64 SeisIOObjInfo::getFileSize( const char* filenm, int& nrfiles )
{
    if ( !File::isDirectory(filenm) && File::isEmpty(filenm) ) return -1;

    od_int64 totalsz = 0;
    nrfiles = 0;
    if ( File::isDirectory(filenm) )
    {
	DirList dl( filenm, DirList::FilesOnly );
	for ( int idx=0; idx<dl.size(); idx++ )
	{
	    FilePath filepath = dl.fullPath( idx );
	    FixedString ext = filepath.extension();
	    if ( ext != "cbvs" )
		continue;

	    totalsz += File::getKbSize( filepath.fullPath() );
	    nrfiles++;
	}
    }
    else
    {
	while ( true )
	{
	    BufferString fullnm( CBVSIOMgr::getFileName(filenm,nrfiles) );
	    if ( !File::exists(fullnm) ) break;

	    totalsz += File::getKbSize( fullnm );
	    nrfiles++;
	}
    }

    return totalsz;
}


od_int64 SeisIOObjInfo::getFileSize() const
{
    const char* fnm = ioobj_->fullUserExpr();
    int nrfiles;
    return getFileSize( fnm, nrfiles );
}


bool SeisIOObjInfo::getRanges( TrcKeyZSampling& cs ) const
{
    mChk(false);
    mDynamicCastGet(IOStream*,iostrm,ioobj_)
    if ( is2D() )
    {
	StepInterval<int> trcrg;
	if ( (iostrm && iostrm->isMulti()) ||
	     !getRanges(cs.hsamp_.getGeomID(),trcrg,cs.zsamp_) )
	    return false;

	cs.hsamp_.setTrcRange( trcrg );
	return true;
    }

    cs.init( true );
    if ( !isPS() )
	return SeisTrcTranslator::getRanges( *ioobj_, cs );

    PtrMan<SeisPS3DReader> rdr = SPSIOPF().get3DReader( *ioobj_ );
    if ( !rdr )
	return false;

    cs.zsamp_ = rdr->getZRange();
    const PosInfo::CubeData& cd = rdr->posData();
    StepInterval<int> rg;
    cd.getInlRange( rg ); cs.hsamp_.setInlRange( rg );
    cd.getCrlRange( rg ); cs.hsamp_.setCrlRange( rg );
    return true;
}


bool SeisIOObjInfo::getDataChar( DataCharacteristics& dc ) const
{
    mChk(false);
    mDynamicCast(SeisTrcTranslator*,PtrMan<SeisTrcTranslator> sttr,
		 ioobj_->createTranslator() );
    if ( !sttr )
	{ pErrMsg("No Translator!"); return false; }

    Conn* conn = ioobj_->getConn( Conn::Read );
    if ( !sttr->initRead(conn,Seis::PreScan) )
	return false;

    ObjectSet<SeisTrcTranslator::TargetComponentData>& comps
		= sttr->componentInfo();
    if ( comps.isEmpty() )
	return false;

    dc = comps.first()->org.datachar;
    return true;
}


bool SeisIOObjInfo::getBPS( int& bps, int icomp ) const
{
    mChk(false);
    if ( is2D() )
	return 4;

    if ( isPS() )
    {
	pErrMsg("TODO: no BPS for PS");
	return false;
    }

    mDynamicCast(SeisTrcTranslator*,PtrMan<SeisTrcTranslator> sttr,
		 ioobj_->createTranslator() );
    if ( !sttr )
	{ pErrMsg("No Translator!"); return false; }

    Conn* conn = ioobj_->getConn( Conn::Read );
    bool isgood = sttr->initRead(conn);
    bps = 0;
    if ( isgood )
    {
	ObjectSet<SeisTrcTranslator::TargetComponentData>& comps
		= sttr->componentInfo();
	for ( int idx=0; idx<comps.size(); idx++ )
	{
	    int thisbps = (int)comps[idx]->datachar.nrBytes();
	    if ( icomp < 0 )
		bps += thisbps;
	    else if ( icomp == idx )
		bps = thisbps;
	}
    }

    if ( bps == 0 ) bps = 4;
    return isgood;
}


#define mGetZDomainGE \
    const GlobExpr zdomge( o2d.zdomky_.isEmpty() ? ZDomain::SI().key() \
						 : o2d.zdomky_.buf() )

void SeisIOObjInfo::getGeomIDs( TypeSet<Pos::GeomID>& geomids ) const
{
    if ( !isOK() )
	return;

    if ( isPS() )
    {
	SPSIOPF().getGeomIDs( *ioobj_, geomids );
	return;
    }

    if ( !is2D() ) return;

    PtrMan<Seis2DDataSet> dset = new Seis2DDataSet( *ioobj_ );
    dset->getGeomIDs( geomids );
}


bool SeisIOObjInfo::isCompatibleType( const char* typestr1,
				      const char* typestr2 )
{
    const FixedString typ1( typestr1 ); const FixedString typ2( typestr2 );
    if ( typ1 == typ2 )
	return true;

    // One extra chance: 'Attribute' is compatible with 'no type'.
    const bool isnorm1 = typ1.isEmpty() || typ1 == sKey::Attribute();
    const bool isnorm2 = typ2.isEmpty() || typ2 == sKey::Attribute();
    return isnorm1 && isnorm2;
}


void SeisIOObjInfo::getNms( BufferStringSet& bss,
			    const SeisIOObjInfo::Opts2D& o2d ) const
{
    if ( !isOK() )
	return;

    if ( isPS() )
    {
	SPSIOPF().getLineNames( *ioobj_, bss );
	return;
    }

   if ( !isOK() || !is2D() || isPS() ) return;

    PtrMan<Seis2DDataSet> dset
	= new Seis2DDataSet( *ioobj_ );
    if ( dset->nrLines() == 0 )
	return;
    mGetZDomainGE;

    BufferStringSet rejected;
    for ( int idx=0; idx<dset->nrLines(); idx++ )
    {
	const char* nm = dset->lineName(idx);
	if ( bss.isPresent(nm) )
	    continue;

	if ( o2d.bvs_ )
	{
	    if ( rejected.isPresent(nm) )
		continue;
	}

	bss.add( nm );
    }

    bss.sort();
}


bool SeisIOObjInfo::getRanges( const Pos::GeomID geomid,
			       StepInterval<int>& trcrg,
			       StepInterval<float>& zrg ) const
{
    mChk(false);
    PtrMan<Seis2DDataSet> dataset = new Seis2DDataSet( *ioobj_ );
    return dataset->getRanges( geomid, trcrg, zrg );
}


static BufferStringSet& getTypes()
{
    mDefineStaticLocalObject( BufferStringSet, types, );
    return types;
}

static TypeSet<MultiID>& getIDs()
{
    mDefineStaticLocalObject( TypeSet<MultiID>, ids, );
    return ids;
}


void SeisIOObjInfo::initDefault( const char* typ )
{
    BufferStringSet& typs = getTypes();
    if ( typs.isPresent(typ) ) return;

    IOObjContext ctxt( SeisTrcTranslatorGroup::ioContext() );
    ctxt.toselect_.require_.set( sKey::Type(), typ );
    int nrpresent = 0;
    PtrMan<IOObj> ioobj = IOM().getFirst( ctxt, &nrpresent );
    if ( !ioobj || nrpresent > 1 )
	return;

    typs.add( typ );
    getIDs() += ioobj->key();
}


const MultiID& SeisIOObjInfo::getDefault( const char* typ )
{
    mDefineStaticLocalObject( const MultiID, noid, ("") );
    const int typidx = getTypes().indexOf( typ );
    return typidx < 0 ? noid : getIDs()[typidx];
}


void SeisIOObjInfo::setDefault( const MultiID& id, const char* typ )
{
    BufferStringSet& typs = getTypes();
    TypeSet<MultiID>& ids = getIDs();

    const int typidx = typs.indexOf( typ );
    if ( typidx > -1 )
	ids[typidx] = id;
    else
    {
	typs.add( typ );
	ids += id;
    }
}


int SeisIOObjInfo::nrComponents( Pos::GeomID geomid ) const
{
    return getComponentInfo( geomid, 0 );
}


void SeisIOObjInfo::getComponentNames( BufferStringSet& nms,
				       Pos::GeomID geomid ) const
{
    getComponentInfo( geomid, &nms );
}


void SeisIOObjInfo::getCompNames( const MultiID& mid, BufferStringSet& nms )
{
    SeisIOObjInfo ioobjinf( mid );
    ioobjinf.getComponentNames( nms, Survey::GM().cUndefGeomID() );
}


int SeisIOObjInfo::getComponentInfo( Pos::GeomID geomid,
				     BufferStringSet* nms ) const
{
    int ret = 0; if ( nms ) nms->erase();
    mChk(ret);
    if ( isPS() )
	return 0;

    if ( !is2D() )
    {
	mDynamicCast(SeisTrcTranslator*,PtrMan<SeisTrcTranslator> sttr,
		     ioobj_->createTranslator() );
	if ( !sttr )
	    { pErrMsg("No Translator!"); return 0; }
	Conn* conn = ioobj_->getConn( Conn::Read );
	if ( sttr->initRead(conn) )
	{
	    ret = sttr->componentInfo().size();
	    if ( nms )
	    {
		for ( int icomp=0; icomp<ret; icomp++ )
		    nms->add( sttr->componentInfo()[icomp]->name() );
	    }
	}
    }
    else
    {
	PtrMan<Seis2DDataSet> dataset = new Seis2DDataSet( *ioobj_ );
	if ( !dataset || dataset->nrLines() == 0 )
	    return 0;

	int lidx = dataset->indexOf( geomid );
	if ( lidx < 0 ) lidx = 0;
	SeisTrcBuf tbuf( true );
	Executor* ex = dataset->lineFetcher( dataset->geomID(lidx), tbuf, 1 );
	if ( ex ) ex->doStep();
	ret = tbuf.isEmpty() ? 0 : tbuf.get(0)->nrComponents();
	if ( nms )
	{
	    mDynamicCastGet(Seis2DLineGetter*,lg,ex)
	    if ( !lg )
	    {
		for ( int icomp=0; icomp<ret; icomp++ )
		    nms->add( BufferString("[",icomp+1,"]") );
	    }
	    else
	    {
		ret = lg->translator() ?
		      lg->translator()->componentInfo().size() : 0;
		if ( nms )
		{
		    for ( int icomp=0; icomp<ret; icomp++ )
			nms->add(
			    lg->translator()->componentInfo()[icomp]->name() );
		}
	    }
	}
	delete ex;
    }

    return ret;
}


bool SeisIOObjInfo::hasData( Pos::GeomID geomid )
{
    const char* linenm = Survey::GM().getName( geomid );
    const MultiID mid ( IOObjContext::getStdDirData(IOObjContext::Seis)->id_ );
    const IODir iodir( mid );
    const ObjectSet<IOObj>& ioobjs = iodir.getObjs();
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	const IOObj& ioobj = *ioobjs[idx];
	if ( SeisTrcTranslator::isPS(ioobj) )
	{
	    BufferStringSet linenames;
	    SPSIOPF().getLineNames( ioobj, linenames );
	    if ( linenames.isPresent(linenm) )
		return true;
	}
	else
	{
	    if ( !(*ioobj.group() == '2') )
		continue;

	    Seis2DDataSet dset( ioobj );
	    if ( dset.isPresent(geomid) )
		return true;
	}
    }

    return false;
}


void SeisIOObjInfo::getDataSetNamesForLine( const char* lnm,
					    BufferStringSet& datasets,
					    Opts2D o2d )
{ getDataSetNamesForLine( Survey::GM().getGeomID(lnm), datasets, o2d ); }

void SeisIOObjInfo::getDataSetNamesForLine( Pos::GeomID geomid,
					    BufferStringSet& datasets,
					    Opts2D o2d )
{
    if ( geomid == mUdfGeomID )
	return;

    IOObjContext ctxt( mIOObjContext(SeisTrc2D) );
    const IODir iodir( ctxt.getSelKey() );
    const IODirEntryList del( iodir, ctxt );
    for ( int idx=0; idx<del.size(); idx++ )
    {
	const IOObj* ioobj = del[idx]->ioobj_;
	if ( !ioobj )
	    continue;

	if ( !o2d.zdomky_.isEmpty() )
	{
	    const FixedString zdomkey = ioobj->pars().find( ZDomain::sKey() );
	    if ( zdomkey && (o2d.zdomky_ != zdomkey) )
		continue;
	}

	if ( o2d.steerpol_ != 2 )
	{
	    const FixedString dt = ioobj->pars().find( sKey::Type() );
	    const bool issteering = dt==sKey::Steering();
	    const bool wantsteering = o2d.steerpol_ == 1;
	    if ( issteering != wantsteering ) continue;
	}

	Seis2DDataSet ds( *ioobj );
	if ( ds.isPresent(geomid) )
	    datasets.add( ioobj->name() );
    }
}


bool SeisIOObjInfo::isFullyRectAndRegular() const
{
    PtrMan<Translator> trl = ioobj_->createTranslator();
    mDynamicCastGet(CBVSSeisTrcTranslator*,cbvstrl,trl.ptr())
    if ( !cbvstrl ) return false;

    Conn* conn = ioobj_->getConn( Conn::Read );
    if ( !cbvstrl->initRead(conn) || !cbvstrl->readMgr() )
	return false;

    const CBVSInfo& info = cbvstrl->readMgr()->info();
    return info.geom_.fullyrectandreg;
}


void SeisIOObjInfo::getLinesWithData( BufferStringSet& lnms,
				      TypeSet<Pos::GeomID>& gids )
{
    Survey::GMAdmin().updateGeometries( 0 );
    Survey::GM().getList( lnms, gids, true );
    BoolTypeSet hasdata( gids.size(), false );

    const MultiID mid ( IOObjContext::getStdDirData(IOObjContext::Seis)->id_ );
    const IODir iodir( mid );
    const ObjectSet<IOObj>& ioobjs = iodir.getObjs();
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	const IOObj& ioobj = *ioobjs[idx];
	TypeSet<Pos::GeomID> dsgids;
	if ( SeisTrcTranslator::isPS(ioobj) )
	    SPSIOPF().getGeomIDs( ioobj, dsgids );
	else if ( *ioobj.group() == '2' )
	{
	    Seis2DDataSet dset( ioobj );
	    dset.getGeomIDs( dsgids );
	}

	if ( dsgids.isEmpty() )
	    continue;

	for ( int idl=0; idl<gids.size(); idl++ )
	{
	    if ( !hasdata[idl] && dsgids.isPresent(gids[idl]) )
		hasdata[idl] = true;
	}
    }

    for ( int idl=gids.size()-1; idl>=0; idl-- )
    {
	if ( !hasdata[idl] )
	{
	    lnms.removeSingle( idl );
	    gids.removeSingle( idl );
	}
    }
}


bool SeisIOObjInfo::getDisplayPars( IOPar& iop ) const
{
    if ( !ioobj_ )
	return false;

    FilePath fp( ioobj_->fullUserExpr(true) );
    fp.setExtension( "par" );
    return iop.read(fp.fullPath(),sKey::Pars()) && !iop.isEmpty();
}
