#######################################################################################
#
#  Copyright 2020 Alexander Stukowski
#
#  This file is part of OVITO (Open Visualization Tool).
#
#  OVITO is free software; you can redistribute it and/or modify it either under the
#  terms of the GNU General Public License version 3 as published by the Free Software
#  Foundation (the "GPL") or, at your option, under the terms of the MIT License.
#  If you do not alter this notice, a recipient may use your version of this
#  file under either the GPL or the MIT License.
#
#  You should have received a copy of the GPL along with this program in a
#  file LICENSE.GPL.txt.  You should have received a copy of the MIT License along
#  with this program in a file LICENSE.MIT.txt
#
#  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND,
#  either express or implied. See the GPL or the MIT License for the specific language
#  governing rights and limitations.
#
#######################################################################################

# This CMake script compiles the user manual for OVITO
# by transforming the docbook files to HTML.

# Controls the generation of the user manual.
OPTION(OVITO_BUILD_DOCUMENTATION "Build the user manual" "OFF")

# Find the XSLT processor program.
FIND_PROGRAM(XSLT_PROCESSOR "xsltproc" DOC "Path to the XSLT processor program used to build the documentation.")
IF(NOT XSLT_PROCESSOR)
	IF(OVITO_BUILD_DOCUMENTATION)
		MESSAGE(FATAL_ERROR "The XSLT processor program (xsltproc) was not found. Please install it and/or specify its location manually.")
	ENDIF()
ENDIF()
SET(XSLT_PROCESSOR_OPTIONS "--xinclude" CACHE STRING "Additional to pass to the XSLT processor program when building the documentation")
MARK_AS_ADVANCED(XSLT_PROCESSOR_OPTIONS)

# XSL transform documentation files.
IF(XSLT_PROCESSOR AND OVITO_BUILD_GUI)
	# Create destination directories.
	FILE(MAKE_DIRECTORY "${OVITO_SHARE_DIRECTORY}/doc/manual")
	FILE(MAKE_DIRECTORY "${OVITO_SHARE_DIRECTORY}/doc/manual/html")

	# This CMake target generates the user manual as a set of static HTML pages which get shipped with
	# the Ovito installation packages and which can be accessed from the Help menu of the application.
	ADD_CUSTOM_TARGET(documentation
					COMMAND ${CMAKE_COMMAND} "-E" copy_directory "images/" "${OVITO_SHARE_DIRECTORY}/doc/manual/html/images/"
					COMMAND ${CMAKE_COMMAND} "-E" copy "manual.css" "${OVITO_SHARE_DIRECTORY}/doc/manual/html/"
					COMMAND ${XSLT_PROCESSOR} "${XSLT_PROCESSOR_OPTIONS}" --nonet --stringparam base.dir "${OVITO_SHARE_DIRECTORY}/doc/manual/html/" html-customization-layer.xsl Manual.docbook
					WORKING_DIRECTORY "${Ovito_SOURCE_DIR}/doc/manual/"
					COMMENT "Generating user documentation")

	# Install the documentation alongside with the application.
	INSTALL(DIRECTORY "${OVITO_SHARE_DIRECTORY}/doc/manual/html/" DESTINATION "${OVITO_RELATIVE_SHARE_DIRECTORY}/doc/manual/html/")
	
	IF(OVITO_BUILD_DOCUMENTATION)
		ADD_DEPENDENCIES(Ovito documentation)
	ENDIF()
ENDIf()

# Find the Doxygen program.
FIND_PACKAGE(Doxygen QUIET)

# Generate API documentation files.
IF(DOXYGEN_FOUND)
	ADD_CUSTOM_TARGET(apidocs
					COMMAND "${CMAKE_COMMAND}" -E env
					"OVITO_VERSION_STRING=${OVITO_VERSION_MAJOR}.${OVITO_VERSION_MINOR}.${OVITO_VERSION_REVISION}"
					"OVITO_INCLUDE_PATH=${Ovito_SOURCE_DIR}/src/"
					${DOXYGEN_EXECUTABLE} Doxyfile
					WORKING_DIRECTORY "${Ovito_SOURCE_DIR}/doc/develop/"
					COMMENT "Generating C++ API documentation")
ENDIF()
