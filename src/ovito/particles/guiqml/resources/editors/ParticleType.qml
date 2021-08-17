import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.ovito

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("Particle type")
	
	ColumnLayout {
		anchors.fill: parent

		GroupBox {
			title: qsTr("Particle type")
			Layout.fillWidth: true

			GridLayout {
				anchors.fill: parent
				columns: 2

				Label { text: "Numeric ID:" }
				Label { 
					id: numeriIdLabel
					ParameterUI on text {
						propertyName: "numericId"
						editObject: propertyEditor.editObject
					}
				}

				Ui.ParameterLabel { propertyField: "name" }
				Ui.StringParameter {
					propertyField: "name"
					Layout.fillWidth: true
					placeholderText: "‹Type " + numeriIdLabel.text + "›"
				}
			}
		}

		GroupBox {
			title: qsTr("Appearance")
			Layout.fillWidth: true

			GridLayout {
				anchors.fill: parent
				columns: 2

				Ui.ParameterLabel { propertyField: "color" }
				Ui.ColorParameter {
					propertyField: "color"
					Layout.fillWidth: true
				}

				Ui.ParameterLabel { propertyField: "radius" }
				Ui.FloatParameter {
					propertyField: "radius"
					standardValue: 0.0
					placeholderText: qsTr("‹unspecified›")
					Layout.fillWidth: true
				}
			}
		}

		GroupBox {
			title: qsTr("Physical properties")
			Layout.fillWidth: true

			GridLayout {
				anchors.fill: parent
				columns: 2

				Ui.ParameterLabel { propertyField: "mass" }
				Ui.FloatParameter {
					propertyField: "mass"
					standardValue: 0.0
					placeholderText: qsTr("‹unspecified›")
					Layout.fillWidth: true
				}

				Ui.ParameterLabel { propertyField: "vdwRadius" }
				Ui.FloatParameter {
					propertyField: "vdwRadius"
					standardValue: 0.0
					placeholderText: qsTr("‹unspecified›")
					Layout.fillWidth: true
				}
			}
		}
	}
}