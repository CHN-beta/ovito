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

# Tell CMake to run Qt moc whenever necessary.
SET(CMAKE_AUTOMOC ON)
# Tell CMake to run the Qt resource compiler on all .qrc files added to a target.
SET(CMAKE_AUTORCC ON)

# The set of required Qt modules:
LIST(APPEND OVITO_REQUIRED_QT_COMPONENTS Core Gui Xml)
IF(OVITO_QT_MAJOR_VERSION STREQUAL "Qt6")
	# QOpenGLFunctions classes have been moved to the OpenGL module in Qt 6.
	LIST(APPEND OVITO_REQUIRED_QT_COMPONENTS OpenGL OpenGLWidgets)
ENDIF()
IF(OVITO_BUILD_GUI)
	# Note: QtConcurrent and QtPrintSupport are a dependency of the Qwt library.
	# Note: QtDBus is an indirect dependency of the Xcb platform plugin under Linux.
	LIST(APPEND OVITO_REQUIRED_QT_COMPONENTS Widgets Concurrent PrintSupport Svg DBus)
ENDIF()
IF(NOT EMSCRIPTEN)
	# The Qt Network module is used by the OVITO Core module, except in the wasm build.
	LIST(APPEND OVITO_REQUIRED_QT_COMPONENTS Network)
ENDIF()
IF(OVITO_BUILD_WEBGUI)
	# The user interface is implemented using Qt Qml and Quick when running inside a web browser.
	LIST(APPEND OVITO_REQUIRED_QT_COMPONENTS Qml Quick QuickControls2 QuickTemplates2 Svg)
	# Additionally, when building for the desktop platform, we need the QtWidgets module.
	IF(NOT EMSCRIPTEN)
		LIST(APPEND OVITO_REQUIRED_QT_COMPONENTS Widgets)
	ENDIF()
ENDIF()

# Find the required Qt modules.
IF(OVITO_QT_MAJOR_VERSION STREQUAL "Qt6")
	FIND_PACKAGE(Qt6 COMPONENTS ${OVITO_REQUIRED_QT_COMPONENTS} REQUIRED)
ELSEIF(OVITO_QT_MAJOR_VERSION STREQUAL "Qt5")
	FIND_PACKAGE(Qt5 5.12 COMPONENTS ${OVITO_REQUIRED_QT_COMPONENTS} REQUIRED)
ELSE()
	MESSAGE(FATAL_ERROR "Invalid OVITO_QT_MAJOR_VERSION value: ${OVITO_QT_MAJOR_VERSION}. OVITO Supports only Qt5 and Qt6.")
ENDIF()

# This macro installs a third-party shared library or DLL in the OVITO program directory
# so that it can be distributed together with the program.
FUNCTION(OVITO_INSTALL_SHARED_LIB shared_lib destination_dir)
	IF(WIN32 OR OVITO_REDISTRIBUTABLE_PACKAGE OR OVITO_BUILD_PYTHON_PACKAGE)
		# Make sure the destination directory exists.
		SET(_abs_dest_dir "${Ovito_BINARY_DIR}/${OVITO_RELATIVE_3RDPARTY_LIBRARY_DIRECTORY}/${destination_dir}")
		FILE(MAKE_DIRECTORY "${_abs_dest_dir}")
		# Strip version number from shared lib filename.
		GET_FILENAME_COMPONENT(shared_lib_ext "${shared_lib}" EXT)
		STRING(REPLACE ${shared_lib_ext} "" shared_lib_new "${shared_lib}")
		FILE(GLOB lib_versions LIST_DIRECTORIES FALSE "${shared_lib}" "${shared_lib_new}.*${CMAKE_SHARED_LIBRARY_SUFFIX}" "${shared_lib_new}${CMAKE_SHARED_LIBRARY_SUFFIX}.*")
		IF(NOT lib_versions)
			MESSAGE(FATAL_ERROR "Did not find any library files that match the file path ${shared_lib} (globbing patterns: ${shared_lib_new}.*${CMAKE_SHARED_LIBRARY_SUFFIX}; ${shared_lib_new}${CMAKE_SHARED_LIBRARY_SUFFIX}.*)")
		ENDIF()
		# Find all variants of the shared library name, including symbolic links.
		UNSET(lib_files)
		FOREACH(lib_version ${lib_versions})
			WHILE(IS_SYMLINK ${lib_version})
				GET_FILENAME_COMPONENT(symlink_target "${lib_version}" REALPATH)
				GET_FILENAME_COMPONENT(symlink_target_name "${symlink_target}" NAME)
				GET_FILENAME_COMPONENT(lib_version_name "${lib_version}" NAME)
				IF(NOT lib_version_name STREQUAL symlink_target_name AND NOT OVITO_BUILD_PYTHON_PACKAGE)
					MESSAGE("Installing symlink ${lib_version_name} to ${symlink_target_name}")
					EXECUTE_PROCESS(COMMAND "${CMAKE_COMMAND}" -E create_symlink ${symlink_target_name} "${_abs_dest_dir}/${lib_version_name}")
					IF(NOT APPLE)
						INSTALL(FILES "${_abs_dest_dir}/${lib_version_name}" DESTINATION "${OVITO_RELATIVE_3RDPARTY_LIBRARY_DIRECTORY}/${destination_dir}/")
					ENDIF()
				ENDIF()
				SET(lib_version "${symlink_target}")
			ENDWHILE()
			IF(NOT IS_SYMLINK ${lib_version})
				LIST(APPEND lib_files "${lib_version}")
			ENDIF()
		ENDFOREACH()
		LIST(REMOVE_DUPLICATES lib_files)
		FOREACH(lib_file ${lib_files})
			IF(NOT APPLE)
				MESSAGE("Installing shared library ${lib_file}")
				EXECUTE_PROCESS(COMMAND "${CMAKE_COMMAND}" "-E" "copy_if_different" "${lib_file}" "${_abs_dest_dir}/" RESULT_VARIABLE _error_var)
				IF(_error_var)
					MESSAGE(FATAL_ERROR "Failed to copy shared library into build directory: ${lib_file}")
				ENDIF()
				IF(WIN32 OR NOT OVITO_BUILD_PYTHON_PACKAGE)
					INSTALL(FILES "${lib_file}" DESTINATION "${OVITO_RELATIVE_3RDPARTY_LIBRARY_DIRECTORY}/${destination_dir}/")
				ELSE()
					# Detect if this .so file is a linker script starting with the string "INPUT". 
					# The TBB libraries use this special GNU ld feature instead of regular symbolic links to create aliases of a shared library in the same directory.
					FILE(READ "${lib_file}" _SO_FILE_HEADER LIMIT 5 HEX)
					IF("${_SO_FILE_HEADER}" STREQUAL "494e505554") # 494e505554 = "INPUT"
						INSTALL(FILES "${lib_file}" DESTINATION "${OVITO_RELATIVE_3RDPARTY_LIBRARY_DIRECTORY}/${destination_dir}/")
					ELSE()
						# Use the objfump command to read out the SONAME of the shared library.
						GET_FILENAME_COMPONENT(lib_filename "${lib_file}" NAME)
						EXECUTE_PROCESS(COMMAND objdump -p "${lib_file}" COMMAND grep "SONAME" OUTPUT_VARIABLE _output_var RESULT_VARIABLE _error_var OUTPUT_STRIP_TRAILING_WHITESPACE)
						STRING(REPLACE "SONAME" "" lib_soname "${_output_var}")
						STRING(STRIP "${lib_soname}" lib_soname)
						IF(_error_var OR NOT lib_soname)
							MESSAGE(FATAL_ERROR "Failed to determine SONAME of shared library: ${lib_file}")
						ENDIF()
						# Use the SONAME as file name when installing the library in the OVITO directory.
						FILE(RENAME "${_abs_dest_dir}/${lib_filename}" "${_abs_dest_dir}/${lib_soname}")
						INSTALL(PROGRAMS "${_abs_dest_dir}/${lib_soname}" DESTINATION "${OVITO_RELATIVE_3RDPARTY_LIBRARY_DIRECTORY}/${destination_dir}/")
					ENDIF()
				ENDIF()
			ENDIF()
		ENDFOREACH()
		UNSET(lib_files)
	ENDIF()
ENDFUNCTION()

# Ship the required Qt libraries with the program package.
IF(UNIX AND NOT APPLE AND OVITO_REDISTRIBUTABLE_PACKAGE)

	# Install copies of the Qt libraries.
	FILE(MAKE_DIRECTORY "${OVITO_LIBRARY_DIRECTORY}/lib")
	FOREACH(component IN LISTS OVITO_REQUIRED_QT_COMPONENTS)
		GET_TARGET_PROPERTY(lib ${OVITO_QT_MAJOR_VERSION}::${component} LOCATION)
		GET_TARGET_PROPERTY(lib_soname ${OVITO_QT_MAJOR_VERSION}::${component} IMPORTED_SONAME_RELEASE)
		CONFIGURE_FILE("${lib}" "${OVITO_LIBRARY_DIRECTORY}" COPYONLY)
		GET_FILENAME_COMPONENT(lib_realname "${lib}" NAME)
		EXECUTE_PROCESS(COMMAND "${CMAKE_COMMAND}" -E create_symlink "${lib_realname}" "${OVITO_LIBRARY_DIRECTORY}/${lib_soname}")
		EXECUTE_PROCESS(COMMAND "${CMAKE_COMMAND}" -E create_symlink "../${lib_soname}" "${OVITO_LIBRARY_DIRECTORY}/lib/${lib_soname}")
		INSTALL(FILES "${lib}" DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/")
		INSTALL(FILES "${OVITO_LIBRARY_DIRECTORY}/${lib_soname}" DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/")
		INSTALL(FILES "${OVITO_LIBRARY_DIRECTORY}/lib/${lib_soname}" DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/lib/")
		GET_FILENAME_COMPONENT(QtBinaryPath ${lib} PATH)
	ENDFOREACH()

	# Install Qt plugins.
	INSTALL(DIRECTORY "${QtBinaryPath}/../plugins/platforms" DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/plugins_qt/")
	INSTALL(DIRECTORY "${QtBinaryPath}/../plugins/bearer" DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/plugins_qt/")
	INSTALL(DIRECTORY "${QtBinaryPath}/../plugins/imageformats" DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/plugins_qt/" PATTERN "libqsvg.so" EXCLUDE)
	INSTALL(DIRECTORY "${QtBinaryPath}/../plugins/iconengines" DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/plugins_qt/")
	INSTALL(DIRECTORY "${QtBinaryPath}/../plugins/xcbglintegrations" DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/plugins_qt/")
	# The XcbQpa library is required by the Qt5 Gui module.
	OVITO_INSTALL_SHARED_LIB("${QtBinaryPath}/libQt5XcbQpa.so" "./lib")

	# Distribute libxkbcommon.so with Ovito, which is a dependency of the Qt XCB plugin that might not be present on all systems.
	FIND_LIBRARY(OVITO_XKBCOMMON_DEP NAMES libxkbcommon.so.0 PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib64 NO_DEFAULT_PATH)
	IF(NOT OVITO_XKBCOMMON_DEP)
		MESSAGE(FATAL_ERROR "Could not find shared library libxkbcommon.so.0 in system path.")
	ENDIF()
	OVITO_INSTALL_SHARED_LIB("${OVITO_XKBCOMMON_DEP}" "./lib")
	UNSET(OVITO_XKBCOMMON_DEP CACHE)

	# Distribute libxkbcommon-x11.so with Ovito, which is a dependency of the Qt XCB plugin that might not be present on all systems.
	FIND_LIBRARY(OVITO_XKBCOMMONX11_DEP NAMES libxkbcommon-x11.so.0 PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib64 NO_DEFAULT_PATH)
	IF(NOT OVITO_XKBCOMMONX11_DEP)
		MESSAGE(FATAL_ERROR "Could not find shared library libxkbcommon-x11.so.0 in system path.")
	ENDIF()
	OVITO_INSTALL_SHARED_LIB("${OVITO_XKBCOMMONX11_DEP}" "./lib")
	UNSET(OVITO_XKBCOMMONX11_DEP CACHE)

	# Distribute libxcb-xinerama.so with Ovito, which is a dependency of the Qt XCB plugin that might not be present on all systems.
	FIND_LIBRARY(OVITO_XINERAMA_DEP NAMES libxcb-xinerama.so.0 PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib64 NO_DEFAULT_PATH)
	IF(NOT OVITO_XINERAMA_DEP)
		MESSAGE(FATAL_ERROR "Could not find shared library libxcb-xinerama.so.0 in system path.")
	ENDIF()
	OVITO_INSTALL_SHARED_LIB("${OVITO_XINERAMA_DEP}" "./lib")
	UNSET(OVITO_XINERAMA_DEP CACHE)
	
ELSEIF(WIN32 AND NOT OVITO_BUILD_PYTHON_PACKAGE AND NOT OVITO_BUILD_CONDA)

	# On Windows, the third-party library DLLs need to be installed in the OVITO directory.
	# Gather Qt dynamic link libraries.
	FOREACH(component IN LISTS OVITO_REQUIRED_QT_COMPONENTS)
		GET_TARGET_PROPERTY(dll ${OVITO_QT_MAJOR_VERSION}::${component} LOCATION_${CMAKE_BUILD_TYPE})
		IF(NOT TARGET ${OVITO_QT_MAJOR_VERSION}::${component} OR NOT dll)
			MESSAGE(FATAL_ERROR "Target does not exist or has no LOCATION property: ${OVITO_QT_MAJOR_VERSION}::${component}")
		ENDIF()
		OVITO_INSTALL_SHARED_LIB("${dll}" ".")
		IF(${component} MATCHES "Core")
			GET_FILENAME_COMPONENT(QtBinaryPath ${dll} PATH)
			IF(dll MATCHES "Cored.dll$")
				SET(_using_qt_debug_build TRUE)
			ENDIF()
		ENDIF()
	ENDFOREACH()

	# Install Qt plugins.
	IF(_using_qt_debug_build)
		OVITO_INSTALL_SHARED_LIB("${QtBinaryPath}/../plugins/platforms/qwindowsd.dll" "plugins/platforms/")
		OVITO_INSTALL_SHARED_LIB("${QtBinaryPath}/../plugins/imageformats/qjpegd.dll" "plugins/imageformats/")
		OVITO_INSTALL_SHARED_LIB("${QtBinaryPath}/../plugins/imageformats/qgifd.dll" "plugins/imageformats/")
		OVITO_INSTALL_SHARED_LIB("${QtBinaryPath}/../plugins/iconengines/qsvgicond.dll" "plugins/iconengines/")
		OVITO_INSTALL_SHARED_LIB("${QtBinaryPath}/../plugins/styles/qwindowsvistastyled.dll" "plugins/styles/")
	ELSE()
		OVITO_INSTALL_SHARED_LIB("${QtBinaryPath}/../plugins/platforms/qwindows.dll" "plugins/platforms/")
		OVITO_INSTALL_SHARED_LIB("${QtBinaryPath}/../plugins/imageformats/qjpeg.dll" "plugins/imageformats/")
		OVITO_INSTALL_SHARED_LIB("${QtBinaryPath}/../plugins/imageformats/qgif.dll" "plugins/imageformats/")
		OVITO_INSTALL_SHARED_LIB("${QtBinaryPath}/../plugins/iconengines/qsvgicon.dll" "plugins/iconengines/")
		OVITO_INSTALL_SHARED_LIB("${QtBinaryPath}/../plugins/styles/qwindowsvistastyle.dll" "plugins/styles/")
	ENDIF()

ENDIF()
