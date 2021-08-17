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

# Define plugin module.
OVITO_STANDARD_PLUGIN(GuiBase
	SOURCES
		actions/ActionManager.cpp
		actions/ViewportModeAction.cpp
		actions/ViewportActions.cpp
		actions/AnimationActions.cpp
		mainwin/MainWindowInterface.cpp
		mainwin/OverlayListModel.cpp
		mainwin/OverlayListItem.cpp
		mainwin/OverlayTypesModel.cpp
		mainwin/PipelineListModel.cpp
		mainwin/PipelineListItem.cpp
		mainwin/ModifierListModel.cpp
		viewport/ViewportInputManager.cpp
		viewport/ViewportInputMode.cpp
		viewport/NavigationModes.cpp
		viewport/SelectionMode.cpp
		resources/guibase.qrc
	PRECOMPILED_HEADERS
		GUIBase.h
)

# The GUI base module depends on the QAction class, which used to be defined only in the QtWidgets module in Qt5.
# Starting with Qt6, teh QAction class is part of the QtGui module.
IF(OVITO_QT_MAJOR_VERSION STREQUAL "Qt5")
	TARGET_LINK_LIBRARIES(GuiBase PUBLIC ${OVITO_QT_MAJOR_VERSION}::Widgets)
ENDIF()

IF(NOT EMSCRIPTEN)
	# Compute the relative path where the documentation files get installed relative to the executable. 
	FILE(RELATIVE_PATH OVITO_DOCUMENTATION_PATH "${OVITO_BINARY_DIRECTORY}" "${OVITO_SHARE_DIRECTORY}/doc/manual/html/")
	TARGET_COMPILE_DEFINITIONS(GuiBase PRIVATE "OVITO_DOCUMENTATION_PATH=\"${OVITO_DOCUMENTATION_PATH}\"")
ELSE()
	# Download the documentation inventory file before creating the resource file.
	FILE(DOWNLOAD https://docs.ovito.org/objects.txt "${CMAKE_CURRENT_BINARY_DIR}/objects.txt")
	# Link the manual inventory file into the executable.
	QT_ADD_RESOURCES(GuiBase PackagedManual
		PREFIX "/doc/manual/"
		BASE "${CMAKE_CURRENT_BINARY_DIR}"
		FILES "${CMAKE_CURRENT_BINARY_DIR}/objects.txt")
ENDIF()

# Compute the relative path where the Python extension scripts get installed relative to the executable. 
FILE(RELATIVE_PATH OVITO_SCRIPT_EXTENSIONS_RELATIVE_PATH "${OVITO_BINARY_DIRECTORY}" "${OVITO_SHARE_DIRECTORY}/scripts/")
TARGET_COMPILE_DEFINITIONS(GuiBase PRIVATE "OVITO_SCRIPT_EXTENSIONS_RELATIVE_PATH=\"${OVITO_SCRIPT_EXTENSIONS_RELATIVE_PATH}\"")

# Propagate list of plugins to parent scope.
SET(OVITO_PLUGIN_LIST ${OVITO_PLUGIN_LIST} PARENT_SCOPE)
