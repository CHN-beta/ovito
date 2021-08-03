import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

// Items in this module are defined in C++ code.
import org.ovito 1.0

import "ui" as Ui

ApplicationWindow {
    visible: true
	width: 800
	height: 600
	color: "#444"

	Ui.ErrorDialog {
		id: errorDialog
	}

	Ui.AboutDialog {
		id: aboutDialog
	}

	RowLayout {
		anchors.fill: parent
		spacing: 0

		ColumnLayout {
			spacing: 0

			Ui.ToolBar {
				id: toolBar
				objectName: "toolBar"
				Layout.fillWidth: true
			}

			MainWindow {
				id: mainWindow
				Layout.fillWidth: true
				Layout.fillHeight: true

				ViewportsPanel {
					id: viewportsPanel
					anchors.fill: parent
				}

				Ui.StatusBar {
					id: statusBar
					anchors.left: parent.left
					anchors.leftMargin: 4.0
					anchors.bottom: parent.bottom
					anchors.bottomMargin: 4.0
					objectName: "statusBar"
				}

				onError: function (message) {
					errorDialog.text = message
					errorDialog.open()
				}
			}
			Pane {
				Layout.fillWidth: true
				padding: 0
				hoverEnabled: true
				
				ColumnLayout {
					anchors.fill: parent

					Ui.AnimationBar {
						id: animationBar
						objectName: "animationBar"
					}
				}
			}			
		}

		Ui.CommandPanel {
			id: commandPanel
			Layout.preferredWidth: 320
			Layout.fillHeight: true
		}
	}
}