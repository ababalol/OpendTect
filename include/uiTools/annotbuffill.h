#ifndef annotbuffill_h
#define annotbuffill_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Huck
 Date:          04/09/2006
 RCS:           $Id: annotbuffill.h,v 1.7 2009/07/22 16:01:23 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigeom.h"
#include "draw.h"

class uiWorld2Ui;
class uiRGBArray;

/*! \brief: Fills an uiRGBArray ( image buffer ) used to draw annotations 
  on 2D images

*/

typedef Geom::Point2D<double> dPoint;
typedef Geom::Point2D<int> iPoint;

mClass AnnotBufferFiller
{

public:

    mClass LineInfo
    {
	public:

	    LineStyle               		linestyle_;
	    TypeSet<dPoint>      		pts_;
    };
    //TODO add later on sth for the polygones

    
			AnnotBufferFiller(const uiWorld2Ui* w=0);
			~AnnotBufferFiller();

    void		addLineInfo(const LineStyle&,TypeSet<dPoint>,bool,bool);
    void		setW2UI( const uiWorld2Ui* w )	{ w2u_ = w; }
    void		fillBuffer(const uiWorldRect&,uiRGBArray&) const;
    void		fillInterWithBufArea(const uiWorldRect&,const LineInfo*,
	    				     uiRGBArray&) const;

    void		eraseGridLines(bool);
    bool		dispannotlines_;
    bool		disphorgdlines_;
    bool		dispvertgdlines_;
    


protected:

    const uiWorld2Ui*	w2u_;
    ObjectSet<LineInfo> annotlines_;
    ObjectSet<LineInfo> horgdlines_;
    ObjectSet<LineInfo> vertgdlines_;

    void		setPoint(const iPoint&,int,uiRGBArray&) const;
    void		setLine(const iPoint&,const iPoint&,
	    			const LineInfo*,uiRGBArray&) const;
    bool		isLineOutside(const LineInfo*,const uiWorldRect&) const;
    dPoint		computeIntersect(const dPoint&,const dPoint&,
	    				 const uiWorldRect&)const;
    void		dummytest();
};


#endif
