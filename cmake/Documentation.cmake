#######################################################################################
#
#  Copyright 2020 OVITO GmbH, Germany
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
# by transforming the Docbook or reStructured text files into HTML.

# Controls the generation of the user manual.
OPTION(OVITO_BUILD_DOCUMENTATION "Build the user manual" "OFF")

# Locate the XSLT processor program, which transforms Docbook documents into the output format.
FIND_PROGRAM(XSLT_PROCESSOR "xsltproc" DOC "Path to the XSLT processor program used to build the documentation.")
IF(NOT XSLT_PROCESSOR)
	IF(OVITO_BUILD_DOCUMENTATION)
		MESSAGE(FATAL_ERROR "The XSLT processor program (xsltproc) was not found. Please install it and/or specify its location manually.")
	ENDIF()
ENDIF()
SET(XSLT_PROCESSOR_OPTIONS "--xinclude" CACHE STRING "Additional to pass to the XSLT processor program when building the documentation")
MARK_AS_ADVANCED(XSLT_PROCESSOR_OPTIONS)

IF(OVITO_BUILD_GUI)
	# Create destination directories.
	FILE(MAKE_DIRECTORY "${OVITO_SHARE_DIRECTORY}/doc/manual")
	FILE(MAKE_DIRECTORY "${OVITO_SHARE_DIRECTORY}/doc/manual/html")

	# XSL transform Docbook files.
	IF(XSLT_PROCESSOR)

		# This CMake target generates the user manual as a set of static HTML pages which get shipped with
		# the Ovito installation packages and which can be accessed from the Help menu of the application.
		ADD_CUSTOM_TARGET(documentation
						COMMAND ${CMAKE_COMMAND} "-E" copy_directory "images/" "${OVITO_SHARE_DIRECTORY}/doc/manual/html/images/"
						COMMAND ${CMAKE_COMMAND} "-E" copy "manual.css" "${OVITO_SHARE_DIRECTORY}/doc/manual/html/"
						COMMAND ${XSLT_PROCESSOR} "${XSLT_PROCESSOR_OPTIONS}" --nonet --stringparam base.dir "${OVITO_SHARE_DIRECTORY}/doc/manual/html/" html-customization-layer.xsl Manual.docbook
						WORKING_DIRECTORY "${Ovito_SOURCE_DIR}/doc/manual/"
						COMMENT "Generating user documentation")
		
		IF(OVITO_BUILD_DOCUMENTATION)
			# Run the documentation target automatically as part of the build process.
			ADD_DEPENDENCIES(Ovito documentation)
			# Install the generated documentation files alongside with the application.
			INSTALL(DIRECTORY "${OVITO_SHARE_DIRECTORY}/doc/manual/html/" DESTINATION "${OVITO_RELATIVE_SHARE_DIRECTORY}/doc/manual/html/" PATTERN "*.pptx" EXCLUDE)
		ENDIF()
	ENDIf()

	# Locate a Python interpreter.
	# Note: Sphinx 3.x requires at least Python 3.5.
	IF(CMAKE_VERSION VERSION_LESS 3.12)
		FIND_PACKAGE(PythonInterp 3.5 REQUIRED)
		SET(Python3_EXECUTABLE "${PYTHON_EXECUTABLE}")
	ELSE()
		FIND_PACKAGE(Python3 REQUIRED COMPONENTS Interpreter)
	ENDIF()

	# Get current year.
	STRING(TIMESTAMP current_year "%Y")

	# Build reStructuredText documentation using Sphinx.
	ADD_CUSTOM_TARGET(documentation_sphinx
					# Create the destination directory for the generated HTML files:
					COMMAND "${CMAKE_COMMAND}" -E make_directory "${OVITO_SHARE_DIRECTORY}/doc/manual/sphinx"
					# Run Sphinx command to generate HTML files:
					COMMAND "${Python3_EXECUTABLE}" "${Ovito_SOURCE_DIR}/doc/manual/sphinx-build.py" 
								"."                                           # Sphinx source directory 
								"${OVITO_SHARE_DIRECTORY}/doc/manual/sphinx/" # Destination directory
								-n                                            # Run in nit-picky mode
								-W                                            # Turn warnings into errors
								# Additional config settings passed to Sphinx, which are added to the options found in conf.py: 
								-D "version=${OVITO_VERSION_MAJOR}.${OVITO_VERSION_MINOR}"
								-D "release=${OVITO_VERSION_STRING}"
								-D "copyright=${current_year} OVITO GmbH, Germany"

					# Run Sphinx spellchecker:
					# Note: Setting the DYLD_FALLBACK_LIBRARY_PATH environment variable is needed for PyEnchant module to find the libenchant.dylib from MacPorts.
					COMMAND "${CMAKE_COMMAND}" -E env DYLD_FALLBACK_LIBRARY_PATH=/opt/local/lib "${Python3_EXECUTABLE}" "${Ovito_SOURCE_DIR}/doc/manual/sphinx-build.py" 
								"."                                           # Sphinx source directory 
								"${OVITO_SHARE_DIRECTORY}/doc/manual/sphinx/" # Destination directory
								-W                                            # Turn warnings into errors
								-b spelling                                   # Execute the 'spelling' builder

					WORKING_DIRECTORY "${Ovito_SOURCE_DIR}/doc/manual/"
					COMMENT "Generating user documentation using Sphinx")
ENDIF()

# Find the Doxygen program.
#FIND_PACKAGE(Doxygen QUIET)

# Generate API documentation files.
#IF(DOXYGEN_FOUND)
#	ADD_CUSTOM_TARGET(apidocs
#					COMMAND "${CMAKE_COMMAND}" -E env
#					"OVITO_VERSION_STRING=${OVITO_VERSION_MAJOR}.${OVITO_VERSION_MINOR}.${OVITO_VERSION_REVISION}"
#					"OVITO_INCLUDE_PATH=${Ovito_SOURCE_DIR}/src/"
#					${DOXYGEN_EXECUTABLE} Doxyfile
#					WORKING_DIRECTORY "${Ovito_SOURCE_DIR}/doc/develop/"
#					COMMENT "Generating C++ API documentation")
#ENDIF()
