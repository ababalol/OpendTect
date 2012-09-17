#ifndef welltiepickset_h
#define welltiepickset_h

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Feb 2009
RCS:           $Id: welltiepickset.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "color.h"
#include "valseriesevent.h"

class SeisTrc;

namespace WellTie
{
    
struct PickData;
struct Marker;

mClass PickSetMgr : public CallBacker
{
public:
    				PickSetMgr( PickData& pd );
    
    Notifier<PickSetMgr> 	pickadded;
    
    enum TrackType      	{ Maxima, Minima, ZeroCrossings };
				DeclareEnumUtils(TrackType)

    bool			lastpicksynth_;
    VSEvent::Type		evtype_;

    void           		addPick(float,bool,const SeisTrc* trc =0);
    void			addPick(float,float);
    bool 	   		isPick() const;
    bool 	   		isSynthSeisSameSize() const;
    void           		clearAllPicks();
    void 	   		clearLastPicks();
    float 	   		findEvent(const SeisTrc&,float) const;
    void			setPickSetPos(bool issynth, int idx, float z);
    void 	   		sortByPos();
    void 	   		setEventType(int);
    int				getEventType() const;
    const TypeSet<Marker>& 	synthPickSet() const 	{ return synthpickset_;}
    const TypeSet<Marker>& 	seisPickSet() const 	{ return seispickset_; }

protected :
    void 	   		sortByPos(TypeSet<Marker>&);

    TypeSet<Marker>& 		synthpickset_;
    TypeSet<Marker>& 		seispickset_;
};

}; //namespace WellTie

#endif

