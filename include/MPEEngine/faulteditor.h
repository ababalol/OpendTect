#ifndef faulteditor_h
#define faulteditor_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          January 2005
 RCS:           $Id: faulteditor.h,v 1.13 2011/02/09 16:50:08 cvsjaap Exp $
________________________________________________________________________

-*/

#include "emeditor.h"

namespace EM { class Fault3D; };
namespace Geometry { class FaultStickSurface; };

template <class T> class Selector;

namespace MPE
{

mClass FaultEditor : public ObjectEditor
{
public:
    				FaultEditor(EM::Fault3D&);

    static ObjectEditor*	create(EM::EMObject&);
    static void			initClass();

    void			setLastClicked(const EM::PosID&);
    void			setSowingPivot(const Coord3);


    void			setZScale(float);
    void			setScaleVector(const Coord3& v);
    				//!< x'=x, y'=v1*x*+v2*y, z'=v3*z

    void			getInteractionInfo(bool& makenewstick,
				    EM::PosID& insertpid,const Coord3& pos,
				    const Coord3* posnormal=0) const;

    bool			removeSelection(const Selector<Coord3>&);

protected:
    float		distToStick(const Geometry::FaultStickSurface&,
				    int curstick,const Coord3& pos,
				    const Coord3* posnormal) const;
    float		panelIntersectDist(const Geometry::FaultStickSurface&,
				    int sticknr,const Coord3& mousepos,
				    const Coord3& posnormal) const;
    int			getSecondKnotNr(const Geometry::FaultStickSurface&,
				    int sticknr,const Coord3& mousepos) const;

    float		getNearestStick(int& stick,EM::SectionID& sid,
			    const Coord3& pos,const Coord3* posnormal) const;
    bool		getInsertStick(int& stick,EM::SectionID& sid,
			    const Coord3& pos,const Coord3* posnormal) const;
    void		getPidsOnStick( EM::PosID& insertpid,int stick,
			    const EM::SectionID&,const Coord3& pos) const;

    Geometry::ElementEditor*	createEditor(const EM::SectionID&);
    Coord3			scalevector_;

    int				getLastClickedStick() const;

    void			cloneMovingNode();

    Coord3			sowingpivot_;
    TypeSet<Coord3>		sowinghistory_;
};


}  // namespace MPE

#endif
