/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : January 2008
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "gridder2d.h"

#include "delaunay.h"
#include "linsolv.h"
#include "iopar.h"
#include "positionlist.h"
#include "math2.h"
#include "sorting.h"
#include "trigonometry.h"

#define mEpsilon 0.0001

mImplFactory( Gridder2D, Gridder2D::factory );


Gridder2D::Gridder2D()
    : values_(0)
    , points_(0)
    , trend_(0)
{}


Gridder2D::Gridder2D( const Gridder2D& g )
    : values_(g.values_)
    , points_(g.points_)
    , trend_(0)
    , usedpoints_(g.usedpoints_)
{
    if ( g.trend_ )
	trend_ = new PolyTrend( *g.trend_ );
}


Gridder2D::~Gridder2D()
{
    delete trend_;
}


bool Gridder2D::operator==( const Gridder2D& g ) const
{
    if ( factoryKeyword() != g.factoryKeyword() )
	return false;

    if ( ( trend_ && !g.trend_ ) )
	return false;

    if ( trend_ && !( *trend_ == *g.trend_ ) )
	return false;

    return usedpoints_ == g.usedpoints_;
}


bool Gridder2D::isPointUsable(const Coord& cpt,const Coord& dpt) const
{ return true; }


bool Gridder2D::setPoints( const TypeSet<Coord>& cl )
{
    points_ = &cl;

    if ( trend_ && values_ && points_->size() == values_->size() )
	trend_->set( *points_, values_->arr() );

    usedpoints_.setEmpty();
    for ( int idx=0; idx<points_->size(); idx++ )
    {
	if ( (*points_)[idx].isDefined() )
	    usedpoints_ += idx;
    }

    if ( !pointsChangedCB(0) )
    {
	points_ = 0;
	return false;
    }

    return true;
}


bool Gridder2D::setValues( const TypeSet<float>& vl )
{
    values_ = &vl;

    if ( trend_ && points_ && points_->size() == values_->size() )
	trend_->set( *points_, values_->arr() );

    valuesChangedCB(0);

    return true;
}


void Gridder2D::setTrend( PolyTrend::Order order )
{
    if ( !points_ || !values_ || points_->size() != values_->size() )
	return;

    if ( trend_ )
    {
	if ( order == trend_->getOrder() )
	    return;

	delete trend_;
	trend_ = 0;
    }

    if ( order != PolyTrend::None )
    {
	trend_ = new PolyTrend();
	trend_->setOrder( order );
    }

    if ( points_ && values_ && points_->size() == values_->size() )
    {
	if ( trend_ )
	    trend_->set( *points_, *values_ );

	valuesChangedCB(0);
    }
}


float Gridder2D::getDetrendedValue( int idx ) const
{
    if ( !values_ || !values_->validIdx(idx) )
	return mUdf(float);

    float val = (*values_)[idx];
    if ( trend_ && points_ && points_->validIdx(idx) )
	trend_->apply( (*points_)[idx], true, val );

    return val;
}


bool Gridder2D::isAtInputPos( const Coord& gridpoint, int& exactpos ) const
{
    if ( !gridpoint.isDefined() || !points_ )
	return false;

    for ( int idx=0; idx<usedpoints_.size(); idx++ )
    {
	const Coord& pos = (*points_)[usedpoints_[idx]];
	if ( mIsZero(gridpoint.sqDistTo(pos),mEpsilon) )
	{
	    exactpos = idx;
	    return true;
	}
    }

    return false;
}


float Gridder2D::getValue( const Coord& gridpoint,
			   const TypeSet<double>* inpweights,
			   const TypeSet<int>* inprelevantpoints ) const
{
    if ( !values_ )
	return mUdf(float);

    int exactpos;
    if ( isAtInputPos(gridpoint,exactpos) )
	return (*values_)[exactpos];

    TypeSet<double> weights;
    TypeSet<int> relevantpoints;
    const bool needweight = !inpweights || !inprelevantpoints ||
			    inpweights->size() != inprelevantpoints->size();
    if ( needweight && !getWeights(gridpoint,weights,relevantpoints) )
	return mUdf(float);

    const TypeSet<double>* curweights = needweight ? &weights : inpweights;
    const TypeSet<int>* currelevantpoints = needweight ? &relevantpoints
						       : inprelevantpoints;

    double valweightsum = 0.;
    double weightsum = 0.;
    int nrvals = 0;
    const int sz = currelevantpoints->size();
    for ( int idx=0; idx<sz; idx++ )
    {
	const float val = getDetrendedValue( (*currelevantpoints)[idx] );
	if ( mIsUdf(val) )
	    continue;

	valweightsum += val * (*curweights)[idx];
	weightsum += (*curweights)[idx];
	nrvals++;
    }

    if ( !nrvals )
	return mUdf(float);

    double val = nrvals==sz ? valweightsum : valweightsum / weightsum;
    //TODO: QC above
    if ( trend_ )
	trend_->apply( gridpoint, false, val );

    return mCast( float, val );
}


void Gridder2D::fillPar( IOPar& par ) const
{
    if ( trend_ )
	par.set( PolyTrend::sKeyOrder(), trend_->getOrder() );
}


bool Gridder2D::usePar( const IOPar& par )
{
    int order;
    if ( par.get(PolyTrend::sKeyOrder(),order) )
	setTrend( (PolyTrend::Order)order );

    return true;
}



InverseDistanceGridder2D::InverseDistanceGridder2D()
    : radius_( mUdf(float) )
{}


InverseDistanceGridder2D::InverseDistanceGridder2D(
	const InverseDistanceGridder2D& g )
    : Gridder2D( g )
    , radius_( g.radius_ )
{}


Gridder2D* InverseDistanceGridder2D::clone() const
{ return new InverseDistanceGridder2D( *this ); }


bool InverseDistanceGridder2D::operator==( const Gridder2D& b ) const
{
    if ( !Gridder2D::operator==( b ) )
	return false;

    mDynamicCastGet( const InverseDistanceGridder2D*, bidg, &b );
    if ( !bidg )
	return false;

    if ( mIsUdf(radius_) && mIsUdf( bidg->radius_ ) )
	return true;

    return mIsEqual(radius_,bidg->radius_, 1e-5 );
}


void InverseDistanceGridder2D::setSearchRadius( float r )
{
    if ( r <= 0 )
	return;

    radius_ = r;
}


bool InverseDistanceGridder2D::isPointUsable( const Coord& calcpt,
					      const Coord& datapt ) const
{
    if ( !datapt.isDefined() || !calcpt.isDefined() )
	return false;

    return mIsUdf(radius_) || calcpt.sqDistTo( datapt ) < radius_*radius_;
}


bool InverseDistanceGridder2D::getWeights( const Coord& gridpoint,
					   TypeSet<double>& weights,
					   TypeSet<int>& relevantpoints ) const
{
    weights.setEmpty();
    relevantpoints.setEmpty();
    const int sz = usedpoints_.size();
    if ( !gridpoint.isDefined() || !points_ || !sz )
	return false;

    const bool useradius = !mIsUdf(radius_);
    double weightsum = 0.;
    for ( int idx=0; idx<sz; idx++ )
    {
	const int index = usedpoints_[idx];
	if ( !points_->validIdx(index) )
	    continue;

	const Coord& pos = (*points_)[index];
	const double dist = gridpoint.distTo( pos );
	if ( useradius && dist > radius_ )
	    continue;

	relevantpoints += index;
	const double weight = useradius ? 1.-dist/radius_ : 1./dist;
	weightsum += weight;
	weights += weight;
    }

    for ( int idx=0; idx<weights.size(); idx++ )
	weights[idx] /= weightsum;

    return !relevantpoints.isEmpty();
}


bool InverseDistanceGridder2D::usePar( const IOPar& par )
{
    Gridder2D::usePar( par );
    float radius;
    if ( !par.get( sKeySearchRadius(), radius ) )
	return false;

    setSearchRadius( radius );
    return true;
}


void InverseDistanceGridder2D::fillPar( IOPar& par ) const
{
    Gridder2D::fillPar( par );
    par.set( sKeySearchRadius(), getSearchRadius() );
}



TriangulatedGridder2D::TriangulatedGridder2D()
    : triangles_( 0 )
    , interpolator_( 0 )
    , xrg_( mUdf(float), mUdf(float) )
    , yrg_( mUdf(float), mUdf(float) )
    , center_( 0, 0 )
{}


TriangulatedGridder2D::TriangulatedGridder2D(
				const TriangulatedGridder2D& g )
    : Gridder2D( g )
    , triangles_( 0 )
    , interpolator_( 0 )
    , xrg_( g.xrg_ )
    , yrg_( g.yrg_ )
    , center_( g.center_ )
{
    if ( g.triangles_ )
    {
	triangles_ = new DAGTriangleTree( *g.triangles_ );
	interpolator_ = new Triangle2DInterpolator( *triangles_ );
    }
}


TriangulatedGridder2D::~TriangulatedGridder2D()
{
    delete trend_;
    delete triangles_;
    delete interpolator_;
}


void TriangulatedGridder2D::setGridArea( const Interval<float>& xrg,
					 const Interval<float>& yrg )
{
    xrg_ = xrg;
    yrg_ = yrg;
}


Gridder2D* TriangulatedGridder2D::clone() const
{ return new TriangulatedGridder2D( *this ); }


bool TriangulatedGridder2D::getWeights( const Coord& gridpoint,
					TypeSet<double>& weights,
					TypeSet<int>& relevantpoints ) const
{
    weights.setEmpty();
    relevantpoints.setEmpty();
    const int sz = usedpoints_.size();
    if ( !gridpoint.isDefined() || !points_ || !sz )
	return false;

    Interval<double> xrg, yrg;
    if ( !DAGTriangleTree::computeCoordRanges( *points_, xrg, yrg ) )
	return false;

    if ( !triangles_ || !xrg.includes(gridpoint.x,false) ||
			!yrg.includes(gridpoint.y,false) )
    {
	//fallback to inverse distance without radius
	relevantpoints = usedpoints_;
	double weightsum = 0.;
	for ( int idx=0; idx<sz; idx++ )
	{
	    const int index = usedpoints_[idx];
	    if ( !points_->validIdx(index) )
		continue;

	    const Coord& pos = (*points_)[index];
	    const double weight = 1. / gridpoint.distTo( pos );
	    weightsum += weight;
	    weights += weight;
	}

	for ( int idx=0; idx<weights.size(); idx++ )
	    weights[idx] /= weightsum;

	return true;
    }

    const Coord relgridpoint( gridpoint-center_ );
    if ( !interpolator_->computeWeights(relgridpoint,relevantpoints,weights) )
	return false;

    return true;
}


bool TriangulatedGridder2D::pointsChangedCB( CallBacker* )
{
    delete triangles_;
    triangles_ = new DAGTriangleTree;
    Interval<double> xrg, yrg;
    if ( !DAGTriangleTree::computeCoordRanges( *points_, xrg, yrg ) )
    {
	delete triangles_;
	triangles_ = 0;
	return false;
    }

    if ( !mIsUdf(xrg_.start) ) xrg.include( xrg_.start );
    if ( !mIsUdf(xrg_.stop) ) xrg.include( xrg_.stop );
    if ( !mIsUdf(yrg_.start) ) yrg.include( yrg_.start );
    if ( !mIsUdf(yrg_.stop) ) yrg.include( yrg_.stop );

    TypeSet<Coord>* translatedpoints =
	new TypeSet<Coord>( points_->size(), Coord::udf() );

    center_.x = xrg.center();
    center_.y = yrg.center();

    for ( int idx=points_->size()-1; idx>=0; idx-- )
	(*translatedpoints)[idx] = (*points_)[idx]-center_;

    if ( !triangles_->setCoordList( translatedpoints, OD::TakeOverPtr ) )
    {
	delete triangles_;
	triangles_ = 0;
	return false;
    }

    xrg.shift( -center_.x );
    yrg.shift( -center_.y );

    if ( !triangles_->setBBox( xrg, yrg ) )
    {
	delete triangles_;
	triangles_ = 0;
	return false;
    }

    DelaunayTriangulator triangulator( *triangles_ );
    triangulator.dataIsRandom( false );
    if ( !triangulator.executeParallel( false ) )
    {
	delete triangles_;
	triangles_ = 0;
	return false;
    }

    delete interpolator_;

    interpolator_ = new Triangle2DInterpolator( *triangles_ );

    return true;
}



RadialBasisFunctionGridder2D::RadialBasisFunctionGridder2D()
    : globalweights_(0)
    , solv_(0)
    , ismetric_(false)
{}


RadialBasisFunctionGridder2D::RadialBasisFunctionGridder2D(
	const RadialBasisFunctionGridder2D& g )
    : Gridder2D( g )
    , globalweights_(0)
    , solv_(0)
    , ismetric_(g.ismetric_)
{
    if ( g.globalweights_ )
	globalweights_ = new TypeSet<double>( *g.globalweights_ );

    if ( g.solv_ )
	solv_ = new LinSolver<double> ( *g.solv_ );
}


RadialBasisFunctionGridder2D::~RadialBasisFunctionGridder2D()
{
    delete globalweights_;
    delete solv_;
}


Gridder2D* RadialBasisFunctionGridder2D::clone() const
{ return new RadialBasisFunctionGridder2D( *this ); }


bool RadialBasisFunctionGridder2D::operator==( const Gridder2D& b ) const
{
    if ( !Gridder2D::operator==( b ) )
	return false;

    mDynamicCastGet( const RadialBasisFunctionGridder2D*, bidg, &b );
    if ( !bidg )
	return false;

    if ( ismetric_ != bidg->ismetric_ ||
	 !mIsEqual(m11_,bidg->m11_,m11_*mDefEps) ||
	 !mIsEqual(m12_,bidg->m12_,m12_*mDefEps) ||
	 !mIsEqual(m22_,bidg->m22_,m22_*mDefEps) )
	return false;

    return true;
}


bool RadialBasisFunctionGridder2D::pointsChangedCB( CallBacker* )
{
    updateSolver();

    return true;
}


void RadialBasisFunctionGridder2D::valuesChangedCB( CallBacker* )
{
    updateSolution();
}


void RadialBasisFunctionGridder2D::setMetricTensor( double m11, double m12,
						    double m22 )
{
    if ( (mIsUdf(m11) || mIsUdf(m12) || mIsUdf(m22)) ||
	 ( ( m11*m22 >= m12*m12 ) ||
	 ( mIsEqual(m11,1.0,mDefEps) || mIsZero(m12,mDefEps) ||
	   mIsEqual(m22,1.0,mDefEps) ) ) )
    {
	ismetric_ = false;
	return;
    }

    m11_ = m11;
    m12_ = m12;
    m22_ = m22;
    ismetric_ = true;
}


bool RadialBasisFunctionGridder2D::getWeights( const Coord& gridpoint,
					       TypeSet<double>& weights,
					   TypeSet<int>& relevantpoints ) const
{
    const int sz = usedpoints_.size();
    if ( !gridpoint.isDefined() || !points_ || !sz )
	return false;

    if ( !globalweights_ || globalweights_->size() != sz )
	return false;

    weights.setSize( sz, 0. );
    relevantpoints = usedpoints_;
    for ( int idx=0; idx<sz; idx++ )
    {
	const int index = relevantpoints[idx];
	if ( !points_->validIdx(index) )
	    continue;

	const Coord& pos = (*points_)[index];
	weights[idx] = (*globalweights_)[idx] *
		       evaluateRBF( getRadius(gridpoint,pos) );
    }

    return true;
}


float RadialBasisFunctionGridder2D::getValue( const Coord& gridpoint,
				   const TypeSet<double>* inpweights,
				   const TypeSet<int>* inprelevantpoints ) const
{
    int exactpos;
    if ( isAtInputPos(gridpoint,exactpos) )
	return (*values_)[exactpos];

    TypeSet<double> weights;
    TypeSet<int> relevantpoints;
    const bool needweight = !inpweights || !inprelevantpoints ||
			    inpweights->size() != inprelevantpoints->size();
    if ( needweight && !getWeights(gridpoint,weights,relevantpoints) )
	return mUdf(float);

    const TypeSet<double>* curweights = needweight ? &weights : inpweights;
    double val = 0.;
    for ( int idx=0; idx<curweights->size(); idx++ )
	val += (*curweights)[idx];

    if ( trend_ )
	trend_->apply( gridpoint, false, val );

    return mCast(float,val);
}


bool RadialBasisFunctionGridder2D::updateSolver()
{
    delete solv_;
    delete globalweights_; //previous solution is invalid too
    int sz = usedpoints_.size();
    if ( !points_ || !sz )
	return false;

    for ( int idx=0; idx<sz; idx++ )
    {
	if ( !points_->validIdx(usedpoints_[idx]) )
	    return false;
    }

    Array2DImpl<double> a( sz, sz );
    for ( int idx=0; idx<sz; idx++ )
    {
	const int indexX = usedpoints_[idx];
	const Coord& posX = (*points_)[indexX];
	for ( int idy=0; idy<sz; idy++ )
	{
	    const int indexY = usedpoints_[idy];
	    const Coord& posY = (*points_)[indexY];
	    const double val = evaluateRBF( getRadius( posX, posY ) );
	    a.set( idx, idy, val );
	}
    }

    solv_ = new LinSolver<double>( a );

    return true;
}


bool RadialBasisFunctionGridder2D::updateSolution()
{
    delete globalweights_;
    if ( !values_ || values_->isEmpty() )
	return false;

    int sz = usedpoints_.size();
    if ( !solv_ || solv_->size() != sz )
	return false;

    Array1DImpl<double> b( sz );
    for ( int idx=0; idx<sz; idx++ )
    {
	const float val = getDetrendedValue( usedpoints_[idx] );
	if ( mIsUdf(val) )
	    return false;

	b.set( idx, val );
    }

    mDeclareAndTryAlloc(double*,x,double[sz])
    solv_->apply( b.getData(), x );

    globalweights_ = new TypeSet<double>( sz, mUdf(float) );
    for ( int idx=0; idx<sz; idx++ )
	(*globalweights_)[idx] = x[idx];

    delete [] x;
    return true;
}


double RadialBasisFunctionGridder2D::getRadius( const Coord& pos1,
						const Coord& pos2 ) const
{
    const double xdiff = pos1.x - pos2.x;
    const double ydiff = pos1.y - pos2.y;
    if ( ismetric_ )
    {
	return Math::Sqrt( m11_ * xdiff * xdiff +
			   2.0 * m12_ * xdiff * ydiff +
			   m22_ * ydiff * ydiff );
    }

    return Math::Sqrt( xdiff*xdiff + ydiff*ydiff );
}


double RadialBasisFunctionGridder2D::evaluateRBF( double radius, double scale )
{
    if ( radius < mDefEps )
	return 0.;

    radius *= scale;
    return radius * radius * ( log(radius) - 1. );
}
