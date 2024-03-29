#######################################################################################
#
#  Copyright 2019 OVITO GmbH, Germany
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

###############################################################################
# The netcdf_integration CMake target provides a helper module that can be
# used by other modules in OVITO that make use of the NetCDF library. It
# pulls in the necessary header files and system libraries and provides the
# helper class NetCDFExclusiveAccess, which manages concurrent access to the
# NetCDF functions, which are not thread-safe.
###############################################################################

IF(NOT OVITO_BUILD_CONDA)
	# Locate the HDF5 library.
	FIND_PACKAGE(HDF5 COMPONENTS C HL)
ENDIF()

# Locate the NetCDF library.
FIND_PACKAGE(NetCDF)

IF((HDF5_FOUND OR OVITO_BUILD_CONDA) AND (TARGET netcdf OR TARGET netCDF::netcdf))

	# Build library.
	ADD_LIBRARY(NetCDFIntegration NetCDFIntegration.cpp)

	# Give our library file a name to not confuse it with any system versions of the library.
	SET_TARGET_PROPERTIES(NetCDFIntegration PROPERTIES OUTPUT_NAME "ovito_netcdf")

	# Link to Qt.
	FIND_PACKAGE(${OVITO_QT_MAJOR_VERSION} ${OVITO_MINIMUM_REQUIRED_QT_VERSION} COMPONENTS Core REQUIRED)
	TARGET_LINK_LIBRARIES(NetCDFIntegration PUBLIC ${OVITO_QT_MAJOR_VERSION}::Core)

	# This library depends on the Core module of OVITO.
	TARGET_LINK_LIBRARIES(NetCDFIntegration PUBLIC Core)

	# Inform source code that the NetCDF library is available.
	TARGET_COMPILE_DEFINITIONS(NetCDFIntegration PUBLIC OVITO_NETCDF_SUPPORT)

	# Define macro for symbol export from the shared library.
	IF(BUILD_SHARED_LIBS)
		TARGET_COMPILE_DEFINITIONS(NetCDFIntegration PRIVATE "OVITO_NETCDF_INTEGRATION_EXPORT=Q_DECL_EXPORT")
		TARGET_COMPILE_DEFINITIONS(NetCDFIntegration INTERFACE "OVITO_NETCDF_INTEGRATION_EXPORT=Q_DECL_IMPORT")
	ELSE()
		TARGET_COMPILE_DEFINITIONS(NetCDFIntegration PUBLIC "OVITO_NETCDF_INTEGRATION_EXPORT=")
	ENDIF()

	# Link the NetCDF library
	IF(TARGET netcdf)
		TARGET_LINK_LIBRARIES(NetCDFIntegration PUBLIC netcdf)
		TARGET_INCLUDE_DIRECTORIES(NetCDFIntegration PUBLIC "${netcdf_INCLUDE_DIRS}")
	ELSE()
		TARGET_LINK_LIBRARIES(NetCDFIntegration PUBLIC netCDF::netcdf)
	ENDIF()
	IF(WIN32)
		TARGET_COMPILE_DEFINITIONS(NetCDFIntegration PUBLIC DLL_NETCDF)
	ENDIF()

	IF(NOT BUILD_SHARED_LIBS AND OVITO_BUILD_PYPI)
		# Since we will link this library into the dynamically loaded Python extension module, we need to use the fPIC flag.
		SET_PROPERTY(TARGET NetCDFIntegration PROPERTY POSITION_INDEPENDENT_CODE ON)
	ENDIF()

	# Export this target.
	INSTALL(TARGETS NetCDFIntegration EXPORT OVITO
		RUNTIME DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}"
		LIBRARY DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}"
		ARCHIVE DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}" COMPONENT "development")

	IF(NOT APPLE AND NOT OVITO_BUILD_CONDA)

		# Deploy NetCDF shared library.
		IF(TARGET netcdf)
			GET_TARGET_PROPERTY(NETCDF_LIBRARY netcdf LOCATION)
		ELSE()
			GET_TARGET_PROPERTY(NETCDF_LIBRARY netCDF::netcdf LOCATION)
		ENDIF()
		OVITO_INSTALL_SHARED_LIB("${NETCDF_LIBRARY}" ".")

		# Deploy HDF5 shared library, which is required by NetCDF lib.
		IF(TARGET hdf5-shared)
			GET_TARGET_PROPERTY(HDF5_LIBRARY hdf5-shared IMPORTED_LOCATION_RELEASE)
		ENDIF()
		IF(NOT HDF5_LIBRARY AND TARGET hdf5::hdf5-shared)
			GET_TARGET_PROPERTY(HDF5_LIBRARY hdf5::hdf5-shared IMPORTED_LOCATION_RELEASE)
		ENDIF()
		IF(NOT HDF5_LIBRARY AND UNIX)
			LIST(GET HDF5_LIBRARIES 0 HDF5_LIBRARY)
		ENDIF()
		IF(NOT HDF5_LIBRARY)
			MESSAGE(FATAL_ERROR "CMake target for the HDF5 library does not exist. Please check you HDF5 installation.")
		ENDIF()
		OVITO_INSTALL_SHARED_LIB("${HDF5_LIBRARY}" ".")

		# Deploy HDF5_HL shared library, which is required by NetCDF lib.
		IF(TARGET hdf5_hl-shared)
			GET_TARGET_PROPERTY(HDF5_HL_LIBRARY hdf5_hl-shared IMPORTED_LOCATION_RELEASE)
		ENDIF()
		IF(NOT HDF5_HL_LIBRARY AND TARGET hdf5::hdf5_hl-shared)
			GET_TARGET_PROPERTY(HDF5_HL_LIBRARY hdf5::hdf5_hl-shared IMPORTED_LOCATION_RELEASE)
		ENDIF()
		IF(NOT HDF5_HL_LIBRARY AND UNIX)
					LIST(GET HDF5_HL_LIBRARIES 0 HDF5_HL_LIBRARY)
		ENDIF()
		IF(NOT HDF5_HL_LIBRARY)
			MESSAGE(FATAL_ERROR "CMake target for the HDF5_HL library does not exist. Please check you HDF5 installation.")
		ENDIF()
		OVITO_INSTALL_SHARED_LIB("${HDF5_HL_LIBRARY}" ".")

	ENDIF()

ELSE()

	MESSAGE("HDF5/NetCDF libraries not found. OVITO will be built without support for NetCDF-based file formats.")

ENDIF()
