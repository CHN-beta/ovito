import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.ovito

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("External file")
	helpTopicId: "manual:scene_objects.file_source"

	ColumnLayout {
		anchors.fill: parent

		GroupBox {
			title: qsTr("Data source")
			Layout.fillWidth: true

			GridLayout {
				anchors.fill: parent
				columns: 2

				Label { text: qsTr("Current file:") }
				TextField { 
					text: propertyEditor.editObject ? propertyEditor.editObject.currentFileName : ""
					readOnly: true
					Layout.fillWidth: true
				}

				Label { text: qsTr("Directory:") }
				TextField {
					text: propertyEditor.editObject ? propertyEditor.editObject.currentDirectoryPath : ""
					readOnly: true
					Layout.fillWidth: true
				}
			}
		}
/*
		GroupBox {
			title: qsTr("File sequence")
			Layout.fillWidth: true

			GridLayout {
				anchors.fill: parent
				columns: 2

				Label { text: qsTr("Search pattern:") }
				TextField { 
					readOnly: true
					Layout.fillWidth: true
				}

				Ui.BooleanCheckBoxParameter {
					propertyField: "autoGenerateFilePattern"
					text: "auto-generate"
				}
				Label { text: "..." }
			}
		}

		GroupBox {
			title: qsTr("Trajectory")
			Layout.fillWidth: true

			GridLayout {
				anchors.fill: parent
				columns: 2

				Label { text: qsTr("Current frame:") }
				ComboBox { 
					Layout.fillWidth: true
				}

				Label { 
					text: qsTr("Showing frame...") 
					Layout.row: 1
					Layout.column: 1
					Layout.fillWidth: true
				}
			}
		}
*/

		Text { text: "Status:" }
		Ui.ObjectStatusWidget {
			Layout.fillWidth: true
			Layout.columnSpan: 2
		}
	}
}