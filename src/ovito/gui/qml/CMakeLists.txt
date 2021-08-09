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
OVITO_STANDARD_PLUGIN(Gui
	SOURCES
		app/WasmApplication.cpp
		dataset/WasmDataSetContainer.cpp
		dataset/WasmFileManager.cpp
		mainwin/MainWindow.cpp
		mainwin/ViewportsPanel.cpp
		properties/ParameterUI.cpp
		properties/ModifierDelegateParameterUI.cpp
		viewport/QuickViewportWindow.cpp
		resources/gui.qrc
	PLUGIN_DEPENDENCIES
		OpenGLRenderer
		GuiBase
	LIB_DEPENDENCIES
		${OVITO_QT_MAJOR_VERSION}::Qml 
		${OVITO_QT_MAJOR_VERSION}::Quick 
		${OVITO_QT_MAJOR_VERSION}::QuickControls2 
		${OVITO_QT_MAJOR_VERSION}::QuickTemplates2
	PRECOMPILED_HEADERS 
		GUI.h
)

IF(EMSCRIPTEN)
	FILE(GLOB _controls_basic_qml_files LIST_DIRECTORIES FALSE "${QT_TOOLCHAIN_RELOCATABLE_INSTALL_PREFIX}/qml/QtQuick/Controls/Basic/*.qml")
	QT_ADD_RESOURCES(Gui PackedQmlDirFiles
		PREFIX "/qt-project.org/imports"
		BASE "${QT_TOOLCHAIN_RELOCATABLE_INSTALL_PREFIX}/qml"
		FILES 
			"${QT_TOOLCHAIN_RELOCATABLE_INSTALL_PREFIX}/qml/QtQuick/Controls/qmldir"
			"${QT_TOOLCHAIN_RELOCATABLE_INSTALL_PREFIX}/qml/QtQuick/Controls/Basic/qmldir"
			"${QT_TOOLCHAIN_RELOCATABLE_INSTALL_PREFIX}/qml/QtQuick/Controls/Basic/impl/qmldir"
			"${QT_TOOLCHAIN_RELOCATABLE_INSTALL_PREFIX}/qml/QtQuick/Templates/qmldir"
			"${QT_TOOLCHAIN_RELOCATABLE_INSTALL_PREFIX}/qml/QtQuick/Window/qmldir"
			${_controls_basic_qml_files}
		)

	FILE(GLOB _controls_basic_image_files LIST_DIRECTORIES FALSE "C:/Qt/6.2.0/Src/qtquickcontrols2/src/quickcontrols2/basic/images/*.png")
	QT_ADD_RESOURCES(Gui PackedQmlBasicImageFiles
		PREFIX "/qt-project.org/imports/QtQuick/Controls/Basic/"
		BASE "C:/Qt/6.2.0/Src/qtquickcontrols2/src/quickcontrols2/basic"
		FILES ${_controls_basic_image_files})

	QT_ADD_RESOURCES(Gui PackedQmlFusionImageFiles
		PREFIX "/qt-project.org/imports/QtQuick/Controls/Fusion/"
		BASE "C:/Qt/6.2.0/Src/qtquickcontrols2/src/quickcontrols2/fusion"
		FILES "C:/Qt/6.2.0/Src/qtquickcontrols2/src/quickcontrols2/fusion/images/arrow.png")
ENDIF()

IF(NOT EMSCRIPTEN)
	TARGET_LINK_LIBRARIES(Gui PRIVATE ${OVITO_QT_MAJOR_VERSION}::Widgets)
ENDIF()
