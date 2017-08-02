/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Satyaki Maitra
 * DATE     : April 2017
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "profilemodelfromeventdata.h"

#include "bendpointfinder.h"
#include "profilebase.h"
#include "profileposprovider.h"
#include "randomlinegeom.h"
#include "statruncalc.h"
#include "stratreftree.h"
#include "stratunitrefiter.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "uistrings.h"
#include "wellman.h"
#include "welldata.h"
#include "zvalueprovider.h"
#include <math.h>


bool ProfileModelFromEventData::Section::getSectionTKS(
	TrcKeySampling& sectiontks ) const
{
    sectiontks.init( false );
    if ( is2d_ )
    {
	sectiontks.set2DDef();
	const Survey::Geometry* geom = Survey::GM().getGeometry( geomid_ );
	if ( !geom )
	    return false;

	sectiontks = geom->sampling().hsamp_;
    }
    else
    {
	if ( linegeom_.isEmpty() )
	{
	    Geometry::RandomLine* rdlgeom = Geometry::RLM().get( rdmlinemid_ );
	    if ( !rdlgeom )
		return false;

	    TrcKeyPath linegeom;
	    rdlgeom->allNodePositions( linegeom );
	    for ( int ipos=0; ipos<linegeom.size(); ipos++ )
		sectiontks.include( linegeom[ipos] );
	}
	else
	{
	    for ( int ipos=0; ipos<linegeom_.size(); ipos++ )
		sectiontks.include( SI().transform(linegeom_[ipos]) );
	}
    }

    return true;
}


bool ProfileModelFromEventData::Section::fetchLineGeom()
{
    const bool hasgeom = is2d_ ? fetchLineGeom2D() : fetchLineGeom3D();
    if ( !hasgeom )
	return false;

    profposprov_ = new ProfilePosProviderFromLine( linegeom_ );
    return profposprov_;
}


bool ProfileModelFromEventData::Section::fetchLineGeom2D()
{
    const Survey::Geometry* geom = Survey::GM().getGeometry( geomid_ );
    mDynamicCastGet(const Survey::Geometry2D*,geom2d,geom)
    if ( !geom2d )
    {
	errmsg_ = uiStrings::phrCannotRead(
		tr("the line's geometry from the database") );
	return false;
    }

    TypeSet<Coord> coords;
    const PosInfo::Line2DData& l2dd = geom2d->data();
    for ( int idx=0; idx<l2dd.positions().size(); idx++ )
	coords += l2dd.positions()[idx].coord_;

    BendPointFinder2D bpfndr( coords, 1 );
    bpfndr.execute();
    for ( int idx=0; idx<bpfndr.bendPoints().size(); idx++ )
	linegeom_ += l2dd.positions()[ bpfndr.bendPoints()[idx] ].coord_;

    return true;
}


bool ProfileModelFromEventData::Section::fetchLineGeom3D()
{
    Geometry::RandomLine* rdlgeom = Geometry::RLM().get( rdmlinemid_ );
    if ( !rdlgeom )
    {
	errmsg_ = uiStrings::phrCannotRead(
		tr("the randome line's geometry from the database") );
	return false;
    }

    for ( int idx=0; idx<rdlgeom->nrNodes(); idx++ )
	linegeom_ += SI().transform( rdlgeom->nodePosition( idx ) );

    if ( linegeom_.size() < 2 )
    {
	errmsg_ = tr( "Less than 2 points in random line" );
	return false;
    }

    return true;
}


void ProfileModelFromEventData::Section::fillPar( IOPar& par ) const
{
    par.setYN( sKey::TwoD(), is2d_ );
    if ( is2d_ )
	par.set( sKey::GeomID(), geomid_ );
    else
	par.set( sKeyRandomLineID(), rdmlinemid_ );
    par.set( sKeySeisID(), seismid_ );
}


void ProfileModelFromEventData::Section::usePar( const IOPar& par )
{
    par.getYN( sKey::TwoD(), is2d_ );
    if ( is2d_ )
	par.get( sKey::GeomID(), geomid_ );
    else
	par.get( sKeyRandomLineID(), rdmlinemid_ );
    par.get( sKeySeisID(), seismid_ );
}


ProfileModelFromEventData::Event::Event( ZValueProvider* zprov )
    : zvalprov_(zprov)
    , newintersectmarker_(0)
{
    setMarker( zprov->getName().getFullString() );
}


ProfileModelFromEventData::Event::~Event()
{
    delete zvalprov_;
    if ( newintersectmarker_ )
    {
	const Strat::Level* eventlvl = Strat::LVLS().get( levelid_ );
	if ( eventlvl )
	    Strat::eRT().removeLevelUnit( *eventlvl );
	Strat::eLVLS().remove( levelid_ );
    }

    delete newintersectmarker_;
}


void ProfileModelFromEventData::Event::fillPar( IOPar& par ) const
{
    zvalprov_->fillPar( par );
    par.set( sKeyMarkerName(), getMarkerName() );
}


ProfileModelFromEventData::Event* ProfileModelFromEventData::Event::
	createNewEvent( const IOPar& par, const TrcKeySampling& tks,
			TaskRunner* taskrunner )
{
    BufferString keystr;
    if ( !par.get(ZValueProvider::sType(),keystr) )
	return 0;

    ZValueProvider* zvalprov =
	ZValueProvider::factory().create( keystr, par, tks, taskrunner );
    if ( !zvalprov )
	return 0;

    Event* newevent = new Event( zvalprov );
    BufferString tiemarkernm;
    par.get( sKeyMarkerName(), tiemarkernm );
    newevent->setMarker( tiemarkernm );
    return newevent;
}


void ProfileModelFromEventData::Event::setMarker( const char* markernm )
{
    Strat::LevelSet& lvls = Strat::eLVLS();
    Strat::RefTree& strattree = Strat::eRT();
    if ( newintersectmarker_ )
    {
	lvls.remove( levelid_ );
	const Strat::Level* eventlvl = Strat::LVLS().get( levelid_ );
	if ( eventlvl )
	    strattree.removeLevelUnit( *eventlvl );
    }

    delete newintersectmarker_;
    newintersectmarker_ = 0;
    FixedString markernmstr( markernm );
    const bool invalidmarkernm =
	markernmstr.isEmpty() ||
	markernmstr==ProfileModelFromEventData::addMarkerStr();
    if ( !lvls.isPresent(markernm) || invalidmarkernm )
    {
	newintersectmarker_ = new Well::Marker(
		invalidmarkernm ? zvalprov_->getName().getFullString()
				: markernmstr );
	newintersectmarker_->setColor( zvalprov_->drawColor() );
	Strat::Level* newlevel =
	    lvls.add( newintersectmarker_->name(),newintersectmarker_->color());
	tiemarkernm_ = markernm;
	levelid_ = newlevel->id();
	newintersectmarker_->setLevelID( levelid_ );
	const Strat::Level* eventlvl = Strat::LVLS().get( levelid_ );
	strattree.addLevelUnit( *eventlvl );
    }
    else
    {
	const int lvlidx = lvls.indexOf( markernm );
	tiemarkernm_ = markernmstr;
	levelid_ = lvls.levelID( lvlidx );
    }
}


Color ProfileModelFromEventData::Event::getMarkerColor() const
{
    if ( newintersectmarker_ )
	return newintersectmarker_->color();
    const Strat::Level* stratlvl = Strat::LVLS().get( levelid_ );
    return stratlvl ? stratlvl->color() : Color::NoColor();
}


BufferString ProfileModelFromEventData::Event::getMarkerName() const
{
    return newintersectmarker_ ? newintersectmarker_->name() : tiemarkernm_;
}

static int sDefNrCtrlProfiles = 50;

ProfileModelFromEventData::ProfileModelFromEventData(
	ProfileModelBase* model, const TypeSet<Coord>& linegeom )
    : model_(model)
    , section_(linegeom)
    , totalnrprofs_(sDefNrCtrlProfiles)
    , voiidx_(-1)
{
}


ProfileModelFromEventData::ProfileModelFromEventData(
	ProfileModelBase* model )
    : model_(model)
    , totalnrprofs_(sDefNrCtrlProfiles)
    , voiidx_(-1)
{
}


ProfileModelFromEventData::~ProfileModelFromEventData()
{
    removeAllEvents();
}


bool ProfileModelFromEventData::hasPar( const IOPar& par )
{
    PtrMan<IOPar> proffromevdatapar = par.subselect( sKeyStr() );
    return proffromevdatapar;
}


void ProfileModelFromEventData::fillPar( IOPar& par ) const
{
    IOPar proffromevpar;
    IOPar sectionpar;
    proffromevpar.set( sKeyNrProfs(), totalnrprofs_ );
    proffromevpar.set( sKeyEventType(), eventtypestr_ );
    section_.fillPar( sectionpar );
    proffromevpar.mergeComp( sectionpar, sKeySection() );
    for ( int iev=0; iev<events_.size(); iev++ )
    {
	IOPar eventpar;
	events_[iev]->fillPar( eventpar );
	proffromevpar.mergeComp( eventpar, IOPar::compKey(sKeyEvent(),iev) );
    }

    if ( ztransform_ )
	ztransform_->fillPar( proffromevpar );
    par.mergeComp( proffromevpar, sKeyStr() );
}


bool ProfileModelFromEventData::prepareSectionGeom()
{
    if ( !section_.fetchLineGeom() )
	return false;

    prepareTransform();
    return true;
}


ProfileModelFromEventData* ProfileModelFromEventData::createFrom(
	ProfileModelBase& model, const IOPar& par, TaskRunner* taskrunner )
{
    PtrMan<IOPar> proffromevpar = par.subselect( sKeyStr() );
    if ( !proffromevpar )
	return 0;

    BufferString eventtypestr;
    if ( !proffromevpar->get(sKeyEventType(),eventtypestr) )
	return 0;

    ProfileModelFromEventData* profmodelfromdata =
	new ProfileModelFromEventData( &model );
    profmodelfromdata->eventtypestr_ = eventtypestr;
    proffromevpar->get( sKeyNrProfs(), profmodelfromdata->totalnrprofs_ );
    PtrMan<IOPar> sectionpar = proffromevpar->subselect( sKeySection() );
    profmodelfromdata->section_.usePar( *sectionpar );
    TrcKeySampling sectiontks;
    profmodelfromdata->prepareSectionGeom();
    profmodelfromdata->section_.getSectionTKS( sectiontks );
    int iev = 0;
    while( true )
    {
	PtrMan<IOPar> eventpar =
	    proffromevpar->subselect( IOPar::compKey(sKeyEvent(),iev) );
	if ( !eventpar )
	    break;

	iev++;
	profmodelfromdata->events_ +=
	    Event::createNewEvent( *eventpar, sectiontks, taskrunner );
    }

    ZAxisTransform* zat = ZAxisTransform::create( *proffromevpar );
    if ( zat )
	profmodelfromdata->setTransform( zat );
    profmodelfromdata->sortEventsonDepthIDs();
    profmodelfromdata->prepareIntersectionMarkers();
    return profmodelfromdata;
}


float ProfileModelFromEventData::getZValue( int evidx, const Coord& crd ) const
{
    if ( !events_.validIdx(evidx) )
	return mUdf(float);

    return events_[evidx]->zvalprov_->getZValue( crd );
}


BufferString ProfileModelFromEventData::getMarkerName( int evidx ) const
{
    if ( !events_.validIdx(evidx) )
	return BufferString::empty();

    return events_[evidx]->getMarkerName();
}


bool ProfileModelFromEventData::isIntersectMarker( const char* markernm ) const
{
    for ( int iev=0; iev<events_.size(); iev++ )
    {
	if ( events_[iev]->newintersectmarker_ &&
	     events_[iev]->newintersectmarker_->name()==markernm )
	    return true;
    }

    return false;
}


bool ProfileModelFromEventData::isIntersectMarker( int evidx ) const
{
    if ( !events_.validIdx(evidx) )
	return false;

    return events_[evidx]->newintersectmarker_;
}


int ProfileModelFromEventData::tiedToEventIdx( const char* markernm ) const
{
    for ( int iev=0; iev<events_.size(); iev++ )
    {
	if ( events_[iev]->getMarkerName()==markernm )
	    return iev;
    }

    return -1;
}


void ProfileModelFromEventData::setTieMarker( int evidx, const char* markernm )
{
    if ( !events_.validIdx(evidx) )
	return;

    Event& ev = *events_[evidx];
    if ( ev.newintersectmarker_ )
	model_->removeMarker( ev.newintersectmarker_->name() );

    ev.setMarker( markernm );
}


#define maxAllowedDDah 50

void ProfileModelFromEventData::setNearestTieEvent(
	int ev1idx, int ev2idx, const BufferString& tiemnm )
{
    const int tiedtoevidx = tiedToEventIdx( tiemnm );
    const float ev1avgdzval = getAvgDZval( ev1idx, tiemnm );
    const float ev2avgdzval = getAvgDZval( ev2idx, tiemnm );
    if ( ev1avgdzval<ev2avgdzval )
    {
	if ( tiedtoevidx==ev1idx )
	    setTieMarker( ev2idx, addMarkerStr() );
	else
	{
	    setTieMarker( ev2idx, addMarkerStr() );
	    setTieMarker( ev1idx, tiemnm );
	}
    }
    else if ( ev2avgdzval<ev1avgdzval )
    {
	if ( tiedtoevidx==ev2idx )
	    setTieMarker( ev1idx, addMarkerStr() );
	else
	{
	    setTieMarker( ev1idx, addMarkerStr() );
	    setTieMarker( ev2idx, tiemnm );
	}
    }
}


float ProfileModelFromEventData::getAvgDZval(
	int evidx, const BufferString& markernm ) const
{
    Stats::CalcSetup su;
    su.require( Stats::Average );
    Stats::RunCalc<float> statrc( su );
    for ( int iprof=0; iprof<model_->size(); iprof++ )
    {
	const ProfileBase* prof = model_->get( iprof );
	if ( !prof->isPrimary() )
	    continue;

	const float evdah = getEventDepthVal( evidx, *prof );
	const int markeridx = prof->markers_.indexOf( markernm );
	if ( markeridx<0 )
	    continue;

	statrc.addValue( prof->markers_[markeridx]->dah()-evdah );
    }

    return fabs( mCast(float,statrc.average()) );
}


void ProfileModelFromEventData::findAndSetTieMarkers()
{
    for ( int iev=0; iev<nrEvents(); iev++ )
    {
	BufferString tiemarkernm;
	if ( findTieMarker(iev,tiemarkernm) )
	{
	    const int tiedtoevidx = tiedToEventIdx( tiemarkernm );
	    if ( tiedtoevidx<0 )
		setTieMarker( iev, tiemarkernm );
	    else
		setNearestTieEvent( tiedtoevidx, iev, tiemarkernm );
	}
    }
}


bool ProfileModelFromEventData::findTieMarker( int evidx,
					       BufferString& markernm ) const
{
    if ( !isIntersectMarker(evidx) )
	return true;

    BufferStringSet nearestmarkernms;
    for ( int iprof=0; iprof<model_->size(); iprof++ )
    {
	const ProfileBase* prof = model_->get( iprof );
	if ( !prof->isPrimary() )
	    continue;

	const float evdah = getEventDepthVal( evidx, *prof );
	if ( mIsUdf(evdah) )
	    continue;

	const int topmarkeridx = prof->markers_.getIdxAbove( evdah );
	const int botmarkeridx = prof->markers_.getIdxBelow( evdah );
	if ( !prof->markers_.validIdx(topmarkeridx) &&
	     !prof->markers_.validIdx(botmarkeridx) )
	{
	    pErrMsg( "No top & bottom marker" );
	    continue;
	}

	if ( botmarkeridx-topmarkeridx==2 )
	{
	    nearestmarkernms.addIfNew( prof->markers_[botmarkeridx]->name()+1);
	    continue;
	}

	if ( !prof->markers_.validIdx(topmarkeridx) )
	{
	    nearestmarkernms.addIfNew( prof->markers_[botmarkeridx]->name() );
	    continue;
	}
	else if ( !prof->markers_.validIdx(botmarkeridx) )
	{
	    nearestmarkernms.addIfNew( prof->markers_[topmarkeridx]->name() );
	    continue;
	}

	const float topmarkerddah =
	    evdah - prof->markers_[topmarkeridx]->dah();
	const float botmarkerddah =
	    prof->markers_[botmarkeridx]->dah() - evdah;
	nearestmarkernms.addIfNew(
	     topmarkerddah<botmarkerddah ? prof->markers_[topmarkeridx]->name()
					 :prof->markers_[botmarkeridx]->name());
    }

    if ( nearestmarkernms.isEmpty() )
	return false;

    if ( nearestmarkernms.size()==1 )
    {
	markernm = nearestmarkernms.get( 0 );
	return true;
    }

    TypeSet<float> nearestavgddahs;
    nearestavgddahs.setSize( nearestmarkernms.size(), 0.f );
    for ( int idx=0; idx<nearestmarkernms.size(); idx++ )
    {
	Stats::CalcSetup su;
	su.require( Stats::Average );
	Stats::RunCalc<float> statrc( su );
	for ( int iprof=0; iprof<model_->size(); iprof++ )
	{
	    const ProfileBase* prof = model_->get( iprof );
	    if ( !prof->isPrimary() )
		continue;

	    const float evdah = getEventDepthVal( evidx, *prof );
	    if ( mIsUdf(evdah) )
		continue;

	    const int nearestmarkeridx =
		prof->markers_.indexOf( nearestmarkernms.get(idx) );
	    if ( nearestmarkeridx<0 )
		continue;

	    statrc.addValue( prof->markers_[nearestmarkeridx]->dah()-evdah );
	}

	nearestavgddahs[idx] = fabs( mCast(float,statrc.average()) );
    }

    int nearestmidx = -1;
    float nearestddah = mUdf(float);
    for ( int idx=0; idx<nearestavgddahs.size(); idx++ )
    {
	if ( nearestavgddahs[idx]<nearestddah )
	{
	    nearestddah = nearestavgddahs[idx];
	    nearestmidx = idx;
	}
    }

    if ( nearestddah>maxAllowedDDah )
	return false;

    markernm = nearestmarkernms.get( nearestmidx );
    return true;
}


Well::Marker* ProfileModelFromEventData::getIntersectMarker( int evidx ) const
{
    if ( !events_.validIdx(evidx) )
	return 0;

    return events_[evidx]->newintersectmarker_;
}


void ProfileModelFromEventData::addEvent( ZValueProvider* zprov )
{
    events_ += new ProfileModelFromEventData::Event( zprov );
    BufferString tiemarkernm;
    const int newevidx = events_.size()-1;
    if ( findTieMarker(newevidx,tiemarkernm) )
    {
	const int tiedtoevidx = tiedToEventIdx( tiemarkernm );
	if ( tiedtoevidx<0 )
	    setTieMarker( newevidx, tiemarkernm );
	else
	    setNearestTieEvent( tiedtoevidx, newevidx, tiemarkernm );
    }
}


bool ProfileModelFromEventData::hasIntersectMarker() const
{
    for ( int iev=0; iev<nrEvents(); iev++ )
    {
	if ( isIntersectMarker(iev) )
	    return true;
    }

    return false;
}


void ProfileModelFromEventData::prepareIntersectionMarkers()
{
    model_->removeCtrlProfiles();
    if ( !hasIntersectMarker() )
	return;

    setIntersectMarkers();
    removeCrossingEvents();
}


bool ProfileModelFromEventData::getTopBottomMarker(
	const Event& event, const ProfileBase& prof, BufferString& markernm,
	bool istop ) const
{
    const BufferString evmarkernm = event.getMarkerName();
    const int evmarkeridx = prof.markers_.indexOf( evmarkernm );
    if ( evmarkeridx<0 )
	return false;

    const int incr = istop ? -1 : +1;
    const int topbotmarkeridx =
	prof.markers_.validIdx(evmarkeridx+incr) ? evmarkeridx+incr : -1;
    if ( !prof.markers_.validIdx(topbotmarkeridx) )
	return false;

    markernm = prof.markers_[topbotmarkeridx]->name();
    return true;
}


bool ProfileModelFromEventData::isEventCrossing( const Event& event ) const
{
    for ( int iprof=0; iprof<model_->size()-1; iprof++ )
    {
	const ProfileBase* curprof = model_->get( iprof );
	const ProfileBase* nextprof = model_->get( iprof+1 );
	BufferString curtopmarker, curbotmarker;
	getTopBottomMarker( event, *curprof, curtopmarker, true );
	getTopBottomMarker( event, *curprof, curbotmarker, false );
	const Well::Marker* evmarkerinnext =
	    nextprof->markers_.getByName( event.getMarkerName() );
	if ( !evmarkerinnext )
	    continue;

	const Well::Marker* curproftopmarkerinnext =
	    nextprof->markers_.getByName( curtopmarker );
	int curprotopmarkeridx = curprof->markers_.indexOf( curtopmarker );
	while ( !curproftopmarkerinnext )
	{
	    curprotopmarkeridx--;
	    if ( curprotopmarkeridx<0 )
		break;

	    curtopmarker = curprof->markers_[curprotopmarkeridx]->name();
	    curproftopmarkerinnext = nextprof->markers_.getByName(curtopmarker);
	}

	if ( curproftopmarkerinnext &&
	     curproftopmarkerinnext->dah()>evmarkerinnext->dah() )
	    return true;

	const Well::Marker* curprofbotmarkerinnext =
	    nextprof->markers_.getByName( curbotmarker );
	int curprobotmarkeridx = curprof->markers_.indexOf( curbotmarker );
	while ( !curprofbotmarkerinnext )
	{
	    curprobotmarkeridx++;
	    if ( curprobotmarkeridx>=curprof->markers_.size() )
		break;

	    curbotmarker = curprof->markers_[curprobotmarkeridx]->name();
	    curprofbotmarkerinnext = nextprof->markers_.getByName(curbotmarker);
	}
	if ( curprofbotmarkerinnext &&
	     curprofbotmarkerinnext->dah()<evmarkerinnext->dah() )
	    return true;
    }

    return false;
}


void ProfileModelFromEventData::removeCrossingEvents()
{
    uiStringSet removedevents;
    for ( int iev=events_.size()-1; iev>=0; iev-- )
    {
	if ( isEventCrossing(*events_[iev]) )
	{
	    removedevents += events_[iev]->zvalprov_->getName();
	    removeEvent( iev );
	}
    }

    if ( !removedevents.isEmpty() )
	warnmsg_ = tr( "Following events are removed as they were crossing "
			"other events : %1" )
			.arg(removedevents.createOptionString());
}


void ProfileModelFromEventData::sortEventsonDepthIDs()
{
    if ( nrEvents() )
    {
	ObjectSet<ZValueProvider> zvalprovs;
	for ( int iev=0; iev<events_.size(); iev++ )
	    zvalprovs += events_[iev]->zvalprov_;
	DepthIDSetter* depthidsetter =
	    events_[0]->zvalprov_->getDepthIDSetter( zvalprovs);
	if ( depthidsetter )
	    depthidsetter->go();
    }

    TypeSet<int> evdepthids, sortedevidxs;
    evdepthids.setSize( events_.size(), mUdf(int) );
    sortedevidxs.setSize( events_.size(), mUdf(int) );
    for ( int iev=0; iev<events_.size(); iev++ )
    {
	evdepthids[iev] = events_[iev]->zvalprov_->depthID();
	sortedevidxs[iev] = iev;
    }

    sort_coupled( evdepthids.arr(), sortedevidxs.arr(), sortedevidxs.size() );
    ObjectSet<Event> sortedevents;
    for ( int idx=0; idx<evdepthids.size(); idx++ )
	sortedevents.add( events_[ sortedevidxs[idx] ] );
    events_ = sortedevents;
}


void ProfileModelFromEventData::removeAllEvents()
{
    while( events_.size() )
	removeEvent( events_.size()-1 );
}


void ProfileModelFromEventData::removeMarkers( const char* markernm )
{
    model_->removeMarker( markernm );
    for ( int idx=0; idx<model_->size(); idx++ )
    {
	const ProfileBase* prof = model_->get( idx );
	if ( !prof->isWell() )
	    continue;

	Well::Data* wd = Well::MGR().get( prof->wellid_ );
	const int markeridx = wd->markers().indexOf( markernm );
	if ( markeridx>=0 )
	    wd->markers().removeSingle( markeridx );
    }
}


void ProfileModelFromEventData::removeEvent( int evidx )
{
    if ( !events_.validIdx(evidx) )
	return;

    if ( isIntersectMarker(evidx) )
	removeMarkers( events_[evidx]->newintersectmarker_->name() );

    delete events_.removeSingle( evidx );
}


void ProfileModelFromEventData::setTransform( ZAxisTransform* tr )
{
    ztransform_ = tr;
    if ( model_ )
	model_->setZTransform( tr );
    prepareTransform();
}


bool ProfileModelFromEventData::prepareTransform()
{
    if ( !ztransform_ || section_.linegeom_.isEmpty() )
	return false;

    TrcKeyZSampling linegomtkzs( false );
    for ( int ipos=0; ipos<section_.linegeom_.size(); ipos++ )
    {
	TrcKey postk;
	postk.setFrom( section_.linegeom_[ipos] );
	linegomtkzs.hsamp_.include( postk );
    }

    linegomtkzs.zsamp_ = SI().zRange( true );
    if ( voiidx_>=0 )
	ztransform_->removeVolumeOfInterest( voiidx_ );
    voiidx_ = ztransform_->addVolumeOfInterest( linegomtkzs );
    return ztransform_->loadDataIfMissing( voiidx_, 0 );
}


float ProfileModelFromEventData::getDepthVal( float pos, float zval,
					      bool depthintvdss ) const
{
    return model_->getDepthVal( zval, pos, section_.profposprov_,depthintvdss );
}


float ProfileModelFromEventData::getEventDepthVal(
	int evidx, const ProfileBase& prof, bool depthintvdss ) const
{
    float evdepthval = getZValue( evidx, prof.coord_ );
    if ( mIsUdf(evdepthval) )
	return getInterpolatedDepthAtPosFromEV( prof.pos_,*events_[evidx],
						depthintvdss );
    return getDepthVal( prof.pos_, evdepthval, depthintvdss );
}


float ProfileModelFromEventData::calcZOffsetForIntersection(
	int evidx, const ProfileBase& prof ) const
{
    if ( !isIntersectMarker(evidx) )
	return 0.0f;

    const float evdepthval = getEventDepthVal( evidx, prof );
    if ( mIsUdf(evdepthval) )
	return 0.0f;

    int topevidx = evidx;
    while ( topevidx-- )
    {
	if ( topevidx<0 || !isIntersectMarker(topevidx) )
	    break;
    }

    int botevidx = evidx;
    while ( botevidx++ )
    {
	if ( botevidx>=events_.size()-1 || !isIntersectMarker(botevidx) )
	    break;
    }

    const float topzoffset =
	events_.validIdx(topevidx) ? getZOffset( topevidx, prof ) : 0.f;
    const float topevdepthval = getEventDepthVal( topevidx, prof );
    const float botzoffset =
	events_.validIdx(botevidx) ? getZOffset( botevidx, prof ) : 0.f;
    const float botevdepthval = getEventDepthVal( botevidx, prof );
    if ( mIsUdf(topevdepthval) )
	return botzoffset;
    else if ( mIsUdf(botevdepthval) )
	return topzoffset;

    const float diffdepthval = botevdepthval - topevdepthval;
    const float toprelposfac = 1 - (evdepthval-topevdepthval)/diffdepthval;
    const float botrelposfac = 1 - (botevdepthval-evdepthval)/diffdepthval;
    return toprelposfac*topzoffset + botrelposfac*botzoffset;
}


float ProfileModelFromEventData::getZOffset( int evidx,
					     const ProfileBase& prof ) const
{
    const float evdepthval = getEventDepthVal( evidx, prof );
    if ( mIsUdf(evdepthval) )
	return mUdf(float);

    const Well::Marker* mrkr =
	prof.markers_.getByName( getMarkerName(evidx) );
    return mrkr && !mIsUdf(mrkr->dah()) ? mrkr->dah() - evdepthval
					: mUdf(float);
}


void ProfileModelFromEventData::setIntersectMarkers()
{
    for ( int evidx=0; evidx<nrEvents(); evidx++ )
	setIntersectMarkersForEV( evidx );
}


void setMarker( const Well::Marker& marker, float depth, ProfileBase* prof,
		bool setinwell )
{
    Well::MarkerSet& markers =
	setinwell ? Well::MGR().get(prof->wellid_)->markers() : prof->markers_;
    const int markeridx = markers.indexOf( marker.name() );
    if ( markeridx>=0 )
	markers[markeridx]->setDah( depth );
    else
    {
	Well::Marker* newtiemarker = new Well::Marker( marker.name() );
	newtiemarker->setColor( marker.color() );
	newtiemarker->setDah( depth );
	markers.insertNew( newtiemarker );
    }
}


void ProfileModelFromEventData::setIntersectMarkersForEV( int evidx )
{
    Event& event = *events_[evidx];
    if ( !event.newintersectmarker_ )
	return;

    for ( int iprof=0; iprof<model_->size(); iprof++ )
    {
	ProfileBase* prof = model_->get( iprof );
	if ( !prof->isPrimary() )
	    continue;

	float tvdss = getEventDepthVal( evidx, *prof, true );
	if ( mIsUdf(tvdss) )
	    continue;
	setMarker( *event.newintersectmarker_, tvdss, prof, false);
	if ( prof->isWell() )
	{
	    float dah = getEventDepthVal( evidx, *prof, false );
	    setMarker( *event.newintersectmarker_, dah, prof, true );
	}
    }
}


float ProfileModelFromEventData::getInterpolatedDepthAtPosFromEV(
	float pos, const Event& event, bool depthintvdss ) const
{
    if ( !section_.profposprov_ )
    {
	pErrMsg( "Huh! No geometry found" );
	return mUdf(float);
    }

    const float dpos = 1.f/1000.f;
    const ZValueProvider* zvalprov = event.zvalprov_;

    float prevppdmdepth = mUdf(float);
    float prevpos = pos;
    while ( prevpos>=0 )
    {
	prevpos -= dpos;
	const Coord prevcoord = section_.profposprov_->getCoord( prevpos );
	prevppdmdepth = zvalprov->getZValue( prevcoord );
	prevppdmdepth = getDepthVal( prevpos, prevppdmdepth, depthintvdss );
	if ( !mIsUdf(prevppdmdepth) )
	    break;
    }

    float nextppdmdepth = mUdf(float);
    float nextpos = pos;
    while ( nextpos<=1.0f )
    {
	nextpos += dpos;
	const Coord nextcoord = section_.profposprov_->getCoord( nextpos );
	nextppdmdepth = zvalprov->getZValue( nextcoord );
	nextppdmdepth = getDepthVal( nextpos, nextppdmdepth, depthintvdss );
	if ( !mIsUdf(nextppdmdepth) )
	    break;
    }

    if ( mIsUdf(prevppdmdepth) || mIsUdf(nextppdmdepth) )
	return mUdf(float);

    const float relpos = (pos - prevpos) / (nextpos - prevpos);
    return (prevppdmdepth*(1-relpos)) + (nextppdmdepth*relpos);
}
