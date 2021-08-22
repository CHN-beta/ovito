import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import org.ovito
import "ui" as Ui

ApplicationWindow {
	id: rootWindow
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

	Ui.SystemReportDialog {
		id: systemReportDialog
	}

	header: Ui.ToolBar {
		id: toolBar
		objectName: "toolBar"
		Layout.fillWidth: true
	}

	SplitView {
		anchors.fill: parent
		orientation: Qt.Horizontal

		ColumnLayout {
			spacing: 0
			SplitView.fillWidth: true
			SplitView.fillHeight: true
			SplitView.minimumWidth: 50

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
			SplitView.preferredWidth: 320
			SplitView.minimumWidth: 100
			SplitView.fillWidth: false
			SplitView.fillHeight: true
		}
	}
}