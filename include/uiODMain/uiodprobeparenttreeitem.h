#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2006
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uiodsceneparenttreeitem.h"

class Probe;

mExpClass(uiODMain) uiODProbeParentTreeItem
			: public uiODSceneParentTreeItem
{   mODTextTranslationClass(uiODProbeParentTreeItem);
public:

    enum Type		{ Empty, Select, Default, RGBA };

			uiODProbeParentTreeItem(const uiString&);
    const char*		childObjTypeKey() const;

    bool		showSubMenu();
    virtual bool	canShowSubMenu() const		{ return true; }
    virtual bool	canAddFromWell() const		{ return true ; }
    virtual Probe*	createNewProbe() const		=0;

    static uiString	sAddEmptyPlane();
    static uiString	sAddAndSelectData();
    static uiString	sAddDefaultData();
    static uiString	sAddColorBlended();
    static uiString	sAddAtWellLocation();

protected:

    bool		fillProbe(Probe&,Type);
    bool		setDefaultAttribLayer(Probe&) const;
    bool		setSelAttribProbeLayer(Probe&) const;
    bool		setRGBProbeLayers(Probe&) const;
};
