#ifndef visplanedatadisplay_h
#define visplanedatadisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visplanedatadisplay.h,v 1.43 2004-04-27 11:59:36 kristofer Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "vissurvobj.h"
#include "ranges.h"

class AttribSelSpec;
class ColorAttribSel;
class AttribSliceSet;
class CubeSampling;

namespace visBase { class TextureRect; class VisColorTab; };

namespace visSurvey
{

class Scene;

/*!\brief Used for displaying an inline, crossline or timeslice.

    A PlaneDataDisplay object is the front-end object for displaying an inline,
    crossline or timeslice.  Use <code>setType(Type)</code> for setting the 
    requested orientation of the slice.
*/

class PlaneDataDisplay :  public visBase::VisualObject,
			  public visSurvey::SurveyObject
{
public:

    bool			isInlCrl() const { return true; }

    enum Type			{ Inline, Crossline, Timeslice };

    static PlaneDataDisplay*	create()
				mCreateDataObj(PlaneDataDisplay);

    void			setType(Type type);
    Type			getType() const { return type; }

    void			setGeometry(bool manip=false,bool init_=false);
    void			setRanges(const StepInterval<float>&,
					  const StepInterval<float>&,
					  const StepInterval<float>&,
					  bool manip=false);
    				//!< Sets the maximum range in each direction

    void			setOrigo(const Coord3&);
    void			setWidth(const Coord3&);

    void			resetDraggerSizes( float appvel );
    				//!< Should be called when appvel has changed
    
    void			showDraggers(bool yn);
    void			resetManip();

    CubeSampling&		getCubeSampling(bool manippos=true);
    const CubeSampling&		getCubeSampling(bool manippos=true) const
				{ return const_cast<PlaneDataDisplay*>(this)->
				    	getCubeSampling(manippos); }
    void			setCubeSampling(const CubeSampling&);
    AttribSelSpec&		getAttribSelSpec();
    const AttribSelSpec&	getAttribSelSpec() const;
    void			setAttribSelSpec(const AttribSelSpec&);
    bool			putNewData(AttribSliceSet*,bool);
    				/*!< Becomes mine */
    const AttribSliceSet*	getCachedData(bool colordata=false) const;
    void			showTexture(int);

    void			turnOn(bool);
    bool			isOn() const;

    void			setColorSelSpec(const ColorAttribSel&);
    const ColorAttribSel*	getColorSelSpec() const;

    void			setColorTab(visBase::VisColorTab&);
    visBase::VisColorTab&	getColorTab();
    const visBase::VisColorTab&	getColorTab() const;

    float			getValue( const Coord3& ) const;

    void			setMaterial( visBase::Material* );
    const visBase::Material*	getMaterial() const;
    visBase::Material*		getMaterial();

    const visBase::TextureRect&	textureRect() const { return *trect; }
    visBase::TextureRect&	textureRect() { return *trect; }

    SoNode*			getInventorNode();

    virtual void		fillPar( IOPar&, TypeSet<int>& ) const;
    virtual int			usePar( const IOPar& );

    virtual float		calcDist( const Coord3& ) const;
    virtual NotifierAccess*     getMovementNotification() { return &moving; }

    const char*			getResName(int) const;
    void			setResolution(int);
    int				getResolution() const;
    int				getNrResolutions() const;

    const TypeSet<float>*	getHistogram() const;

protected:
				~PlaneDataDisplay();

    void			setTextureRect(visBase::TextureRect*);
    void			setData(const AttribSliceSet*,int datatype=0);
    void			appVelChCB(CallBacker*);
    void			manipChanged(CallBacker*);

    visBase::TextureRect*	trect;

    Type			type;
    AttribSelSpec&		as;
    ColorAttribSel&		colas;

    AttribSliceSet*             cache;
    AttribSliceSet*             colcache;
    CubeSampling&		cs;
    CubeSampling&		manipcs;

    static const char*		trectstr;
    Notifier<PlaneDataDisplay>	moving;
};

};


#endif
