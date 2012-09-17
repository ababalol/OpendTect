#ifndef uihistogramdisplay_h
#define uihistogramdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Dec 2008
 RCS:		$Id: uihistogramdisplay.h,v 1.11 2011/10/26 14:20:13 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uifunctiondisplay.h"
#include "datapack.h"

class uiTextItem;
template <class T> class Array2D;
namespace Stats { template <class T> class ParallelCalc; }

class DataPointSet;

mClass uiHistogramDisplay : public uiFunctionDisplay
{
public:

    				uiHistogramDisplay(uiParent*,Setup&,
						   bool withheader=false);
				~uiHistogramDisplay();

    bool			setDataPackID(DataPack::ID,DataPackMgr::ID);
    void			setData(const float*,int sz);
    void			setData(const Array2D<float>*);
    void			setData(const DataPointSet&);

    void			setHistogram(const TypeSet<float>&,
	    				     Interval<float>,int N=-1);

    const Stats::ParallelCalc<float>&	getStatCalc()	{ return rc_; }
    int                         nrInpVals() const       { return nrinpvals_; }
    int				nrClasses() const       { return nrclasses_; }
    void			putN(); 

protected:

    Stats::ParallelCalc<float>&	rc_;	
    int                         nrinpvals_;
    int                         nrclasses_;
    bool			withheader_;
    uiTextItem*			header_;
    uiTextItem*			nitm_;
    
    void			updateAndDraw();
    void			updateHistogram();
};


#endif
