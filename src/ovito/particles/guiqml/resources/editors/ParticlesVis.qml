import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("Particle display")
	helpTopicId: "manual:visual_elements.particles"
	
	GridLayout {
		anchors.fill: parent
		columns: 2

		Ui.ParameterLabel { propertyField: "particleShape" }
		Ui.VariantComboBoxParameter {
			propertyField: "particleShape"
			Layout.fillWidth: true
			model: ListModel {
				id: model
				ListElement { text: qsTr("Sphere/Ellipsoid") }
				ListElement { text: qsTr("Cube/Box") }
				ListElement { text: qsTr("Circle") }
				ListElement { text: qsTr("Square") }
				ListElement { text: qsTr("Cylinder") }
				ListElement { text: qsTr("Spherocylinder") }
			}
		}

		Ui.ParameterLabel { propertyField: "defaultParticleRadius" }
		Ui.FloatParameter { 
			propertyField: "defaultParticleRadius"
			Layout.fillWidth: true 
		}

		Ui.ParameterLabel { propertyField: "radiusScaleFactor" }
		Ui.FloatParameter { 
			propertyField: "radiusScaleFactor"
			Layout.fillWidth: true 
		}
	}
}