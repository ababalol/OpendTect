#ifndef faulthorintersect_h
#define faulthorintersect_h
                                                                                
/*+
________________________________________________________________________
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Yuancheng Liu
Date:          March 2010
RCS:           $Id: faulthorintersect.h,v 1.6 2011/04/26 20:09:38 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "position.h"

class Coord3List;

namespace Geometry
{

class BinIDSurface;
class IndexedShape;
class ExplFaultStickSurface;


mClass FaultBinIDSurfaceIntersector
{
public:
				FaultBinIDSurfaceIntersector(float horshift,
					const BinIDSurface&, 
					const ExplFaultStickSurface&,
					Coord3List&);
				~FaultBinIDSurfaceIntersector()	{}

    void			compute();	
				
				//The shape is optional, if not set, we still
				//compute intersections, stored in crdlist_    
    void			setShape(const IndexedShape&);
    const IndexedShape*		getShape(bool takeover=true);

protected:

    float			zshift_;
    Coord3List&			crdlist_;
    const BinIDSurface&		surf_;
    const IndexedShape*		output_;
    const ExplFaultStickSurface& eshape_;
};


};


#endif
