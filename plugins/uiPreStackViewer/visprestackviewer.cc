/*+
_______________________________________________________________________________

 COPYRIGHT:	(C) dGB Beheer B.V.
 AUTHOR:	Yuancheng Liu
 DAT:		May 2007
 RCS:           $Id: visprestackviewer.cc,v 1.12 2008-02-05 16:11:05 cvsyuancheng Exp $
_______________________________________________________________________________

 -*/

#include "visprestackviewer.h"

#include "ioman.h"
#include "iopar.h"
#include "prestackgather.h"
#include "survinfo.h"
#include "viscoord.h"
#include "visdataman.h"
#include "visdepthtabplanedragger.h"
#include "visfaceset.h"
#include "visflatviewer.h"
#include "vismaterial.h"
#include "vispickstyle.h"
#include "visplanedatadisplay.h"
#include "visseis2ddisplay.h"

#include "seispsioprov.h"

mCreateFactoryEntry( PreStackView::PreStackViewer );

#define mDefaultWidth ((SI().inlDistance() + SI().crlDistance() ) * 100)

namespace PreStackView
{

PreStackViewer::PreStackViewer()
    : VisualObjectImpl( true )
    , draggerrect_( visBase::FaceSet::create() )
    , pickstyle_( visBase::PickStyle::create() )
    , planedragger_( visBase::DepthTabPlaneDragger::create() )	
    , flatviewer_( visBase::FlatViewer::create() )
    , bid_(-1,-1)
    , mid_( 0 )		
    , section_( 0 )
    , seis2d_( 0 )
    , factor_( 1 )
    , trcnr_( 0 )
    , basedirection_( mUdf(float), mUdf(float) )		 
    , seis2dpos_( mUdf(float), mUdf(float) )		 
    , width_( mDefaultWidth )
    , offsetrange_( 0, mDefaultWidth )
    , zrg_( SI().zRange(true) )
    , posside_( true )
    , autowidth_( true )
{
    setMaterial( 0 );
    planedragger_->ref();
    planedragger_->removeScaleTabs();
    planedragger_->motion.notify( mCB( this, PreStackViewer, draggerMotion ) );
    planedragger_->finished.notify( mCB( this, PreStackViewer, finishedCB ) );
    addChild( planedragger_->getInventorNode() );
    
    draggerrect_->ref();
    draggerrect_->removeSwitch();
    draggerrect_->setVertexOrdering( 
	    visBase::VertexShape::cCounterClockWiseVertexOrdering() );
    draggerrect_->setShapeType( 
	    visBase::VertexShape::cUnknownShapeType() );
    draggerrect_->getCoordinates()->addPos( Coord3( -1,-1,0 ) );
    draggerrect_->getCoordinates()->addPos( Coord3( 1,-1,0 ) );
    draggerrect_->getCoordinates()->addPos( Coord3( 1,1,0 ) );
    draggerrect_->getCoordinates()->addPos( Coord3( -1,1,0 ) );
    draggerrect_->setCoordIndex( 0, 0 );
    draggerrect_->setCoordIndex( 1, 1 );
    draggerrect_->setCoordIndex( 2, 2 );
    draggerrect_->setCoordIndex( 3, 3 );
    draggerrect_->setCoordIndex( 4, -1 );

    draggermaterial_ = visBase::Material::create();
    draggermaterial_->ref();
    draggerrect_->setMaterial( draggermaterial_ );
    draggermaterial_->setTransparency( 0.5 ); 

    planedragger_->setOwnShape( draggerrect_->getInventorNode() );
    
    pickstyle_->ref();
    addChild( pickstyle_->getInventorNode() );
    pickstyle_->setStyle( visBase::PickStyle::Unpickable );

    flatviewer_->ref();
    flatviewer_->setSelectable( false );
    flatviewer_->removeSwitch();
    flatviewer_->appearance().setGeoDefaults( true );

    flatviewer_->dataChange.notify( mCB( this,PreStackViewer,dataChangedCB ) );
    addChild( flatviewer_->getInventorNode() );
}


PreStackViewer::~PreStackViewer()
{
    pickstyle_->unRef();
    draggerrect_->unRef();
    draggermaterial_->unRef();
    
    if ( section_ )
    {
	if ( planedragger_ )
	{ 
	    planedragger_->motion.remove( 
		    mCB(this,PreStackViewer,draggerMotion) );
	    planedragger_->finished.remove( 
		    mCB(this,PreStackViewer,finishedCB) );
	    planedragger_->unRef();
	}

    	section_->getMovementNotifier()->remove( 
    		mCB( this, PreStackViewer, sectionMovedCB ) );
    	section_->unRef();
    }
    
    if ( seis2d_ )
    {
	if ( seis2d_->getMovementNotifier() )
    	    seis2d_->getMovementNotifier()->remove( 
    		    mCB( this, PreStackViewer, seis2DMovedCB ) );
	seis2d_->unRef();
    }

    flatviewer_->dataChange.remove( mCB( this, PreStackViewer, dataChangedCB ));
    flatviewer_->unRef();
}


void PreStackViewer::allowShading( bool yn )
{ flatviewer_->allowShading( yn ); }


void PreStackViewer::setColor( Color nc )
{ getMaterial()->setColor(nc); }


void  PreStackViewer::setMultiID( const MultiID& mid )
{ mid_ = mid; }
 

bool PreStackViewer::setPosition( const BinID& nb )
{
    if ( nb.inl==-1 || nb.crl==-1 )
    	turnOn(false);
    else
    {
	const bool haddata = flatviewer_->pack( false );
	bid_ = nb;
	PreStack::Gather* gather = new PreStack::Gather;
	if ( !gather->readFrom( mid_, bid_ ) )
	{
	    delete gather;
	    if ( haddata )
		flatviewer_->setPack( false, DataPack::cNoID, false );
	    else
	    {
		dataChangedCB( 0 );
		return false;
	    }
	}
	else
	{
	    DPM(DataPackMgr::FlatID).add( gather );
	    flatviewer_->setPack( false, gather->id(), false, !haddata );
	}
	
	turnOn( true );
    }

    return true;
}


void PreStackViewer::displaysAutoWidth( bool yn )
{
    if ( autowidth_ == yn )
	return;

    autowidth_ = yn;
    dataChangedCB( 0 );
}


void PreStackViewer::displaysOnPositiveSide( bool yn )
{
    if ( posside_ == yn )
	return;

    posside_ = yn;
    dataChangedCB( 0 );
}


void PreStackViewer::setFactor( float scale )
{
    if (  factor_ == scale )
	return;

    factor_ = scale;
    dataChangedCB( 0 );
}

void PreStackViewer::setWidth( float width )
{
    if ( width_ == width )
	return;

    width_ = width;
    dataChangedCB( 0 );
}


void PreStackViewer::dataChangedCB( CallBacker* )
{
    if ( !section_ && !seis2d_ || factor_<0 || width_<0 )
	return;

    const Coord direction = posside_ ? basedirection_ : -basedirection_;
    const float offsetscale = Coord( basedirection_.x*SI().inlDistance(),
	    			     basedirection_.y*SI().crlDistance()).abs();

    const FlatDataPack* fdp = flatviewer_->pack( false );
    if ( fdp )
    {
	offsetrange_.setFrom( fdp->posData().range( true ) );
	zrg_.setFrom( fdp->posData().range( false ) );
    }


    Coord startpos( bid_.inl, bid_.crl );
    if ( seis2d_ )
	startpos = seis2dpos_;

    const Coord stoppos = autowidth_
	? startpos + direction*offsetrange_.width()*factor_ / offsetscale
	: startpos + direction*width_ / offsetscale;

    if ( autowidth_ )
	width_ = offsetrange_.width()*factor_;
    else
	factor_ = width_/offsetrange_.width();

    const Coord3 c00( startpos, zrg_.start );
    const Coord3 c01( stoppos, zrg_.start ); 
    const Coord3 c11( stoppos, zrg_.stop );
    const Coord3 c10( startpos, zrg_.stop );


    flatviewer_->setPosition( c10, c11, c00, c01 );
    if ( section_ )
    {
	planedragger_->setDim( section_->getOrientation() 
		==visSurvey::PlaneDataDisplay::Inline ? 1:0 );
    
	const Coord3 width( fabs(stoppos.x-startpos.x),
		fabs(stoppos.y-startpos.y), zrg_.width(true) );
    	planedragger_->setSize( width );
	
    	const Coord3 center( (startpos+stoppos)/2, (zrg_.start+zrg_.stop)/2 );
    	planedragger_->setCenter( center );
	
    	const Interval<float> xlim = Interval<float>( 
		SI().inlRange( true ).start, SI().inlRange( true ).stop );
    	const Interval<float> ylim = Interval<float>( 
		SI().crlRange( true ).start, SI().crlRange( true ).stop ); 
	
    	planedragger_->setSpaceLimits( xlim, ylim, SI().zRange( true ) );    
    }
    
    draggermaterial_->setTransparency( 1 ); 
}


const BinID& PreStackViewer::getPosition() const 
{ return bid_; }


const visSurvey::PlaneDataDisplay* PreStackViewer::getSectionDisplay() const
{ return section_;}


void PreStackViewer::setDisplayTransformation( visBase::Transformation* nt )
{ 
    flatviewer_->setDisplayTransformation( nt ); 
    if ( planedragger_ )
	planedragger_->setDisplayTransformation( nt );

    if ( seis2d_ )
	seis2d_->setDisplayTransformation( nt );
}


void PreStackViewer::setSectionDisplay( visSurvey::PlaneDataDisplay* pdd )
{
    if ( section_ )
    {
	if ( section_->getMovementNotifier() )
	    section_->getMovementNotifier()->remove(
		    mCB( this, PreStackViewer, sectionMovedCB ) );
	section_->unRef();
    }

    section_ = pdd;
    if ( !section_ )
	return;

    section_->ref();

    const bool offsetalonginl = 
	section_->getOrientation()==visSurvey::PlaneDataDisplay::Crossline;
    basedirection_ = offsetalonginl ? Coord( 0, 1  ) : Coord( 1, 0 );

    if ( section_->getOrientation() == visSurvey::PlaneDataDisplay::Timeslice )
	return;
    
    if ( section_->getMovementNotifier() )
	section_->getMovementNotifier()->notify(
		mCB( this, PreStackViewer, sectionMovedCB ) );
}


void  PreStackViewer::sectionMovedCB( CallBacker* )
{
    BinID newpos = bid_;

    if ( !section_ )
	return;
    else
    {
    	if ( section_->getOrientation() == visSurvey::PlaneDataDisplay::Inline )
	    newpos.inl = section_->getCubeSampling( -1 ).hrg.start.inl;
    	else if ( section_->getOrientation() == 
		visSurvey::PlaneDataDisplay::Crossline )
	    newpos.crl = section_->getCubeSampling( -1 ).hrg.start.crl;
    	else
	    return;
    }

    if ( !setPosition(newpos) )
	return;
}    


const visSurvey::Seis2DDisplay* PreStackViewer::getSeis2DDisplay() const
{ return seis2d_; }


void PreStackViewer::setSeis2DData( DataPack::ID dpid )
{
    flatviewer_->setPack( false, dpid, false, true );
    dataChangedCB(0);
}


DataPack::ID PreStackViewer::getDataPackID() const
{
    return flatviewer_->packID( false );
}


bool PreStackViewer::is3DSeis() const
{
    if ( section_ )
	return true;
    else
	return false;
}


void PreStackViewer::setSeis2DDisplay(visSurvey::Seis2DDisplay* s2d, int trcnr)
{
    if ( planedragger_ )
    { 
     	planedragger_->motion.remove( mCB(this,PreStackViewer,draggerMotion) );
    	planedragger_->finished.remove( mCB(this,PreStackViewer,finishedCB) );
    	planedragger_->unRef();
    }

    pickstyle_->setStyle( visBase::PickStyle::Shape );
    if ( seis2d_ ) 
    {
	if ( seis2d_->getMovementNotifier() )
    	    seis2d_->getMovementNotifier()->remove( 
    		    mCB( this, PreStackViewer, seis2DMovedCB ) );

	seis2d_->unRef();
    }

    trcnr_ = trcnr;
    seis2d_ = s2d;
    
    const Coord orig = SI().binID2Coord().transformBackNoSnap( Coord(0,0) );
    basedirection_ = SI().binID2Coord().transformBackNoSnap(
	    seis2d_->getNormal( trcnr_ ) ) - orig;
    seis2dpos_ = SI().binID2Coord().transformBackNoSnap( 
	    seis2d_->getCoord(trcnr))-orig;
    if ( !seis2d_ )
	return;

    seis2d_->ref();
    if ( seis2d_->getMovementNotifier() )
	seis2d_->getMovementNotifier()->notify( 
    		    mCB( this, PreStackViewer, seis2DMovedCB ) );
}


void  PreStackViewer::seis2DMovedCB( CallBacker* )
{
    if ( !seis2d_ )
	return;
    
    const Coord orig = SI().binID2Coord().transformBackNoSnap( Coord(0,0) );
    basedirection_ = SI().binID2Coord().transformBackNoSnap(
	    seis2d_->getNormal( trcnr_ ) ) - orig;

    seis2dpos_ = SI().binID2Coord().transformBackNoSnap( 
	    seis2d_->getCoord(trcnr_))-orig;

    dataChangedCB(0);
}    


const char* PreStackViewer::lineName()
{
    if ( !seis2d_ )
	return 0;

    return seis2d_->name();
}


void  PreStackViewer::otherObjectsMoved( const ObjectSet<const SurveyObject>&
					 , int whichobj )
{
    if ( !section_ && ! seis2d_ )
	return;

    if ( whichobj == -1 )
    {
	if ( section_ )
    	    turnOn( section_->isShown() );

	if ( seis2d_ )
	    turnOn( seis2d_->isShown() );
	return; 
    }

    if ( section_ && section_->id() != whichobj ||
	 seis2d_ && seis2d_->id() != whichobj )
	return;
    
    if ( section_ )
	turnOn( section_->isShown() );

    if ( seis2d_ )
	turnOn( seis2d_->isShown() );
}


void PreStackViewer::draggerMotion( CallBacker* )
{
    if ( !section_ )
	return;
    
    const int newinl = SI().inlRange( true ).snap( planedragger_->center().x );
    const int newcrl = SI().inlRange( true ).snap( planedragger_->center().y );

    const visSurvey::PlaneDataDisplay::Orientation orientation =
	    section_->getOrientation();

    bool showplane = false;
    if ( orientation==visSurvey::PlaneDataDisplay::Inline && newcrl!=bid_.crl )
        showplane = true;
    else if ( orientation==visSurvey::PlaneDataDisplay::Crossline &&
	      newinl!=bid_.inl )
	showplane = true;

    draggermaterial_->setTransparency( showplane ? 0.5 : 1 );
}


void  PreStackViewer::finishedCB( CallBacker* )
{ 
    if ( !section_ )
	return;

    int inl = SI().inlRange( true ).snap( planedragger_->center().x );
    int crl = SI().inlRange( true ).snap( planedragger_->center().y );
    BinID newpos = BinID( inl, crl );
    
    if ( section_->getOrientation() == visSurvey::PlaneDataDisplay::Inline )
	newpos.inl = section_->getCubeSampling( -1 ).hrg.start.inl;
    else if (section_->getOrientation() == 
	    visSurvey::PlaneDataDisplay::Crossline )
	newpos.crl = section_->getCubeSampling( -1 ).hrg.start.crl;
    else
	return;

    if ( !setPosition(newpos) )
	return;
}


void PreStackViewer::getMousePosInfo( const visBase::EventInfo& ei,
				      const Coord3& pos, 
				      BufferString& val,
				      BufferString& info ) const
{
    val = "";
    info = "";
    if ( !flatviewer_  ) 
	return;

    if ( seis2d_ )
    {
	seis2d_->getMousePosInfo( ei, pos, val, info );
	return;
    }

    if ( !section_ )
	return;

    const FlatDataPack* fdp = flatviewer_->pack(false);
    if ( !fdp ) return;

    const FlatPosData& posdata = fdp->posData();
    const BinID bid = SI().transform( pos );
    const float distance = sqrt(bid_.sqDistTo( bid ));
    float offset = mUdf(float);
    
    if ( SI().inlDistance()==0 || SI().crlDistance()==0 || width_==0 )
	return;

    float start = posdata.range( true ).start;
    float cal = posdata.width(true)*distance/width_;
    if ( section_->getOrientation()==visSurvey::PlaneDataDisplay::Inline )
	offset = cal*SI().inlDistance()+start;
    else
	offset= cal*SI().crlDistance()+start;
   
    int offsetsample;
    float traceoffset;
    if ( posdata.isIrregular() )
    {
	float mindist;
	for ( int idx=0; idx<posdata.nrPts(true); idx++ )
	{
	    const float dist = fabs( posdata.position(true,idx)-offset );
	    if ( !idx || dist<mindist )
	    {
		offsetsample = idx;
		mindist = dist;
		traceoffset = posdata.position(true,idx);
	    }
	}
    }
    else
    {
	const StepInterval<double>& rg = posdata.range( true );
	offsetsample = rg.nearestIndex( offset );
	traceoffset = rg.atIndex( offsetsample );
    }

    info = "Offset: ";
    info += traceoffset;

    const int zsample = posdata.range(false).nearestIndex( pos.z );
    val = fdp->data().get( offsetsample, zsample );
}


void PreStackViewer::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
   if ( !section_ && !seis2d_ )
	return;

    SurveyObject::fillSOPar( par );
    VisualObjectImpl::fillPar( par, saveids );
    if ( section_ )
    {
	saveids.addIfNew( section_->id() );
        par.set( sKeySectionID(), section_->id() );
	par.set( sKeyBinID(), bid_ );
    }

    if  ( seis2d_ )
    {
	saveids.addIfNew( seis2d_->id() );
    	par.set( sKeySeis2DID(), seis2d_->id() );
	par.set( sKeyTraceNr(), trcnr_ );
	par.set( sKeyLineName(), seis2d_->name() );
    }
    
    par.set( sKeyMultiID(), mid_ );
    par.setYN( sKeyiAutoWidth(), autowidth_ );
    par.setYN( sKeySide(), posside_ );
    
    if ( autowidth_ )
	par.set( sKeyFactor(), factor_ );
    else
	par.set( sKeyWidth(), width_ );
}


int PreStackViewer::usePar( const IOPar& par )
{
    int res =  VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    res = SurveyObject::useSOPar( par );
    if ( res!=1 ) return res;

    int parentid = -1;
    const bool is2d = par.get( sKeySeis2DID(), parentid );
    if ( !is2d )
    {
	if ( !par.get(sKeySectionID(),parentid) )
	    return -1;
    }

    visBase::DataObject* parent = visBase::DM().getObject( parentid );
    if ( !parent )
	return 0;

    MultiID mid;
    if ( !par.get(sKeyMultiID(),mid) ) 
	return -1;
    
    setMultiID( mid );

    mDynamicCastGet( visSurvey::PlaneDataDisplay*, pdd, parent );
    mDynamicCastGet( visSurvey::Seis2DDisplay*, s2d, parent );
    if ( !pdd && !s2d )
	return -1;
    
    if ( pdd )
    {	
    	setSectionDisplay( pdd );
	BinID bid;
    	if ( !par.get(sKeyBinID(),bid) || !setPosition( bid ) )
    	    return -1;
    }

    if ( s2d )
    {
	int tnr;
	if ( !par.get(sKeyTraceNr(),tnr) )
	    return -1;
	
	PtrMan<SeisPSReader> psrdr = SPSIOPF().get2DReader( *IOM().get(mid), 
		s2d->name() );

	if ( !psrdr ) 
	    return -1;
	
	SeisTrcBuf* tbuf = new SeisTrcBuf( true );
	if ( !psrdr->getGather( BinID(0,tnr), *tbuf ) )
	    return -1;
	
	if ( tbuf->size() == 0 )
	    return -1;

	SeisTrcBufDataPack* dp = new SeisTrcBufDataPack( tbuf, Seis::LinePS, 
		SeisTrcInfo::Offset, "Pre-Stack Gather", 0 );

	DPM(DataPackMgr::FlatID).add( dp );
	DPM(DataPackMgr::FlatID).obtain( dp->id() );
	setSeis2DData( dp->id() );
	DPM(DataPackMgr::FlatID).release( dp );
	
	setSeis2DDisplay( s2d, tnr );
    }
       
    float factor, width;
    if ( par.get(sKeyFactor(), factor) )
	setFactor( factor );

    if ( par.get(sKeyWidth(), width) )
	setWidth( width );

    bool autowidth, side;
    if ( par.getYN(sKeyiAutoWidth(), autowidth) )
	 displaysAutoWidth( autowidth );

    if ( par.getYN(sKeySide(), side) )
	displaysOnPositiveSide( side );
	
    return 1;
}


}; //namespace
