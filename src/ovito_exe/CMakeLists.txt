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

# Include the resource in the Win32 executable.
IF(WIN32)
	SET(WINDOWS_RESOURCES resources/ovito.rc)
ENDIF(WIN32)

IF(CMAKE_BUILD_TYPE STREQUAL "Release")
	# This will create a GUI application on Windows platform.
	SET(OVITO_WIN32_EXECUTABLE "WIN32")
ELSE()
	# This will create a console application on Windows platform.
	SET(OVITO_WIN32_EXECUTABLE "")
ENDIF()

# Builds the main executable of the application
IF(NOT OVITO_QML_GUI)
	ADD_EXECUTABLE(Ovito ${OVITO_WIN32_EXECUTABLE} Main.cpp ${WINDOWS_RESOURCES})
ELSE()
	ADD_EXECUTABLE(Ovito ${OVITO_WIN32_EXECUTABLE} MainQml.cpp ${WINDOWS_RESOURCES})
ENDIF()

# The executable depends on the core module, of course.
TARGET_LINK_LIBRARIES(Ovito PRIVATE Core)

# The executable depends on the gui module.
IF(NOT OVITO_QML_GUI)
	TARGET_LINK_LIBRARIES(Ovito PRIVATE Gui)
	TARGET_LINK_LIBRARIES(Ovito PRIVATE ${OVITO_QT_MAJOR_VERSION}::Core ${OVITO_QT_MAJOR_VERSION}::Gui)
ELSE()
	TARGET_LINK_LIBRARIES(Ovito PRIVATE GuiWeb)
	TARGET_LINK_LIBRARIES(Ovito PRIVATE ${OVITO_QT_MAJOR_VERSION}::Core ${OVITO_QT_MAJOR_VERSION}::Gui ${OVITO_QT_MAJOR_VERSION}::Qml ${OVITO_QT_MAJOR_VERSION}::Quick ${OVITO_QT_MAJOR_VERSION}::QuickControls2 ${OVITO_QT_MAJOR_VERSION}::QuickTemplates2 ${OVITO_QT_MAJOR_VERSION}::Svg)
ENDIF()

# Put the executable into the right directory.
IF(APPLE AND NOT OVITO_BUILD_CONDA)
	SET_TARGET_PROPERTIES(Ovito PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${Ovito_BINARY_DIR}")
	INSTALL(TARGETS Ovito DESTINATION ".")
ELSEIF(UNIX)
	SET_TARGET_PROPERTIES(Ovito PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${Ovito_BINARY_DIR}/bin")
	INSTALL(TARGETS Ovito DESTINATION "bin/")
ELSEIF(WIN32)
	IF(NOT OVITO_BUILD_CONDA)
		SET_TARGET_PROPERTIES(Ovito PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${Ovito_BINARY_DIR}")
		INSTALL(TARGETS Ovito DESTINATION ".")
	ELSE()
		SET_TARGET_PROPERTIES(Ovito PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OVITO_BINARY_DIRECTORY}")
		INSTALL(TARGETS Ovito DESTINATION "${OVITO_RELATIVE_BINARY_DIRECTORY}")
	ENDIF()
ENDIF()

# Copy in HTML/JS launch files.
IF(EMSCRIPTEN)
	CONFIGURE_FILE("${_qt5Core_install_prefix}/plugins/platforms/qtloader.js" "${OVITO_BINARY_DIRECTORY}/qtloader.js" COPYONLY)
	CONFIGURE_FILE("wasm/wasm_shell.html" "${OVITO_BINARY_DIRECTORY}/wasm_shell.html")
	CONFIGURE_FILE("wasm/index.html" "${OVITO_BINARY_DIRECTORY}/index.html" COPYONLY)
	CONFIGURE_FILE("../ovito/gui/base/resources/mainwin/window_icon_256.png" "${OVITO_BINARY_DIRECTORY}/ovito_logo.png" COPYONLY)
ENDIF()

# Set name of executable.
SET_TARGET_PROPERTIES(Ovito PROPERTIES OUTPUT_NAME "ovito")

# Under Linux, Qt's Xcb platform plugin will dynamically load the DBus Qt module.
# To avoid the dynamic linker pulling the wrong versions of the Qt module from the
# system path, we make the executable preload the module on application startup.
IF(UNIX AND NOT APPLE AND OVITO_REDISTRIBUTABLE_PACKAGE)
	TARGET_LINK_LIBRARIES(Ovito PRIVATE ${OVITO_QT_MAJOR_VERSION}::DBus)
ENDIF()

IF(EMSCRIPTEN)
	# Link to missing Qt libraries (the order does matter).
	TARGET_LINK_LIBRARIES(Ovito PRIVATE
		"${_qt5Core_install_prefix}/qml/QtQuick.2/libqtquick2plugin.a"      # QtQuick2Plugin
		"${_qt5Core_install_prefix}/qml/QtQuick/Window.2/libwindowplugin.a" # QtQuick2WindowPlugin
		"${_qt5Core_install_prefix}/qml/QtQuick/Layouts/libqquicklayoutsplugin.a" # QtQuick2LayoutsPlugin
		"${_qt5Core_install_prefix}/qml/QtQuick/Controls.2/libqtquickcontrols2plugin.a" # QtQuickControls2Plugin
		"${_qt5Core_install_prefix}/qml/QtQuick/Templates.2/libqtquicktemplates2plugin.a" # QtQuickTemplates2Plugin
		"${_qt5Core_install_prefix}/plugins/iconengines/libqsvgicon.a"		# SVG Icon engine plugin
		"${_qt5Core_install_prefix}/plugins/imageformats/libqsvg.a"			# SVG image format plugin
		"${_qt5Core_install_prefix}/plugins/platforms/libqwasm.a"       	# No longer needed in Qt 5.14
		"${_qt5Core_install_prefix}/lib/libQt5QuickControls2.a" 			# No longer needed in Qt 5.14
		"${_qt5Core_install_prefix}/lib/libQt5QuickTemplates2.a" 			# No longer needed in Qt 5.14
		"${_qt5Core_install_prefix}/lib/libQt5Quick.a"						# No longer needed in Qt 5.14
		"${_qt5Core_install_prefix}/lib/libQt5EventDispatcherSupport.a"		# No longer needed in Qt 5.14
		"${_qt5Core_install_prefix}/lib/libQt5FontDatabaseSupport.a"		# No longer needed in Qt 5.14
		"${_qt5Core_install_prefix}/lib/libqtfreetype.a"					# No longer needed in Qt 5.14
		"${_qt5Core_install_prefix}/lib/libQt5Svg.a"						# No longer needed in Qt 5.14
	)
ENDIF()

SET_TARGET_PROPERTIES(Ovito PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
IF(APPLE)
	IF(NOT OVITO_BUILD_CONDA)
	    SET_TARGET_PROPERTIES(Ovito PROPERTIES MACOSX_BUNDLE TRUE)
		SET_TARGET_PROPERTIES(Ovito PROPERTIES INSTALL_RPATH "@executable_path/;@executable_path/../Frameworks/;@executable_path/../PlugIns/")
	ELSE()
		SET_TARGET_PROPERTIES(Ovito PROPERTIES MACOSX_BUNDLE FALSE)
		SET_TARGET_PROPERTIES(Ovito PROPERTIES INSTALL_RPATH "@loader_path/;@loader_path/../${OVITO_RELATIVE_PLUGINS_DIRECTORY}/")
	ENDIF()
ELSEIF(UNIX)
	SET_TARGET_PROPERTIES(Ovito PROPERTIES INSTALL_RPATH "$ORIGIN/../${OVITO_RELATIVE_LIBRARY_DIRECTORY}/:$ORIGIN/../${OVITO_RELATIVE_PLUGINS_DIRECTORY}/")
ENDIF()

# Build all plugins first before building the main executable.
FOREACH(plugin ${OVITO_PLUGIN_LIST})
	IF(BUILD_SHARED_LIBS)
		ADD_DEPENDENCIES(Ovito ${plugin})
	ELSE()
		# When generating a static executable that contains all plugins, make sure
		# the linker doesn't remove apparently unreferenced symbols. They are needed to
		# have the plugin classes register themselves.
		IF(UNIX)
			TARGET_LINK_LIBRARIES(Ovito PRIVATE -Wl,--whole-archive ${plugin} -Wl,--no-whole-archive)
		ELSE()
			TARGET_LINK_LIBRARIES(Ovito PRIVATE ${plugin})
			IF(MSVC)
				SET_PROPERTY(TARGET Ovito APPEND_STRING PROPERTY LINK_FLAGS " /WHOLEARCHIVE:${plugin}")
			ENDIF()
		ENDIF()
	ENDIF()
ENDFOREACH()

# Install fonts required by 'minimal' platform plugin on Linux
IF(UNIX AND NOT APPLE AND NOT OVITO_BUILD_CONDA)
    INSTALL(DIRECTORY "resources/fonts" DESTINATION "${OVITO_RELATIVE_SHARE_DIRECTORY}/")
ENDIF()

IF(UNIX AND NOT APPLE AND OVITO_REDISTRIBUTABLE_PACKAGE)

	# Create a qt.conf file.
	# This is required to let Qt search for its plugin modules in the program directory.
	INSTALL(CODE "
	    FILE(WRITE \"\${CMAKE_INSTALL_PREFIX}/${OVITO_RELATIVE_BINARY_DIRECTORY}/qt.conf\" \"[Paths]\\nPlugins = ../${OVITO_RELATIVE_LIBRARY_DIRECTORY}/plugins_qt/\")
	    ")

ELSEIF(WIN32 AND NOT OVITO_BUILD_CONDA)

	# Create a qt.conf file.
	# This is required to let Qt search for its plugin modules in the program directory.
	FILE(WRITE "${OVITO_BINARY_DIRECTORY}/qt.conf" "[Paths]\nPlugins = plugins/")
	INSTALL(CODE "
	    FILE(WRITE \"\${CMAKE_INSTALL_PREFIX}/${OVITO_RELATIVE_BINARY_DIRECTORY}/qt.conf\" \"[Paths]\\nPlugins = plugins/\")
		")

ELSEIF(APPLE AND NOT OVITO_BUILD_CONDA)

	# Install the Info.plist file in the app bundle.
	CONFIGURE_FILE("resources/Info.plist" "${Ovito_BINARY_DIR}/${MACOSX_BUNDLE_NAME}.app/Contents/Info.plist")
	SET_TARGET_PROPERTIES(Ovito PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${Ovito_BINARY_DIR}/${MACOSX_BUNDLE_NAME}.app/Contents/Info.plist")

	# Copy the application icon into the resource directory.
	INSTALL(FILES "resources/ovito.icns" DESTINATION "${OVITO_RELATIVE_SHARE_DIRECTORY}")

	# Fix Mac app bundle.
	INCLUDE("${Ovito_SOURCE_DIR}/cmake/FixupMacBundle.cmake")

ENDIF()

# Special steps needed to build OVITO with Python scripting support.
IF(OVITO_BUILD_PLUGIN_PYSCRIPT AND NOT OVITO_BUILD_BASIC)

	# Make sure the Python library gets loaded at program startup. We need to do it
	# this way, because the OVITO scripting plugins do not explicitly link the
	# Python interpreter library. They rely on it already being loaded into the 
	# process memory (as is the case if the OVITO Python modules are imported by an external interpreter).
	FIND_PACKAGE(Python3 REQUIRED COMPONENTS Interpreter Development)
	TARGET_LINK_LIBRARIES(Ovito PRIVATE Python3::Python)
	TARGET_COMPILE_DEFINITIONS(Ovito PRIVATE "OVITO_BUILD_PLUGIN_PYSCRIPT=1")
	TARGET_INCLUDE_DIRECTORIES(Ovito PRIVATE "${CMAKE_SOURCE_DIR}/proprietary")

	IF(OVITO_REDISTRIBUTABLE_PACKAGE AND UNIX AND NOT APPLE)
		# Uninstall Python packages from the embedded interpreter that should not be part of the official distribution.
		INSTALL(CODE "${OVITO_UNINSTALL_UNUSED_PYTHON_MODULES_CODE}")
	ENDIF()
ENDIF()