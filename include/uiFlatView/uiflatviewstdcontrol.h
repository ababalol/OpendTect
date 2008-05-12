#ifndef uiflatviewstdcontrol_h
#define uiflatviewstdcontrol_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2007
 RCS:           $Id: uiflatviewstdcontrol.h,v 1.7 2008-05-12 05:32:16 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiflatviewcontrol.h"
#include "menuhandler.h"
class uiMenuHandler;
class uiToolButton;
class uiToolBar;


/*!\brief The standard tools to control uiFlatViewer(s). */

class uiFlatViewStdControl : public uiFlatViewControl
{
public:

    struct Setup
    {
			Setup( uiParent* p=0 )
			    : parent_(p)
			    , helpid_("")
			    , withwva_(true)
			    , withstates_(true)		{}

	mDefSetupMemb(uiParent*,parent) //!< null => viewer's parent
	mDefSetupMemb(bool,withwva)
	mDefSetupMemb(bool,withstates)
	mDefSetupMemb(BufferString,helpid)
    };

    			uiFlatViewStdControl(uiFlatViewer&,const Setup&);
    			~uiFlatViewStdControl();

protected:

    uiToolBar*		tb_;
    uiToolButton*	zoominbut_;
    uiToolButton*	zoomoutbut_;
    uiToolButton*	manipbut_;
    uiToolButton*	drawbut_;
    uiToolButton*	parsbut_;

    virtual void	finalPrepare();
    void		updatePosButtonStates();

    void		vwChgCB(CallBacker*);
    void		vwrAdded(CallBacker*);
    void		wheelMoveCB(CallBacker*);
    void		zoomCB(CallBacker*);
    void		panCB(CallBacker*);
    void		flipCB(CallBacker*);
    void		parsCB(CallBacker*);
    void		stateCB(CallBacker*);
    void		helpCB(CallBacker*);

    bool		handleUserClick();

    uiMenuHandler&      menu_;
    MenuItem           	propertiesmnuitem_;
    void                createMenuCB(CallBacker*);
    void                handleMenuCB(CallBacker*);

    BufferString	helpid_;

};

#endif
