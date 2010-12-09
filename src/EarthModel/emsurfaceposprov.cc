/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2005
-*/

static const char* rcsID = "$Id: emsurfaceposprov.cc,v 1.24 2010-12-09 11:35:53 cvsnanne Exp $";

#include "emsurfaceposprov.h"

#include "cubesampling.h"
#include "datapointset.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emrowcoliterator.h"
#include "emsurface.h"
#include "emsurfacegeometry.h"
#include "emsurfaceiodata.h"
#include "emhorizon2d.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "posinfo2d.h"
#include "survinfo.h"

const char* Pos::EMSurfaceProvider::id1Key()		{ return "ID.1"; }
const char* Pos::EMSurfaceProvider::id2Key()		{ return "ID.2"; }
const char* Pos::EMSurfaceProvider::zstepKey()		{ return "Z Step"; }
const char* Pos::EMSurfaceProvider::extraZKey()		{ return "Extra Z"; }


Pos::EMSurfaceProvider::EMSurfaceProvider()
    : surf1_(0)
    , surf2_(0)
    , hs_(SI().sampling(false).hrg)
    , zstep_(SI().zRange(true).step)
    , extraz_(0,0)
    , zrg1_(0,0)
    , zrg2_(0,0)
    , iterator_(0)
    , curz_(mUdf(float))
    , curzrg_(0,0)
    , estnrpos_(-1)
{
}


Pos::EMSurfaceProvider::EMSurfaceProvider( const Pos::EMSurfaceProvider& pp )
    : surf1_(0)
    , surf2_(0)
{
    *this = pp;
}


Pos::EMSurfaceProvider::~EMSurfaceProvider()
{
    if ( surf1_ ) surf1_->unRef();
    if ( surf2_ ) surf2_->unRef();
}


void Pos::EMSurfaceProvider::copyFrom( const Pos::EMSurfaceProvider& pp )
{
    if ( &pp == this ) return;

    if ( surf1_ ) surf1_->unRef();
    if ( surf2_ ) surf2_->unRef();
    surf1_ = pp.surf1_; if ( surf1_ ) surf1_->ref();
    surf2_ = pp.surf2_; if ( surf2_ ) surf2_->ref();
    zstep_ = pp.zstep_;
    extraz_ = pp.extraz_;
    id1_ = pp.id1_;
    id2_ = pp.id2_;
    hs_ = pp.hs_;
    zrg1_ = pp.zrg1_; zrg2_ = pp.zrg2_;
    estnrpos_ = -1;
}


const char* Pos::EMSurfaceProvider::type() const
{
    return sKey::Surface;
}


static void getSurfRanges( const EM::Surface& surf, HorSampling& hs,
			   Interval<float>& zrg, od_int64& estnrpos )
{
    bool veryfirst = true;
    for ( int idx=0; idx<surf.nrSections(); idx++ )
    {
	EM::RowColIterator it( surf, surf.sectionID(idx) );
	EM::PosID posid = it.next();
	while ( posid.objectID() != -1 )
	{
	    const Coord3 coord = surf.getPos( posid );
	    const BinID bid( SI().transform(coord) );
	    if ( veryfirst )
	    {
		veryfirst = false;
		hs.start = hs.stop = bid;
		zrg.start = zrg.stop = coord.z;
	    }
	    else
	    {
		if ( coord.z < zrg.start ) zrg.start = coord.z;
		if ( coord.z > zrg.stop ) zrg.stop = coord.z;
		hs.include( bid );
	    }
	    estnrpos++;

	    posid = it.next();
	}
    }
}


bool Pos::EMSurfaceProvider::initialize( TaskRunner* tr )
{
    if ( nrSurfaces() == 0 ) return false;

    EM::EMObject* emobj = EM::EMM().getObject( EM::EMM().getObjectID(id1_) );
    if ( !emobj )
	emobj = EM::EMM().loadIfNotFullyLoaded( id1_, tr );
    mDynamicCastGet(EM::Surface*,surf1,emobj)
    if ( !surf1 ) return false;
    surf1_ = surf1; surf1_->ref();

    getSurfRanges( *surf1_, hs_, zrg1_, estnrpos_ );

    if ( !id2_.isEmpty() )
    {
	emobj = EM::EMM().getObject( EM::EMM().getObjectID(id2_) );
	if ( !emobj )
	    emobj = EM::EMM().loadIfNotFullyLoaded( id2_, tr );
	mDynamicCastGet(EM::Surface*,surf2,emobj)
	if ( !surf2 ) return false;
	surf2_ = surf2; surf2_->ref();
	HorSampling hs( hs_ );
	od_int64 estnrpos2 = -1;
	getSurfRanges( *surf2_, hs, zrg2_, estnrpos2 );
	if ( estnrpos2 < estnrpos_ )
	    estnrpos_ = estnrpos2;
	hs_.limitTo( hs );
    }

    reset();
    return true;
}


void Pos::EMSurfaceProvider::reset()
{
    delete iterator_; iterator_ = 0;
    if ( surf1_ )
	iterator_ = new EM::RowColIterator( *surf1_, surf1_->sectionID(0) );
    curpos_ = EM::PosID( -1, -1, -1 );
}


bool Pos::EMSurfaceProvider::toNextPos()
{
    curpos_ = iterator_->next();
    if ( curpos_.objectID() == -1 )
	return false;

    curzrg_.start = curzrg_.stop = surf1_->getPos( curpos_ ).z;
    if ( surf2_ )
    {
	const float stop =
	    surf2_->getPos( surf2_->sectionID(0), curpos_.subID()).z;
	if ( !mIsUdf(stop) )
	    curzrg_.stop = stop;
    }

    curzrg_.sort();
    curzrg_ += extraz_;

    Interval<float> unsnappedzrg = curzrg_;
    if ( surf2_ || extraz_.width()>0 )
    {
	SI().snapZ( curzrg_.start, 1 );
	SI().snapZ( curzrg_.stop, -1 );
	if ( !unsnappedzrg.includes(curzrg_.start) )
	{
	    curz_ = mUdf(float);
	    return true;
	}
    }

    curz_ = curzrg_.start;
    return true;
}


bool Pos::EMSurfaceProvider::toNextZ()
{
    if ( mIsUdf(curz_) || !surf2_ )
	return toNextPos();

    curz_ += zstep_;
    if ( curz_ > curzrg_.stop )
	return toNextPos();

    return true;
}


float Pos::EMSurfaceProvider::curZ() const
{ return curz_; }


bool Pos::EMSurfaceProvider::hasZAdjustment() const
{
    return surf1_ && !surf2_;
}


float Pos::EMSurfaceProvider::adjustedZ( const Coord& c, float z ) const
{
    if ( !hasZAdjustment() ) return z;

    const BinID bid = SI().transform( c );
    return surf1_->getPos( surf1_->sectionID(0), bid.toInt64() ).z;
}


#define mGetSurfKey(k) IOPar::compKey(sKey::Surface,k)


void Pos::EMSurfaceProvider::usePar( const IOPar& iop )
{
    iop.get( mGetSurfKey(id1Key()), id1_ );
    iop.get( mGetSurfKey(id2Key()), id2_ );
    iop.get( mGetSurfKey(zstepKey()), zstep_ );
    iop.get( mGetSurfKey(extraZKey()), extraz_ );

    if ( id1_.isEmpty() ) return;
    EM::SurfaceIOData sd;
    EM::IOObjInfo eminfo( id1_ );
    if ( !eminfo.isOK() ) return;

    HorSampling hs;
    hs.set( eminfo.getInlRange(), eminfo.getCrlRange() );
    hs_ = hs;

    if ( id2_.isEmpty() ) return;
    eminfo = EM::IOObjInfo( id2_ );
    if ( !eminfo.isOK() ) return;
    hs.set( eminfo.getInlRange(), eminfo.getCrlRange() );
    hs_.limitTo( hs );
    // TODO: get zrg's
}


void Pos::EMSurfaceProvider::fillPar( IOPar& iop ) const
{
    iop.set( mGetSurfKey(id1Key()), id1_ );
    iop.set( mGetSurfKey(id2Key()), id2_ );
    iop.set( mGetSurfKey(zstepKey()), zstep_ );
    iop.set( mGetSurfKey(extraZKey()), extraz_ );
}


void Pos::EMSurfaceProvider::getSummary( BufferString& txt ) const
{
    switch ( nrSurfaces() )
    {
	case 0:
	return;
	case 1:
	    txt += "On '"; txt += IOM().nameOf(id1_); txt += "'";
	break;
	default:
	    txt += "Between '"; txt += IOM().nameOf(id1_);
	    txt += "' and '"; txt += IOM().nameOf(id2_);
	break;
    }
}


void Pos::EMSurfaceProvider::getZRange( Interval<float>& zrg ) const
{
    if ( !surf1_ ) return;

    zrg = zrg1_;

    if ( surf2_ )
    {
	if ( zrg2_.start < zrg.start ) zrg.start = zrg2_.start;
	if ( zrg2_.stop > zrg.stop ) zrg.stop = zrg2_.stop;
    }
}


int Pos::EMSurfaceProvider::estNrZPerPos() const
{
    if ( !surf2_ ) return 1;
    Interval<float> avgzrg( (zrg1_.start + zrg2_.start)*.5,
			    (zrg1_.stop + zrg2_.stop)*.5 );
    return (int)((avgzrg.stop-avgzrg.start) / zstep_ + .5);
}


int Pos::EMSurfaceProvider::nrSurfaces() const
{
    if ( !surf1_ )
	return id1_.isEmpty() ? 0 : (id2_.isEmpty() ? 1 : 2);
    return surf2_ ? 2 : 1;
}


BinID Pos::EMSurfaceProvider3D::curBinID() const
{
    BinID bid; bid.fromInt64( curpos_.subID() );
    return bid;
}


bool Pos::EMSurfaceProvider3D::includes( const BinID& bid, float z ) const
{
    if ( !surf1_ )
	return true;

    // TODO: support multiple sections
    Interval<float> zrg;
    const EM::SubID subid = bid.toInt64();
    const Coord3 crd1 = surf1_->getPos( surf1_->sectionID(0), subid );
    if ( !crd1.isDefined() )
	return false;

    if ( surf2_ )
    {
	const Coord3 crd2 = surf2_->getPos( surf2_->sectionID(0), subid );
	if ( !crd2.isDefined() )
	    return false;

	zrg.start = crd1.z; zrg.stop = crd2.z;
	zrg.sort();
    }
    else
    {
	zrg.start = crd1.z - SI().zStep()/2;
	zrg.stop = crd1.z + SI().zStep()/2;
    }

    zrg += extraz_;
    return zrg.includes( z );
}


void Pos::EMSurfaceProvider3D::getExtent( BinID& start, BinID& stop ) const
{
    start = hs_.start; stop = hs_.stop;
}


void Pos::EMSurfaceProvider3D::initClass()
{
    Pos::Provider3D::factory().addCreator( create, sKey::Surface );
}


// ***** Pos::EMSurfaceProvider2D ****
const char* Pos::EMSurfaceProvider2D::curLine() const
{
    BinID bid;
    bid.fromInt64( curpos_.subID() );
    if ( surf1_ )
    {
	mDynamicCastGet(EM::Horizon2D*,hor2d,surf1_);
	if ( !hor2d )
	    return 0;

	const PosInfo::GeomID& geomid = hor2d->geometry().lineGeomID( bid.inl );
	S2DPOS().setCurLineSet( geomid.lsid_ );
	return S2DPOS().getLineName( geomid.lineid_ );
    }

    return 0;
}


int Pos::EMSurfaceProvider2D::curNr() const
{
    BinID bid;
    bid.fromInt64( curpos_.subID() );
    return bid.crl;
}


Coord Pos::EMSurfaceProvider2D::curCoord() const
{
    if ( surf1_ )
	return surf1_->getPos( curpos_ );

    return Coord(0,0);
}


bool Pos::EMSurfaceProvider2D::includes( const Coord& c, float z ) const
{
    PosInfo::Line2DPos pos;
    if ( lineData() && lineData()->getPos( c, pos, SI().inlDistance() ) )
	return includes( pos.nr_, z );

    return false;
}


bool Pos::EMSurfaceProvider2D::includes( int nr, float z ) const
{
    if ( !lineData() || lineData()->lineName().isEmpty() )
	return false;

    mDynamicCastGet(EM::Horizon2D*,hor2d1,surf1_);
    if ( !hor2d1 )
	return false;

    Interval<float> zrg;
    BinID bid;
    bid.crl = nr;
    bid.inl = hor2d1->geometry().lineIndex( lineData()->lineName().buf() );
    const Coord3 crd1 = hor2d1->getPos( hor2d1->sectionID(0), bid.toInt64() );
    if ( !crd1.isDefined() )
	return false;

    mDynamicCastGet(EM::Horizon2D*,hor2d2,surf2_);
    if ( hor2d2 )
    {
	bid.inl = hor2d2->geometry().lineIndex(lineData()->lineName().buf());
	const Coord3 crd2 = hor2d2->getPos( hor2d2->sectionID(0),
					    bid.toInt64() );
	if ( !crd2.isDefined() )
	    false;

	zrg.start = crd1.z; zrg.stop = crd2.z;
	zrg.sort();
    }
    else
    {
	zrg.start = crd1.z - SI().zStep()/2;
	zrg.stop = crd1.z + SI().zStep()/2;
    }

    zrg += extraz_;
    return zrg.includes( z );
}


void Pos::EMSurfaceProvider2D::getExtent( Interval<int>& intv ) const
{
    intv.start = intv.stop = 0;
}


void Pos::EMSurfaceProvider2D::initClass()
{
    Pos::Provider2D::factory().addCreator( create, sKey::Surface );
}


// ***** Pos::EMSurface2DProvider3D ****
Pos::EMSurface2DProvider3D::EMSurface2DProvider3D()
    :dpssurf1_(*new DataPointSet(true,true))
    ,dpssurf2_(*new DataPointSet(true,true))
{}


Pos::EMSurface2DProvider3D::~EMSurface2DProvider3D()
{
    delete &dpssurf1_; delete &dpssurf2_;
}


Pos::EMSurface2DProvider3D::EMSurface2DProvider3D( 
	const Pos::EMSurface2DProvider3D& p )
    : dpssurf1_(*new DataPointSet(true,false))
    , dpssurf2_(*new DataPointSet(true,false))
{
    *this = p;
}


Pos::EMSurface2DProvider3D& Pos::EMSurface2DProvider3D::operator =(
	const Pos::EMSurface2DProvider3D& p )
{
    copyFrom(p);
    dpssurf1_ = p.dpssurf1_;
    dpssurf2_ = p.dpssurf2_;
    return *this;
}


void Pos::EMSurface2DProvider3D::mkDPS( const EM::Surface& s,
					DataPointSet& dps )
{
    mDynamicCastGet(const EM::Horizon2D&,surf,s)

    for ( int idx=0; idx<surf.nrSections(); idx++ )
    {
	EM::RowColIterator it( surf, surf.sectionID(idx) );
	while ( true )
	{
	    EM::PosID posid = it.next();
	    if ( posid.objectID() < 0 )
		break;

	    const BinID bid2d( posid.subID() );
	    DataPointSet::Pos pos( surf.getPos(posid) );
	    pos.nr_ = bid2d.crl;
	    dps.addRow( DataPointSet::DataRow(pos,bid2d.inl) );
	}
    }
}


bool Pos::EMSurface2DProvider3D::initialize( TaskRunner* tr )
{
    if ( !EMSurfaceProvider::initialize(tr) || !surf1_ )
	return false;

    mkDPS( *surf1_, dpssurf1_ );
    if ( surf2_ )
	mkDPS( *surf2_, dpssurf2_ );
    return true;
}


BinID Pos::EMSurface2DProvider3D::curBinID() const
{
    return !surf1_ ? BinID(0,0) : SI().transform( surf1_->getPos(curpos_) );
}


bool Pos::EMSurface2DProvider3D::includes( const BinID& bid, float z ) const
{
    if ( !surf1_ )
	return false;

    const DataPointSet::RowID rid1 = dpssurf1_.findFirst( bid );
    if ( rid1 < 0 )
	return false;
    DataPointSet::RowID rid2 = -1;
    if ( surf2_ )
    {
	rid2 = dpssurf2_.findFirst( bid );
	if ( rid2 < 0 )
	    return false;
    }

    Interval<float> zrg( dpssurf1_.z(rid1), 0 ); zrg.stop = zrg.start;
    if ( rid2 >= 0 )
	zrg.include( dpssurf2_.z(rid2), false );
    zrg += extraz_;
    zrg.widen( SI().zStep() * .5, false );
    return zrg.includes( z );
}


void Pos::EMSurface2DProvider3D::getExtent( BinID& start, BinID& stop ) const
{
    start = hs_.start; stop = hs_.stop;
}
