#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to build a release
# Author:       Nageswara
# Date:		August 2012		
#RCS:           $Id: ODMakePackagesUtils.cmake,v 1.11 2012/09/11 12:25:44 cvsnageswara Exp $

macro ( create_package PACKAGE_NAME )

    FILE( MAKE_DIRECTORY ${DESTINATION_DIR}/bin )

    FILE( MAKE_DIRECTORY ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR} )
    FILE( MAKE_DIRECTORY ${DESTINATION_DIR}/plugins
			  ${DESTINATION_DIR}/plugins/${OD_PLFSUBDIR} )
    IF( APPLE )
	set( MACBINDIR "Contents/MacOS" )
	file( MAKE_DIRECTORY ${DESTINATION_DIR}/Contents )
    ENDIF()

    IF( ${PACKAGE_NAME} STREQUAL "base" )
	IF( APPLE )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${PSD}/data/install_files/macscripts/Contents/Resources/qt_menu.nib
			     ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/qt_menu.nib )
	    execute_process( COMMAND chmod 755 ${CMAKE_INSTALL_PREFIX}/macos_link
			     RESULT_VARIABLE STATUS )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			     ${CMAKE_INSTALL_PREFIX}/macos_link
			     ${DESTINATION_DIR}/Contents )
	    execute_process( COMMAND ${DESTINATION_DIR}/Contents/macos_link 
				     WORKING_DIRECTORY ${DESTINATION_DIR}/Contents
				     RESULT_VARIABLE STATUS )
	    FILE( REMOVE_RECURSE ${DESTINATION_DIR}/Contents/macos_link)
	    IF( NOT ${STATUS} EQUAL "0" )
		MESSAGE( FATAL_ERROR "Failed to create link" )
	    ENDIF()
	ENDIF()

        copy_thirdpartylibs()
        SET( LIBLIST ${LIBLIST};${PLUGINS} )
    ENDIF()

    set( COPYTODIR ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR} )
    MESSAGE( "Copying ${OD_PLFSUBDIR} libraries" )
    FOREACH ( FILE ${LIBLIST} )
	IF( ${OD_PLFSUBDIR} STREQUAL "lux64" OR ${OD_PLFSUBDIR} STREQUAL "lux32" )
		SET(LIB "lib${FILE}.so")
	ENDIF()

	IF( ${OD_PLFSUBDIR} STREQUAL "mac" )
		SET( LIB "lib${FILE}.dylib" )
	ENDIF()

	IF( ${OD_PLFSUBDIR} STREQUAL "win32" OR ${OD_PLFSUBDIR} STREQUAL "win64" )
		SET( LIB "${FILE}.dll" )
	ENDIF()

	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/${LIB}
			 ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR} )

	FILE( GLOB ALOFILES ${PSD}/plugins/${OD_PLFSUBDIR}/*.${FILE}.alo )
	FOREACH( ALOFILE ${ALOFILES} )
	   execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${ALOFILE}
				    ${DESTINATION_DIR}/plugins/${OD_PLFSUBDIR} )
	ENDFOREACH()

    ENDFOREACH()

    IF( ${PACKAGE_NAME} STREQUAL "dgbbase" )
#Inslall lm 
	FOREACH( SPECFILE ${SPECFILES} )
	     execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			      ${CMAKE_INSTALL_PREFIX}/${SPECFILE}
			      ${DESTINATION_DIR} )
	ENDFOREACH()

	execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			 ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/lm
			 ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/lm.dgb )
	IF( UNIX OR APPLE )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			     ${CMAKE_INSTALL_PREFIX}/mk_flexlm_links
			     ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/lm.dgb )
	    execute_process( COMMAND
			${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/lm.dgb/mk_flexlm_links 
			WORKING_DIRECTORY ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/lm.dgb
			RESULT_VARIABLE STATUS )
	    FILE( REMOVE_RECURSE ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/lm.dgb/mk_flexlm_links )
	   
	    IF( NOT ${STATUS} EQUAL "0" )
		MESSAGE( FATAL_ERROR "Failed to create license related links" )
	    ENDIF()
	ENDIF()
    ENDIF()

    IF( WIN32 )
	IF( ${PACKAGE_NAME} STREQUAL "base" )
		SET( EXECLIST "${EXECLIST};${WINEXECLIST}" )
	ENDIF()
    ENDIF()

    MESSAGE( "Copying ${OD_PLFSUBDIR} executables" )
    FOREACH( EXE ${EXECLIST} )
	IF( WIN32 )
		set( EXE "${EXE}.exe" )
	ENDIF()

	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/${EXE} 
			 ${COPYTODIR} )
    ENDFOREACH()

    IF( ${PACKAGE_NAME} STREQUAL "base" )
	FOREACH( SPECFILE ${SPECFILES} )
	     execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			      ${CMAKE_INSTALL_PREFIX}/${SPECFILE}
			      ${DESTINATION_DIR} )
	ENDFOREACH()
	FOREACH( FILES ${ODSCRIPTS} )
	     FILE( GLOB SCRIPTS ${PSD}/bin/${FILES} )
	     FOREACH( SCRIPT ${SCRIPTS} )
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${SCRIPT}
					 ${DESTINATION_DIR}/bin )
	     ENDFOREACH()
	ENDFOREACH()
	IF( WIN32 )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
				     ${CMAKE_INSTALL_PREFIX}/rsm
				     ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/rsm )
	ENDIF()
    ENDIF()

    IF( WIN32 )
	MESSAGE( "Using ${OD_PLFSUBDIR} zip command" )
	execute_process( COMMAND ${PSD}/bin/win/zip -r -q "${PACKAGE_FILENAME}" ${REL_DIR} 
				 WORKING_DIRECTORY ${PACKAGE_DIR}
				 RESULT_VARIABLE STATUS )
    ELSE()
	MESSAGE( "Using ${OD_PLFSUBDIR} zip command" )
	execute_process( COMMAND zip -r -y -q "${PACKAGE_FILENAME}" ${REL_DIR} 
				 WORKING_DIRECTORY ${PACKAGE_DIR}
				 RESULT_VARIABLE STATUS )
    ENDIF()

    IF( NOT ${STATUS} EQUAL "0" )
	MESSAGE( FATAL_ERROR "Could not zip file ${PACKAGE_FILENAME}" )
    ENDIF()
    
    MESSAGE( "DONE" )

endmacro( create_package )


macro( copy_thirdpartylibs )
    SET( COPYTODIR ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR} )

    MESSAGE( "Copying ${OD_PLFSUBDIR} thirdparty libraries" )
    FILE( GLOB LIBS ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Release/* )
    FOREACH( LIB ${LIBS} )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${LIB} ${COPYTODIR} )
    ENDFOREACH()

    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
		     ${CMAKE_INSTALL_PREFIX}/imageformats
		     ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/imageformats )

endmacro( copy_thirdpartylibs )


macro( create_basepackages PACKAGE_NAME )
   IF( EXISTS ${DESTINATION_DIR}/Contents )
	FILE( REMOVE_RECURSE ${DESTINATION_DIR}/Contents )
   ENDIF()
   IF( NOT EXISTS ${DESTINATION_DIR}/doc )
	FILE( MAKE_DIRECTORY ${DESTINATION_DIR}/doc ${DESTINATION_DIR}/doc/User
			     ${DESTINATION_DIR}/doc/ReleaseInfo)
   ENDIF()

   IF( ${PACKAGE_NAME} STREQUAL "basedata" )
       execute_process( COMMAND ${CMAKE_COMMAND} -E copy
				${CMAKE_INSTALL_PREFIX}/doc/about.html
				${DESTINATION_DIR}/doc )
       execute_process( COMMAND ${CMAKE_COMMAND} -E copy
				${CMAKE_INSTALL_PREFIX}/relinfo/RELEASE.txt
				${DESTINATION_DIR}/doc/ReleaseInfo )
       execute_process( COMMAND ${CMAKE_COMMAND} -E copy
				${CMAKE_INSTALL_PREFIX}/relinfo/RELEASEINFO.txt
				${DESTINATION_DIR}/doc/ReleaseInfo )
       FOREACH( LIBS ${LIBLIST} )
	    FILE( GLOB DATAFILES ${CMAKE_INSTALL_PREFIX}/data/${LIBS} )
	    FOREACH( DATA ${DATAFILES} )
    #TODO if possible copy files instead of INSTALL
    #change file permissions if needed on windows
		  FILE( INSTALL DESTINATION ${DESTINATION_DIR}/data
				TYPE DIRECTORY FILES ${DATA}
				REGEX ".svn" EXCLUDE )
	    ENDFOREACH()
       ENDFOREACH()
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${CMAKE_INSTALL_PREFIX}/relinfo/README.txt
			 ${DESTINATION_DIR}/relinfo )
#install WindowLinkTable.txt
       FILE( MAKE_DIRECTORY ${DESTINATION_DIR}/doc/User/base )
       execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			        ${CMAKE_INSTALL_PREFIX}/doc/od_WindowLinkTable.txt
				${DESTINATION_DIR}/doc/User/base/WindowLinkTable.txt )
       execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			        ${CMAKE_INSTALL_PREFIX}/doc/base_.mnuinfo
				${DESTINATION_DIR}/doc/User/base/.mnuinfo )
       execute_process( COMMAND ${CMAKE_COMMAND} -E copy 
			        ${CMAKE_INSTALL_PREFIX}/doc/od_LinkFileTable.txt
				${DESTINATION_DIR}/doc/User/base/LinkFileTable.txt )
   ENDIF()
   IF( ${PACKAGE_NAME} STREQUAL "dgbbasedata" )
       execute_process( COMMAND ${CMAKE_COMMAND} -E copy
				${CMAKE_INSTALL_PREFIX}/relinfo/RELEASE.dgb.txt
				${DESTINATION_DIR}/doc/ReleaseInfo )
       FOREACH( LIB ${LIBLIST} )
	  IF( IS_DIRECTORY "${CMAKE_INSTALL_PREFIX}/data/${LIB}" )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${CMAKE_INSTALL_PREFIX}/data/${LIB}
			     ${DESTINATION_DIR}/data/${LIB} )
	  ELSE()
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			     ${CMAKE_INSTALL_PREFIX}/data/${LIB}
			     ${DESTINATION_DIR}/data/${LIB} )
	  ENDIF()
       ENDFOREACH()
#install WindowLinkTable.txt
       FILE( MAKE_DIRECTORY ${DESTINATION_DIR}/doc/User/dgb )
       execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			        ${CMAKE_INSTALL_PREFIX}/doc/dgb_WindowLinkTable.txt
				${DESTINATION_DIR}/doc/User/dgb/WindowLinkTable.txt )
       execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			        ${CMAKE_INSTALL_PREFIX}/doc/dgb_.mnuinfo
				${DESTINATION_DIR}/doc/User/dgb/.mnuinfo )
       execute_process( COMMAND ${CMAKE_COMMAND} -E copy 
				${CMAKE_INSTALL_PREFIX}/doc/dgb_LinkFileTable.txt
				${DESTINATION_DIR}/doc/User/dgb/LinkFileTable.txt )
   ENDIF()

    IF( WIN32 )
	MESSAGE( "Using ${OD_PLFSUBDIR} zip command" )
	execute_process( COMMAND ${PSD}/bin/win/zip -r -q
					   "${PACKAGE_FILENAME}" ${REL_DIR} 
				 WORKING_DIRECTORY ${PACKAGE_DIR}
				 RESULT_VARIABLE STATUS )
    ELSE()
	MESSAGE( "Using ${OD_PLFSUBDIR} zip command" )
	execute_process( COMMAND zip -r -y -q "${PACKAGE_FILENAME}" ${REL_DIR} 
				 WORKING_DIRECTORY ${PACKAGE_DIR}
				 RESULT_VARIABLE STATUS )
    ENDIF()

   IF( NOT ${STATUS} EQUAL "0" )
	MESSAGE( FATAL_ERROR "Could not zip file ${PACKAGE_FILENAME}" )
   ENDIF()
endmacro( create_basepackages )


macro( init_destinationdir  PACKAGE_NAME )
#    STRING ( TOUPPER ${PACKAGE_NAME} PACKAGE_NAME_UPPER )
#    SET ( FILELIST ${${PACKAGE_NAME_UPPER}_FILELIST} )

    SET ( PACKAGE_FILENAME ${PACKAGE_NAME} )
    SET( PACKAGE_FILENAME "${PACKAGE_FILENAME}_${OD_PLFSUBDIR}.zip" )
    IF( ${PACKAGE_NAME} STREQUAL "basedata" )
        SET( PACKAGE_FILENAME "basedata.zip" )
	IF( APPLE )
	    SET( PACKAGE_FILENAME "basedata_mac.zip" )
	ENDIF()
    ELSEIF( ${PACKAGE_NAME} STREQUAL "dgbbasedata" )
        SET( PACKAGE_FILENAME "dgbbasedata.zip" )
	IF( APPLE )
	    SET( PACKAGE_FILENAME "dgbbasedata_mac.zip" )
	ENDIF()
    ELSEIF( ${PACKAGE_NAME} STREQUAL "doc" )
        SET( PACKAGE_FILENAME "doc.zip" )
	IF( APPLE )
	    SET( PACKAGE_FILENAME "doc_mac.zip" )
	ENDIF()
    ELSEIF( ${PACKAGE_NAME} STREQUAL "dgbdoc" )
        SET( PACKAGE_FILENAME "dgbdoc.zip" )
	IF( APPLE )
	    SET( PACKAGE_FILENAME "dgbdoc_mac.zip" )
	ENDIF()
    ELSEIF( ${PACKAGE_NAME} STREQUAL "classdoc" )
        SET( PACKAGE_FILENAME "classdoc.zip" )
	IF( APPLE )
	    SET( PACKAGE_FILENAME "classdoc_mac.zip" )
	ENDIF()
    ENDIF()

    IF( NOT EXISTS ${PSD}/packages )
        FILE( MAKE_DIRECTORY ${PSD}/packages )
    ENDIF()

    SET( PACKAGE_DIR ${PSD}/packages )
    IF( EXISTS ${PACKAGE_DIR}/${PACKAGE_FILENAME} )
	FILE( REMOVE_RECURSE ${PACKAGE_DIR}/${PACKAGE_FILENAME} )
    ENDIF()
    SET( REL_DIR "${OpendTect_VERSION_MAJOR}.${OpendTect_VERSION_MINOR}.${OpendTect_VERSION_DETAIL}" )
    SET( FULLVER_NAME "${REL_DIR}${OpendTect_VERSION_PATCH}" )
    IF( APPLE )
	SET( REL_DIR "OpendTect${REL_DIR}.app" )
        MESSAGE( "APPLE: reldiris ... ${REL_DIR}" )
    ENDIF()

    SET( DESTINATION_DIR "${PACKAGE_DIR}/${REL_DIR}" )
    IF( EXISTS ${DESTINATION_DIR} )
	FILE ( REMOVE_RECURSE ${DESTINATION_DIR} )
    ENDIF()

    FILE( MAKE_DIRECTORY ${DESTINATION_DIR} ${DESTINATION_DIR}/relinfo )
    IF( APPLE )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			 ${CMAKE_INSTALL_PREFIX}/Contents
			 ${DESTINATION_DIR}/Contents )
    ENDIF()

    SET( VER_FILENAME "${PACKAGE_NAME}_${OD_PLFSUBDIR}" )
    IF( ${PACKAGE_NAME} STREQUAL "basedata" )
        SET( VER_FILENAME "basedata" )
	IF( APPLE )
	    SET( VER_FILENAME "basedata_mac" )
	ENDIF()
    ELSEIF( ${PACKAGE_NAME} STREQUAL "dgbbasedata" )
        SET( VER_FILENAME "dgbbasedata" )
	IF( APPLE )
	    SET( VER_FILENAME "dgbbasedata_mac" )
	ENDIF()
    ELSEIF( ${PACKAGE_NAME} STREQUAL "doc" )
        SET( VER_FILENAME "doc" )
	IF( APPLE )
	    SET( VER_FILENAME "doc_mac" )
	ENDIF()
    ELSEIF( ${PACKAGE_NAME} STREQUAL "dgbdoc" )
        SET( VER_FILENAME "dgbdoc" )
	IF( APPLE )
	    SET( VER_FILENAME "dgbdoc_mac" )
	ENDIF()
    ELSEIF( ${PACKAGE_NAME} STREQUAL "classdoc" )
        SET( VER_FILENAME "classdoc" )
	IF( APPLE )
	    SET( VER_FILENAME "classdoc_mac" )
	ENDIF()
    ENDIF()

    FILE( WRITE ${DESTINATION_DIR}/relinfo/ver.${VER_FILENAME}.txt ${FULLVER_NAME} )
    FILE( APPEND ${DESTINATION_DIR}/relinfo/ver.${VER_FILENAME}.txt "\n" )

    IF( APPLE )
	IF( ${PACKAGE_NAME} STREQUAL "base" )
	    FILE( WRITE ${CMAKE_INSTALL_PREFIX}/macos_link
		  "#!/bin/csh -f\nln -s ../bin/mac MacOS")
	    FILE( APPEND ${CMAKE_INSTALL_PREFIX}/macos_link "\n" )
	ENDIF()
    ENDIF()
endmacro( init_destinationdir )


macro( create_develpackages )
    FILE( MAKE_DIRECTORY ${DESTINATION_DIR}/doc
			 ${DESTINATION_DIR}/doc/Programmer)
    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			     ${PSD}/CMakeLists.txt ${DESTINATION_DIR} )
    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
		     ${CMAKE_INSTALL_PREFIX}/doc/Programmer/batchprogexample
		     ${DESTINATION_DIR}/doc/Programmer/batchprogexample )
    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
		     ${CMAKE_INSTALL_PREFIX}/doc/Programmer/pluginexample
		     ${DESTINATION_DIR}/doc/Programmer/pluginexample )
    FILE( GLOB HTMLFILES ${PSD}/doc/Programmer/*.html )
    FOREACH( HTMLFILE ${HTMLFILES} )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${HTMLFILE} ${DESTINATION_DIR}/doc/Programmer )
    ENDFOREACH()
    FILE( GLOB PNGFILES ${PSD}/doc/Programmer/*.png )
    FOREACH( PNGFILE ${PNGFILES} )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${PNGFILE} ${DESTINATION_DIR}/doc/Programmer )
    ENDFOREACH()

    FOREACH( DIR CMakeModules include src plugins spec tests )
	Message( "Copying ${DIR} files" )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			 ${CMAKE_INSTALL_PREFIX}/${DIR}
			 ${DESTINATION_DIR}/${DIR} )
    ENDFOREACH()
    Message( "Copying Pmake stuff" )
    FOREACH( DIR ${PMAKESTUFF} )
	IF( IS_DIRECTORY "${PSD}/Pmake/${DIR}" )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${PSD}/Pmake/${DIR} ${DESTINATION_DIR}/Pmake/${DIR} )
	ELSE()
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			     ${PSD}/Pmake/${DIR} ${DESTINATION_DIR}/Pmake/${DIR} )
	ENDIF()
#//TODO File permissions are chaged after install. We need to copy as it is. 
#But '-E copy/copy_directory'option is working.
    ENDFOREACH()

#TODO Fond correct way to avoind so many for loops
    IF( WIN32 )
	FILE( MAKE_DIRECTORY ${DESTINATION_DIR}/bin
			     ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}
			     ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/debug
			     ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/release )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
				 ${PSD}/bin/od_cr_dev_env.bat
				 ${DESTINATION_DIR}/bin )
	FOREACH( WLIB ${SRCLIBLIST} )
	    FILE( GLOB DEBUGLIBS
			    ${CMAKE_SOURCE_DIR}/bin/${OD_PLFSUBDIR}/Debug/${WLIB}.lib )
	    FOREACH( DEBUGLIB ${DEBUGLIBS} )
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${DEBUGLIB}
					 ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/debug )
	    ENDFOREACH()
	    FILE( GLOB DEBUGDLLS
			    ${CMAKE_SOURCE_DIR}/bin/${OD_PLFSUBDIR}/Debug/${WLIB}.dll)
	    FOREACH( DEBUGDLL ${DEBUGDLLS} )
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${DEBUGDLL}
					 ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/debug )
	    ENDFOREACH()
	    FILE( GLOB DEBUGPDBS
			    ${CMAKE_SOURCE_DIR}/bin/${OD_PLFSUBDIR}/Debug/${WLIB}.pdb )
	    FOREACH( DEBUGPDB ${DEBUGPDBS} )
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${DEBUGPDB}
					 ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/debug )
	    ENDFOREACH()
	    FILE( GLOB RELEASELIBS
			${CMAKE_SOURCE_DIR}/bin/${OD_PLFSUBDIR}/Release/${WLIB}.lib )
	    FOREACH( RELEASELIB ${RELEASELIBS} )
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${RELEASELIB}
					 ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/release )
	    ENDFOREACH()
	ENDFOREACH()

	FOREACH( WELIB ${EXECLIST} )
	    FILE( GLOB DEBUGEPDBS
			${CMAKE_SOURCE_DIR}/bin/${OD_PLFSUBDIR}/Debug/${WELIB}.pdb )
	    FOREACH( DEBUGEPDB ${DEBUGEPDBS} )
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${DEBUGEPDB}
					 ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/debug )
	    ENDFOREACH()
	    FILE( GLOB DEBUGEXES
			${CMAKE_SOURCE_DIR}/bin/${OD_PLFSUBDIR}/Debug/${WELIB}.exe )
	    FOREACH( DEBUGEXE ${DEBUGEXES} )
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${DEBUGEXE}
					 ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/debug )
	    ENDFOREACH()
	ENDFOREACH()

    ENDIF()

    IF( WIN32 )
	MESSAGE( "Using ${OD_PLFSUBDIR} zip command" )
	execute_process( COMMAND ${PSD}/bin/win/zip -r -q
					   "${PACKAGE_FILENAME}" ${REL_DIR} 
				 WORKING_DIRECTORY ${PACKAGE_DIR}
				 RESULT_VARIABLE STATUS )
    ELSE()
	MESSAGE( "Using ${OD_PLFSUBDIR} zip command" )
	execute_process( COMMAND zip -r -y -q "${PACKAGE_FILENAME}" ${REL_DIR} 
				 WORKING_DIRECTORY ${PACKAGE_DIR}
				 RESULT_VARIABLE STATUS )
    ENDIF()

    IF( NOT STATUS EQUAL "0" )
	message("Failed while creating devel package")
    ENDIF()
endmacro( create_develpackages )


macro( od_sign_libs )
    IF( APPLE )
	MESSAGE( "Signing mac libs..." )
	SET ( SIGN_ID "Developer ID Application: DGB-Earth Sciences B. V." )
	FILE( GLOB LIBFILES ${CMAKE_INSTALL_PREFIX}/bin/mac/*.dylib )
	FILE( GLOB EXEFILES ${CMAKE_INSTALL_PREFIX}/bin/mac/od_* )
	SET ( FILES ${LIBFILES} ${EXEFILES} )
	FOREACH( FIL ${FILES} )
	    execute_process( COMMAND  codesign -f -s ${SIGN_ID} ${FIL} RESULT_VARIABLE STATUS )
	    IF( NOT STATUS EQUAL "0" )
		message(FATAL_ERROR "Failed while signing mac libs")
	    ENDIF()
	ENDFOREACH()
    ELSEIF( WIN32 )
	execute_process( COMMAND ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/sign.bat
		WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}
				 RESULT_VARIABLE STATUS )
	IF( NOT STATUS EQUAL "0" )
	    message("Failed while signing windows libs")
	ENDIF()
    ENDIF()
    MESSAGE( "Done" )
endmacro( od_sign_libs )

macro( download_packages  )
    message( "downloading doc pkgs" )
    SET ( DOCNAMES appman workflows user dgb )
    FOREACH( DOCNAME ${DOCNAMES} )
	SET( url "http://intranet/documentations/rel/" )
	SET( url "${url}/${DOCNAME}doc.zip" )
	SET( DIRNAME ${DOCNAME} )
        IF( ${DOCNAME} STREQUAL "appman" )
	    SET( DIRNAME SysAdm )
	ELSEIF( ${DOCNAME} STREQUAL "user")
	    SET( DIRNAME base )
	ENDIF()

	IF( EXISTS ${CMAKE_INSTALL_PREFIX}/doc/${DIRNAME} )
	    FILE( REMOVE_RECURSE ${CMAKE_INSTALL_PREFIX}/doc/${DIRNAME} )
	ENDIF()

	FILE( DOWNLOAD ${url} "${CMAKE_INSTALL_PREFIX}/doc/${DIRNAME}/${DOCNAME}doc.zip"
	      STATUS var
	      LOG log
	      SHOW_PROGRESS)
	IF( NOT var EQUAL "0" )
	    MESSAGE( "......... ${url} Download Failed.........")
	ELSE()
	    execute_process( COMMAND unzip -o -q
			     ${CMAKE_INSTALL_PREFIX}/doc/${DIRNAME}/${DOCNAME}doc.zip
			     -d ${CMAKE_INSTALL_PREFIX}/doc/${DIRNAME}
			     RESULT_VARIABLE STATUS )
	    FILE( REMOVE_RECURSE ${CMAKE_INSTALL_PREFIX}/doc/${DIRNAME}/${DOCNAME}doc.zip )
	    IF( NOT STATUS EQUAL "0" )
		MESSAGE( "......... unzip Failed.........")
	    ENDIF()
	ENDIF()
    ENDFOREACH()
endmacro( download_packages )

macro( create_docpackages PACKAGE_NAME )
    IF( WIN32 )
	MESSAGE( "Documentation on windows platform is not implemented" )
    ELSE()
	IF( ${PACKAGE_NAME} STREQUAL "doc" )
	    IF( EXISTS ${CMAKE_INSTALL_PREFIX}/doc/base/LinkFileTable.txt )
		FILE( RENAME ${CMAKE_INSTALL_PREFIX}/doc/base/LinkFileTable.txt
			     ${CMAKE_INSTALL_PREFIX}/doc/od_LinkFileTable.txt )
	    ENDIF()

	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${CMAKE_INSTALL_PREFIX}/doc/SysAdm
			     ${DESTINATION_DIR}/doc/SysAdm )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${CMAKE_INSTALL_PREFIX}/doc/Scripts
			     ${DESTINATION_DIR}/doc/Scripts )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${CMAKE_INSTALL_PREFIX}/doc/workflows
			     ${DESTINATION_DIR}/doc/User/workflows )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${CMAKE_INSTALL_PREFIX}/doc/base
			     ${DESTINATION_DIR}/doc/User/base )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${CMAKE_INSTALL_PREFIX}/doc/Credits/base
			     ${DESTINATION_DIR}/doc/Credits/base )
	ELSEIF( ${PACKAGE_NAME} STREQUAL "dgbdoc" )
#install Credits/dgb
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${CMAKE_INSTALL_PREFIX}/doc/Credits/dgb
			     ${DESTINATION_DIR}/doc/Credits/dgb )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${CMAKE_INSTALL_PREFIX}/doc/dgb
			     ${DESTINATION_DIR}/doc/User/dgb )
	    FILE( GLOB FILES ${PSD}/doc/flexnet* )
	    FOREACH( FIL ${FILES} )
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${FIL} ${DESTINATION_DIR}/doc )
	    ENDFOREACH()
	    IF( EXISTS ${CMAKE_INSTALL_PREFIX}/doc/dgb/LinkFileTable.txt )
		FILE( RENAME ${CMAKE_INSTALL_PREFIX}/doc/dgb/LinkFileTable.txt
			     ${CMAKE_INSTALL_PREFIX}/doc/dgb_LinkFileTable.txt )
	    ENDIF()
	ELSEIF( ${PACKAGE_NAME} STREQUAL "classdoc" )
	    IF ( EXISTS ${CMAKE_INSTALL_PREFIX}/doc/Programmer/Generated )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
				     ${CMAKE_INSTALL_PREFIX}/doc/Programmer/Generated
				     ${DESTINATION_DIR}/doc/Programmer/Generated )
	    ELSE()
		MESSAGE( "Class doc does not install.${PACKAGE_FILENAME} is not self contained." )
	    ENDIF()
	ENDIF()
	execute_process( COMMAND zip -r -y -q "${PACKAGE_FILENAME}" ${REL_DIR} 
				 WORKING_DIRECTORY ${PACKAGE_DIR}
				 RESULT_VARIABLE STATUS )
	ENDIF()
endmacro( create_docpackages )
