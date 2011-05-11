/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2006
___________________________________________________________________

-*/

static const char* rcsID = "$Id: horizon2dextender.cc,v 1.9 2011-05-11 07:17:26 cvsumesh Exp $";

#include "horizon2dextender.h"

#include "emhorizon2d.h"
#include "horizon2dtracker.h"
#include "math2.h"
#include "survinfo.h"

namespace MPE 
{

Horizon2DExtender::Horizon2DExtender( EM::Horizon2D& hor,
					      const EM::SectionID& sid )
    : BaseHorizon2DExtender( hor, sid )
{
}


SectionExtender* Horizon2DExtender::create( EM::EMObject* emobj,
						const EM::SectionID& sid )
{
    mDynamicCastGet(EM::Horizon2D*,hor,emobj)
    return emobj && !hor ? 0 : new Horizon2DExtender( *hor, sid );
}


void Horizon2DExtender::initClass()
{
    ExtenderFactory().addCreator( create, Horizon2DTracker::keyword() );
}


BaseHorizon2DExtender::BaseHorizon2DExtender( EM::Horizon2D& hor,
				      const EM::SectionID& sid )
    : SectionExtender( sid )
    , surface_( hor )
    , anglethreshold_( 0.5 )
{}


/*SectionExtender* Horizon2DExtender::create( EM::EMObject* emobj,
					    const EM::SectionID& sid )
{
    mDynamicCastGet(EM::Horizon2D*,hor,emobj)
    return emobj && !hor ? 0 : new Horizon2DExtender( *hor, sid );
}


void Horizon2DExtender::initClass()
{
    ExtenderFactory().addCreator( create, Horizon2DTracker::keyword() );
}*/


void BaseHorizon2DExtender::setAngleThreshold( float rad )
{ anglethreshold_ = cos( rad ); }


float BaseHorizon2DExtender::getAngleThreshold() const
{ return Math::ACos(anglethreshold_); }


void BaseHorizon2DExtender::setDirection( const BinIDValue& dir )
{
    direction_ = dir;
    xydirection_ = SI().transform( BinID(0,0) ) - SI().transform( dir.binid );
    const double abs = xydirection_.abs();
    alldirs_ = mIsZero( abs, 1e-3 );
    if ( !alldirs_ )
	xydirection_ /= abs;
}


int BaseHorizon2DExtender::nextStep()
{
    for ( int idx=0; idx<startpos_.size(); idx++ )
    {
	addNeighbor( false, startpos_[idx] );
	addNeighbor( true, startpos_[idx] );
    }

    return 0;
}


void BaseHorizon2DExtender::addNeighbor( bool upwards, const RowCol& sourcerc )
{
    const StepInterval<int> colrange =
			    surface_.geometry().colRange( sid_, sourcerc.row );
    EM::SubID neighborsubid;
    Coord3 neighborpos;
    RowCol neighborrc = sourcerc;
    const CubeSampling& boundary = getExtBoundary();

    do 
    {
	neighborrc += RowCol( 0, upwards ? colrange.step : -colrange.step );
	if ( !colrange.includes(neighborrc.col) )
	    return;
	if ( !boundary.isEmpty() && !boundary.hrg.includes(BinID(neighborrc)) )
	    return;
	neighborsubid = neighborrc.toInt64();
	neighborpos = surface_.getPos( sid_, neighborsubid );
    }
    while ( !Coord(neighborpos).isDefined() );

    if ( neighborpos.isDefined() )
	return;

    const Coord3 sourcepos = surface_.getPos( sid_,sourcerc.toInt64() );

    if ( !alldirs_ )
    {
	const Coord dir = neighborpos - sourcepos;
	const double dirabs = dir.abs();
	if ( !mIsZero(dirabs,1e-3) )
	{
	    const Coord normdir = dir/dirabs;
	    const double cosangle = normdir.dot(xydirection_);
	    if ( cosangle<anglethreshold_ )
		return;
	}
    }

    Coord3 refpos = surface_.getPos( sid_, neighborsubid );
    refpos.z = sourcepos.z;
    const float testz = getDepth( sourcerc, neighborrc );
    surface_.setPos( sid_, neighborsubid, refpos, true );

    addTarget( neighborsubid, sourcerc.toInt64() );
}


float BaseHorizon2DExtender::getDepth( const RowCol& srcrc,
					 const RowCol& destrc ) const
{
    return surface_.getPos( sid_, srcrc.toInt64() ).z;
}


};  // namespace MPE
