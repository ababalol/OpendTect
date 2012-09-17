/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		September 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: seiseventsnapper.cc,v 1.12 2011/11/22 23:30:11 cvsnanne Exp $";

#include "seiseventsnapper.h"
#include "seistrc.h"
#include "seisioobjinfo.h"
#include "seiscubeprov.h"
#include "seisselectionimpl.h"
#include "ioobj.h"
#include "cubesampling.h"
#include "binidvalset.h"


SeisEventSnapper::SeisEventSnapper( const Interval<float>& gate	)
    : Executor("Snapping to nearest event")
    , searchgate_(gate)
{
}


float SeisEventSnapper::findNearestEvent( const SeisTrc& trc, float tarz ) const
{
    SeisTrcValueSeries valseries( trc, 0 );
    ValueSeriesEvFinder<float,float> evfinder( valseries, trc.size(),
					       trc.info().sampling );
    if ( eventtype_ == VSEvent::GateMax || eventtype_ == VSEvent::GateMin )
    {
	Interval<float> gate( searchgate_ );
	gate.shift( tarz );
	return evfinder.find( eventtype_, gate ).pos;
    }

    Interval<float> gateabove( tarz+searchgate_.start, tarz );
    Interval<float> gatebelow( tarz, tarz+searchgate_.stop );
    const float eventposabove = evfinder.find( eventtype_, gateabove ).pos;
    const float eventposbelow = evfinder.find( eventtype_, gatebelow ).pos;
    if ( mIsUdf(eventposabove) && mIsUdf(eventposbelow) )
    {
	Interval<float> gate( tarz+searchgate_.start, tarz+searchgate_.stop );
	return evfinder.find( eventtype_, gate ).pos;
    }
    else if ( mIsUdf(eventposabove) )
	return eventposbelow;
    else if ( mIsUdf(eventposbelow) )
	return eventposabove;
    else
    {
	const float diffabove = tarz - eventposabove;
	const float diffbelow = eventposbelow - tarz;
	return diffabove < diffbelow ? eventposabove : eventposbelow;
    }

    return tarz;
}


SeisEventSnapper3D::SeisEventSnapper3D( const IOObj& ioobj, BinIDValueSet& bvs,
       				    const Interval<float>& gate	)
    : SeisEventSnapper(gate)
    , positions_(bvs)
{
    mscprov_ = new SeisMSCProvider( ioobj );
    mscprov_->prepareWork();

    const Interval<float> zrg = bvs.valRange( 0 );
    mscprov_->setSelData( new Seis::TableSelData(bvs,&gate) );
    totalnr_ = bvs.totalSize();
    nrdone_ = 0;
}


SeisEventSnapper3D::~SeisEventSnapper3D()
{
    delete mscprov_;
}


int SeisEventSnapper3D::nextStep()
{
    const SeisMSCProvider::AdvanceState res = mscprov_->advance();
    switch ( res )
    {
	case SeisMSCProvider::Error: return ErrorOccurred();
	case SeisMSCProvider::EndReached: return Finished();
	case SeisMSCProvider::NewPosition:
	{
	    SeisTrc* trc = mscprov_->get(0,0);
	    BinIDValueSet::Pos pos = positions_.findFirst( trc->info().binid );
	    if ( pos.valid() )
	    {
		BinID dummy; float zval;
		positions_.get( pos, dummy, zval );
		zval = findNearestEvent( *trc, zval );
		positions_.set( pos, zval );
		nrdone_++;
	    }
	}
	case SeisMSCProvider::Buffering:
	    return MoreToDo();
    }

    return Finished();
}


