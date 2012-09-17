#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODCoinUtils.cmake,v 1.12 2012/04/10 07:17:39 cvskris Exp $
#_______________________________________________________________________________

MACRO(OD_SETUP_COIN)
    IF ( (NOT DEFINED COINDIR) OR COINDIR STREQUAL "" )
	SET(OD_COINDIR_ENV $ENV{OD_COINDIR})

	IF(OD_COINDIR_ENV)
	    SET(COINDIR ${OD_COINDIR_ENV} CACHE PATH "COIN Location" FORCE )
	    MESSAGE( STATUS "Detecting COIN location: ${COINDIR}" )
	ENDIF()
    ENDIF()

    IF ( (NOT DEFINED COINDIR) OR COINDIR STREQUAL "" )
	SET(COINDIR "" CACHE PATH "COIN location" FORCE )
	MESSAGE( FATAL_ERROR "COINDIR not set")
    ENDIF()

    FIND_PACKAGE( OpenGL )

    IF(WIN32)
	FIND_LIBRARY(COINLIB NAMES Coin3 PATHS ${COINDIR}/lib REQUIRED )
	FIND_LIBRARY(SOQTLIB NAMES SoQt1 PATHS ${COINDIR}/lib REQUIRED )
    ELSE()
	FIND_LIBRARY(COINLIB NAMES Coin PATHS ${COINDIR}/lib REQUIRED )
	FIND_LIBRARY(SOQTLIB NAMES SoQt PATHS ${COINDIR}/lib REQUIRED )
    ENDIF()

    IF(WIN32)
	FIND_LIBRARY(OD_SIMVOLEON_LIBRARY NAMES SimVoleon2
		     PATHS ${COINDIR}/lib REQUIRED )
    ELSE()
	SET(TMPVAR ${CMAKE_FIND_LIBRARY_SUFFIXES})
	SET(CMAKE_FIND_LIBRARY_SUFFIXES ${OD_STATIC_EXTENSION})
	FIND_LIBRARY(OD_SIMVOLEON_LIBRARY NAMES SimVoleon
		     PATHS ${COINDIR}/lib REQUIRED )
	SET(CMAKE_FIND_LIBRARY_SUFFIXES ${TMPVAR})
    ENDIF()


    IF(OD_USECOIN)
	IF ( OD_EXTRA_COINFLAGS )
	    ADD_DEFINITIONS( ${OD_EXTRA_COINFLAGS} )
	ENDIF( OD_EXTRA_COINFLAGS )

	LIST(APPEND OD_MODULE_INCLUDEPATH ${COINDIR}/include )
	SET(OD_COIN_LIBS ${COINLIB} ${SOQTLIB} ${OPENGL_gl_LIBRARY} )
    ENDIF()

    LIST(APPEND OD_MODULE_EXTERNAL_LIBS ${OD_COIN_LIBS} )
ENDMACRO(OD_SETUP_COIN)
