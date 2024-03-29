import QtQuick
import QtQuick.Controls

ToolBar {
	leftPadding: 8
	hoverEnabled: true

	Flow {
		id: flow
		width: parent.width - infoLabel.implicitWidth

		Row {
			id: fileRow
			ToolButton {
				id: importLocalFileButton
				icon.name: "file_import"
				ToolTip.text: qsTr("Import local file")
				ToolTip.visible: hovered
				ToolTip.delay: 500
				onClicked: mainWindow.importDataFile()
			}
			ToolSeparator {
				contentItem.visible: fileRow.y == editRow.y
			}
		}

		Row {
			id: editRow
			ToolButton {
				id: undoButton
				icon.name: "edit_undo"
				ToolTip.text: qsTr("Undo action: ") + (mainWindow.datasetContainer.currentSet ? mainWindow.datasetContainer.currentSet.undoStack.undoText : "")
				ToolTip.visible: hovered
				ToolTip.delay: 500
				display: AbstractButton.IconOnly
				enabled: mainWindow.datasetContainer.currentSet ? mainWindow.datasetContainer.currentSet.undoStack.canUndo : false
				onClicked: mainWindow.datasetContainer.currentSet.undoStack.undo()
			}
			ToolButton {
				id: redoButton
				icon.name: "edit_redo"
				ToolTip.text: qsTr("Redo action: ") + (mainWindow.datasetContainer.currentSet ? mainWindow.datasetContainer.currentSet.undoStack.redoText : "")
				ToolTip.visible: hovered
				ToolTip.delay: 500
				display: AbstractButton.IconOnly
				enabled: mainWindow.datasetContainer.currentSet ? mainWindow.datasetContainer.currentSet.undoStack.canRedo : false
				onClicked: mainWindow.datasetContainer.currentSet.undoStack.redo()
			}
			ToolSeparator {
				contentItem.visible: editRow.y === viewportRow.y
			}
		}

		Row {
			id: viewportRow
			ToolButton {
				id: zoomSceneExtentsButton
				icon.source: "qrc:/guibase/actions/viewport/zoom_scene_extents.bw.svg"
				ToolTip.text: qsTr("Zoom to scene extents")
				ToolTip.visible: hovered
				ToolTip.delay: 500
				display: AbstractButton.IconOnly
				enabled: viewportsPanel.viewportConfiguration && viewportsPanel.viewportConfiguration.activeViewport
				onClicked: viewportsPanel.viewportConfiguration.activeViewport.zoomToSceneExtents()
			}
			ToolButton {
				id: multiViewportButton
				checkable: true
				checked: viewportsPanel.viewportConfiguration && viewportsPanel.viewportConfiguration.maximizedViewport == null
				enabled: viewportsPanel.viewportConfiguration
				icon.name: "viewport_multi_viewports"
				ToolTip.text: qsTr("Show multiple viewports")
				ToolTip.visible: hovered
				ToolTip.delay: 500
				display: AbstractButton.IconOnly
				onToggled: viewportsPanel.viewportConfiguration.maximizedViewport = checked ? null : viewportsPanel.viewportConfiguration.activeViewport
			}
			ToolSeparator {
				contentItem.visible: viewportRow.y === aboutRow.y
			}
		}

		Row {
			id: aboutRow
			ToolButton {
				id: helpButton
				icon.name: "help_user_manual"
				ToolTip.text: qsTr("User Manual")
				ToolTip.visible: hovered
				ToolTip.delay: 500
				display: AbstractButton.IconOnly
				onClicked: mainWindow.openHelpTopic("")
			}
			/*
			ToolButton {
				id: aboutButton
				icon.name: "application_about"
				ToolTip.text: qsTr("About OVITO")
				ToolTip.visible: hovered
				ToolTip.delay: 500
				display: AbstractButton.IconOnly
				onClicked: aboutDialog.open()
			}
			*/
		}
	}

	ItemDelegate {
		id: infoLabel
		anchors.right: parent.right
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		text: "OVITO Web (technology demo)"
		padding: 4

		icon.source: "qrc:/guibase/mainwin/window_icon_16.png"
		icon.color: "transparent"

		onClicked: aboutDialog.open()
	}
}
