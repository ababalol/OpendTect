/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Dec 2008
________________________________________________________________________

-*/

#include "uihistogramdisplay.h"

#include "uiaxishandler.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uimsg.h"
#include "uistrings.h"

#include "arrayndimpl.h"
#include "bufstring.h"
#include "datadistribution.h"
#include "datapointset.h"
#include "statparallelcalc.h"

uiHistogramDisplay::uiHistogramDisplay( uiParent* p,
					uiHistogramDisplay::Setup& su,
					bool withheader	)
    : uiFunctionDisplay( p, su.fillbelow(true).yannotinint(true)
			    .noy2axis(true).noy2gridline(true)
			    .yrg(Interval<float>(0,mUdf(float))) )
    , rc_(*new Stats::ParallelCalc<float>(Stats::CalcSetup(false)
					    .require(Stats::Min)
					    .require(Stats::Max)
					    .require(Stats::Average)
					    .require(Stats::Median)
					    .require(Stats::StdDev)
					    .require(Stats::RMS)) )
    , nrinpvals_(0)
    , nrclasses_(0)
    , withheader_(withheader)
    , header_(0)
    , nitm_(0)
    , mydrawrg_(mUdf(float),mUdf(float))
    , usemydrawrg_(false)
    , drawRangeChanged(this)
{
    xAxis()->setCaption( uiStrings::sValue() );
    yAxis(false)->setCaption( uiStrings::sCount() );
}


uiHistogramDisplay::~uiHistogramDisplay()
{
    delete &rc_; delete header_; delete nitm_;
}


static int getNrIntervals( int nrpts )
{
    int res = nrpts / 25;
    if ( res < 10 && nrpts < 10 ) res = 10;
    else if ( res < 20 && nrpts < 20 ) res = 20;
    else if ( res < 50 && nrpts < 50 ) res = 50;
    else res = 100;

    return res;
}


void uiHistogramDisplay::setEmpty()
{
    uiFunctionDisplay::setEmpty();
    rc_.setEmpty();
    updateAndDraw();
}


bool uiHistogramDisplay::setDataPackID(
	DataPack::ID dpid, DataPackMgr::ID dmid, int version )
{
    rc_.setEmpty();
    ConstRefMan<DataPack> dp = DPM(dmid).get( dpid );
    if ( !dp ) return false;

    BufferString dpversionnm;

    if ( dmid == DataPackMgr::SeisID() )
    {
	mDynamicCastGet(const VolumeDataPack*,voldp,dp.ptr());
	if ( !voldp || voldp->isEmpty() ) return false;

	const Array3D<float>* arr3d = &voldp->data( version );
	dpversionnm = voldp->name();
	setData( arr3d );
    }
    else if ( dmid == DataPackMgr::FlatID() )
    {
	mDynamicCastGet(const FlatDataPack*,fdp,dp.ptr())
	mDynamicCastGet(const MapDataPack*,mdp,dp.ptr())
	if ( mdp )
	{
	    dpversionnm = mdp->name();
	    setData( &mdp->rawData() );
	}
	else if( fdp )
	{
	    dpversionnm = fdp->name();
	    setData( &fdp->data() );
	}
	else
	    return false;
    }
    else if ( dmid == DataPackMgr::SurfID() )
    {
	mDynamicCastGet(const DataPointSet*,dpset,dp.ptr())
	if ( !dpset )
	    return false;

	dpversionnm = dpset->name();
	setData( *dpset );
    }
    else
	return false;

    if ( withheader_ )
    {
	if ( !header_ )
	{
	    const uiPoint pt( width()/2, 0 );
	    header_ = scene().addItem( new uiTextItem(pt,
						toUiString(dpversionnm)) );
	    header_->setZValue( 2 );
	}
	else
	    header_->setText( toUiString(dpversionnm) );
    }

    return true;
}


void uiHistogramDisplay::setData( const DataPointSet& dpset )
{
    TypeSet<float> valarr;
    for ( int idx=0; idx<dpset.size(); idx++ )
    {
	const float val = dpset.value( 2, idx );
	if ( mIsUdf(val) )
	    continue;

	valarr += val;
    }

    setData( valarr.arr(), valarr.size() );
}


void uiHistogramDisplay::setData( const Array2D<float>* array )
{
    if ( !array )
	{ rc_.setEmpty(); return; }

    if ( array->getData() )
	{ setData( array->getData(), array->totalSize() ); return; }

    const int sz2d0 = array->getSize( 0 );
    const int sz2d1 = array->getSize( 1 );
    TypeSet<float> valarr;
    for ( int idx0=0; idx0<sz2d0; idx0++ )
    {
	for ( int idx1=0; idx1<sz2d1; idx1++ )
	{
	    const float val = array->get( idx0, idx1 );
	    if ( mIsUdf(val) )
		continue;

	    valarr += val;
	}
    }
    rc_.setValues( valarr.arr(), valarr.size() );
    updateAndDraw();
}


void uiHistogramDisplay::setData( const Array3D<float>* array )
{
    if ( !array )
	{ rc_.setEmpty(); return; }

    if ( array->getData() )
	{ setData( array->getData(), array->totalSize() ); return; }

    const int sz0 = array->getSize( 0 );
    const int sz1 = array->getSize( 1 );
    const int sz2 = array->getSize( 2 );
    TypeSet<float> valarr;
    for ( int idx0=0; idx0<sz0; idx0++ )
	for ( int idx1=0; idx1<sz1; idx1++ )
	    for ( int idx2=0; idx2<sz2; idx2++ )
	    {
		const float val = array->get( idx0, idx1, idx2 );
		if ( mIsUdf(val) ) continue;

		valarr += val;
	    }

    rc_.setValues( valarr.arr(), valarr.size() );
    updateAndDraw();
}


void uiHistogramDisplay::setData( const float* array, od_int64 sz )
{
    if ( !array || sz < 1 )
	{ rc_.setEmpty(); return; }

    if ( array != originaldata_.arr() )
    {
	originaldata_.erase();
	for ( int idx=0; idx<sz; idx++ )
	    originaldata_ += array[idx];
    }

    const bool usedrawrg = usemydrawrg_ && !mIsUdf(mydrawrg_.start) &&
	!mIsUdf(mydrawrg_.stop);
    if ( usedrawrg )
    {
	mydisplaydata_.erase();
	for ( int idx=0; idx<sz; idx++ )
	{
	    if ( mIsUdf(array[idx]) )
		continue;

	    if ( mydrawrg_.includes(array[idx],false) )
		mydisplaydata_ += array[idx];
	}

	rc_.setValues( mydisplaydata_.arr(), mydisplaydata_.size() );
    }
    else
	rc_.setValues( originaldata_.arr(), originaldata_.size() );

    updateAndDraw();
}


void uiHistogramDisplay::updateAndDraw()
{
    updateHistogram();
    draw();
}


void uiHistogramDisplay::updateHistogram()
{
    if ( !rc_.execute() )
	{ uiMSG().error( rc_.errMsg() ); return; }

    const int nrpts = rc_.count();
    nrclasses_ = getNrIntervals( nrpts );
    TypeSet<float> histdata( nrclasses_, 0 );
    const float min = rc_.min(); const float max = rc_.max();
    const float step = (max - min) / nrclasses_;
    if ( mIsZero(step,1e-6) )
    {
	histdata[nrclasses_/2] = mCast( float, nrpts );
	setHistogram( histdata, Interval<float>(min-1,max+1), nrpts );
	return;
    }

    nrinpvals_ = 0;
    for ( int idx=0; idx<nrpts; idx++ )
    {
	int seg = (int)((rc_.medvals_[idx] - min) / step);
	if ( seg < -1 || seg > nrclasses_ )
	   { pErrMsg("Huh"); continue; }

	if ( seg < 0 )			seg = 0;

	if ( seg == nrclasses_ )	seg = nrclasses_ - 1;

	histdata[seg] += 1; nrinpvals_++;
    }

    setHistogram( histdata, Interval<float>(min + 0.5f*step, max - 0.5f*step),
		  nrinpvals_ );
}


void uiHistogramDisplay::useDrawRange( bool yn )
{
   if ( usemydrawrg_==yn )
      return;

    usemydrawrg_ = yn;
    if ( usemydrawrg_ && !mIsUdf(mydrawrg_.start) && !mIsUdf(mydrawrg_.stop) )
    {
	setData( originaldata_.arr(), originaldata_.size() );
	drawRangeChanged.trigger();
    }
}


void uiHistogramDisplay::setDrawRange( const Interval<float>& ni )
{
    if ( mIsEqual(ni.start,mydrawrg_.start,1e-5) &&
	 mIsEqual(ni.stop,mydrawrg_.stop,1e-5) )
	return;

    mydrawrg_ = ni;
    if ( usemydrawrg_ )
    {
	setData( originaldata_.arr(), originaldata_.size() );
	drawRangeChanged.trigger();
    }
}



void uiHistogramDisplay::setHistogram( const TypeSet<float>& histdata,
				       Interval<float> xrg, int nrvals )
{
    nrinpvals_ = nrvals;
    setVals( xrg, histdata.arr(), histdata.size() );
}


void uiHistogramDisplay::setDistribution( const DataDistribution<float>& distr,
					  int nrvals )
{
    nrinpvals_ = nrvals;
    const int sz = distr.size();
    const SamplingData<float> sd = distr.sampling();
    const Interval<float> xrg( sd.start, sd.atIndex(sz-1) );
    const TypeSet<float> distrdata( distr.getSet(false) );
    setVals( xrg, distrdata.arr(), distrdata.size() );
}


void uiHistogramDisplay::putN()
{
    delete nitm_; nitm_ = 0;
    nitm_ = scene().addItem( new uiTextItem(uiPoint(width()/10,0),
						tr("N=%1").arg(nrinpvals_)) );
    nitm_->setPenColor( Color::Black() );
    nitm_->setZValue( 99999 );
}
