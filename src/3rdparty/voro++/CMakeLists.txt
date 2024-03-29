#######################################################################################
#
#  Copyright 2021 OVITO GmbH, Germany
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

IF(WIN32)
	# Always build this module as a static library under Windows, because Voro++ does not explicitly export any symbols.
	# Building as a DLL would require tagging all symbols to be exported with __declspec(dllexport) in the source  code.
	SET(BUILD_SHARED_LIBS OFF)
ENDIF()

# Build library.
ADD_LIBRARY(VoroPlusPlus
	src/cell.cc
	src/common.cc
	src/container.cc
	src/unitcell.cc
	src/v_compute.cc
	src/c_loops.cc
	src/v_base.cc
	src/wall.cc
	src/pre_container.cc
	src/container_prd.cc
)

# Simply export all symbols defined by the Voro++ library, because the code doesn't explicitly export certain API symbols.
SET_TARGET_PROPERTIES(VoroPlusPlus PROPERTIES CXX_VISIBILITY_PRESET "default")

# Give our library file a new name to not confuse it with any system versions of the library.
SET_TARGET_PROPERTIES(VoroPlusPlus PROPERTIES OUTPUT_NAME "ovito_voro++")

# Make header files of this library available to dependent targets.
TARGET_INCLUDE_DIRECTORIES(VoroPlusPlus INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src")

IF(MSVC)
	# Do not warn about use of unsafe CRT Library functions.
	TARGET_COMPILE_DEFINITIONS(VoroPlusPlus PRIVATE "_CRT_SECURE_NO_WARNINGS=")
	# Suppress warning on conversion from size_t to int, possible loss of data.
	TARGET_COMPILE_OPTIONS(VoroPlusPlus PRIVATE "/wd4267")
ENDIF()

IF(NOT BUILD_SHARED_LIBS AND OVITO_BUILD_PYPI)
	# Since we will link this library into the dynamically loaded Python extension module, we need to use the fPIC flag.
	SET_PROPERTY(TARGET VoroPlusPlus PROPERTY POSITION_INDEPENDENT_CODE ON)
ENDIF()

IF(EMSCRIPTEN)
	# Adopt the Emscript compiler flags from Qt's platform target.
	# That's because all libraries linked into the static WASM module need be built with the same flags.
	TARGET_LINK_LIBRARIES(VoroPlusPlus PRIVATE Qt::Platform)
ENDIF()

# Export this target.
INSTALL(TARGETS VoroPlusPlus EXPORT OVITO
	RUNTIME DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}"
	LIBRARY DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}"
	ARCHIVE DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}" COMPONENT "development")
