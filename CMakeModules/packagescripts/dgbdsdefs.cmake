#!/bin/csh
#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to define dgbds(DipSteering) package variables
# Author:       Nageswara
# Date:         August 2012
#RCS:           $Id: dgbdsdefs.cmake,v 1.4 2012/09/06 05:50:52 cvsnageswara Exp $

#//TODO Modify script to work on all platforms.
SET( LIBLIST dGBMPEEngine DipSteer uiDipSteer )
SET( EXECLIST "" )
#SET( PACKAGE_DIR "/dsk/d21/nageswara/dev/buildtest/lux64" )
SET( PACK "dgbds" )
