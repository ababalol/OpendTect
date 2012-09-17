#ifndef uistatsdisplaywin_h
#define uistatsdisplaywin_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id: uistatsdisplaywin.h,v 1.9 2012/05/29 12:56:53 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uimainwin.h"
#include "uistatsdisplay.h"
class BufferStringSet;
class uiComboBox;
class uiStatsDisplay;
namespace Stats { template <class T> class ParallelCalc; }

/*!\brief Stats display main window. See uistatsdisplay.h for details. */

mClass uiStatsDisplayWin : public uiMainWin
{
public:
    				uiStatsDisplayWin(uiParent*,
					const uiStatsDisplay::Setup&,int nr=1,
					bool ismodal=true);
    
    uiStatsDisplay*		statsDisplay(int nr=0)	{ return disps_[nr]; }
    void                        setData(const Stats::ParallelCalc<float>&,
					int nr=0);
    void			addDataNames(const BufferStringSet&);
    void			setDataName(const char*,int nr=0);
    void			setMarkValue(float,bool forx,int nr=0);
    void			showStat(int);

protected:

    ObjectSet<uiStatsDisplay>	disps_;
    uiComboBox*			statnmcb_;

    void			dataChanged(CallBacker*);
public:
    void                        setData( const float* array, int sz,
					int nr=0);
};


#endif
