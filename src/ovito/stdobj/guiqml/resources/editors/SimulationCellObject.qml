import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("Simulation cell")
	helpTopicId: "manual:scene_objects.simulation_cell"
	
	ColumnLayout {
		anchors.fill: parent

		GroupBox {
			title: qsTr("Dimensionality")
			Layout.fillWidth: true
			RowLayout {
				anchors.fill: parent
				Ui.BooleanRadioButtonParameter {
					propertyField: "is2D"
					inverted: false
					text: qsTr("2D")
				}
				Ui.BooleanRadioButtonParameter {
					propertyField: "is2D"
					inverted: true
					text: qsTr("3D")
				}
			}
		}

		GroupBox {
			title: qsTr("Periodic boundary conditions")
			Layout.fillWidth: true
			RowLayout {
				anchors.fill: parent
				Ui.BooleanCheckBoxParameter {
					propertyField: "pbcX"
					text: qsTr("X")
				}
				Ui.BooleanCheckBoxParameter {
					propertyField: "pbcY"
					text: qsTr("Y")
				}
				Ui.BooleanCheckBoxParameter {
					propertyField: "pbcZ"
					text: qsTr("Z")
				}
			}
		}
	}
}