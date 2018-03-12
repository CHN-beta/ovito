###############################################################################
# 
#  Copyright (2018) Alexander Stukowski
#
#  This file is part of OVITO (Open Visualization Tool).
#
#  OVITO is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  OVITO is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
###############################################################################

###############################################################################
# The netcdf_integration CMake target provides a helper module that can be
# used by other modules in OVITO that make use of the NetCDF library. It 
# pulls in the necessary header files and system libraries and provides the
# helper class NetCDFExclusiveAccess, which manages concurrent access to the
# NetCDF functions, which are not thread-safe.
###############################################################################

# Locate the HDF5 library.
IF(WIN32 OR (UNIX AND NOT APPLE AND OVITO_REDISTRIBUTABLE_PACKAGE))
	FIND_PACKAGE(HDF5 COMPONENTS hdf5 REQUIRED NO_MODULE)
ENDIF()

# Locate the NetCDF library.
FIND_PACKAGE(NetCDF REQUIRED)

# Build library.
ADD_LIBRARY(NetCDFIntegration ${OVITO_DEFAULT_LIBRARY_TYPE} NetCDFIntegration.cpp)

# Link to Qt5.
TARGET_LINK_LIBRARIES(NetCDFIntegration PUBLIC Qt5::Core)

# This library depends on the Core module of OVITO.
TARGET_LINK_LIBRARIES(NetCDFIntegration PUBLIC Core)

# Define macro for symbol export from the shared library.
IF(OVITO_BUILD_MONOLITHIC)
	TARGET_COMPILE_DEFINITIONS(NetCDFIntegration PUBLIC "OVITO_NETCDF_INTEGRATION_EXPORT=")
ELSE()
	TARGET_COMPILE_DEFINITIONS(NetCDFIntegration PRIVATE "OVITO_NETCDF_INTEGRATION_EXPORT=Q_DECL_EXPORT")
	TARGET_COMPILE_DEFINITIONS(NetCDFIntegration INTERFACE "OVITO_NETCDF_INTEGRATION_EXPORT=Q_DECL_IMPORT")
ENDIF()
	
# Link in the NetCDF library
TARGET_LINK_LIBRARIES(NetCDFIntegration PUBLIC netcdf)
TARGET_INCLUDE_DIRECTORIES(NetCDFIntegration PUBLIC "${netcdf_INCLUDE_DIRS}")
IF(WIN32)
	TARGET_COMPILE_DEFINITIONS(NetCDFIntegration PUBLIC DLL_NETCDF)
ENDIF()

SET_PROPERTY(TARGET NetCDFIntegration PROPERTY POSITION_INDEPENDENT_CODE ON)

# Export this target.
INSTALL(TARGETS NetCDFIntegration EXPORT OVITO 
	RUNTIME DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}" COMPONENT "runtime"
	LIBRARY DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}" COMPONENT "runtime"
	ARCHIVE DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}" COMPONENT "development")

IF(WIN32)

	# Deploy NetCDF DLLs.
	GET_TARGET_PROPERTY(NETCDF_DLL_LOCATION netcdf IMPORTED_LOCATION_RELEASE)
	OVITO_INSTALL_DLL("${NETCDF_DLL_LOCATION}")

	# Deploy HDF5 DLLs, which are required by NetCDF library.
	GET_TARGET_PROPERTY(HDF5_DLL_LOCATION hdf5-shared IMPORTED_LOCATION_RELEASE)
	IF(NOT HDF5_DLL_LOCATION)
		GET_TARGET_PROPERTY(HDF5_DLL_LOCATION hdf5 IMPORTED_LOCATION_RELEASE)
	ENDIF()
	IF(NOT HDF5_DLL_LOCATION)
		GET_TARGET_PROPERTY(HDF5_DLL_LOCATION hdf5::hdf5-shared IMPORTED_LOCATION_RELEASE)
	ENDIF()
	OVITO_INSTALL_DLL("${HDF5_DLL_LOCATION}")
	GET_TARGET_PROPERTY(HDF5_HL_DLL_LOCATION hdf5_hl-shared IMPORTED_LOCATION_RELEASE)
	IF(NOT HDF5_HL_DLL_LOCATION)
		GET_TARGET_PROPERTY(HDF5_HL_DLL_LOCATION hdf5_hl IMPORTED_LOCATION_RELEASE)
	ENDIF()
	IF(NOT HDF5_HL_DLL_LOCATION)
		GET_TARGET_PROPERTY(HDF5_HL_DLL_LOCATION hdf5::hdf5_hl-shared IMPORTED_LOCATION_RELEASE)
	ENDIF()
	OVITO_INSTALL_DLL("${HDF5_HL_DLL_LOCATION}")

ELSEIF(UNIX AND NOT APPLE AND OVITO_REDISTRIBUTABLE_PACKAGE)

	# Deploy NetCDF shared library.
	GET_TARGET_PROPERTY(NETCDF_LIBRARY netcdf LOCATION)
	OVITO_INSTALL_SHARED_LIB("${NETCDF_LIBRARY}" "${OVITO_RELATIVE_LIBRARY_DIRECTORY}")

	# Deploy HDF5 shared libraries, which are required by NetCDF lib.
	GET_TARGET_PROPERTY(HDF5_LIBRARY hdf5-shared IMPORTED_LOCATION_RELEASE)
	OVITO_INSTALL_SHARED_LIB("${HDF5_LIBRARY}" "${OVITO_RELATIVE_LIBRARY_DIRECTORY}")
	GET_TARGET_PROPERTY(HDF5_HL_LIBRARY hdf5_hl-shared IMPORTED_LOCATION_RELEASE)
	OVITO_INSTALL_SHARED_LIB("${HDF5_HL_LIBRARY}" "${OVITO_RELATIVE_LIBRARY_DIRECTORY}")

ENDIF()