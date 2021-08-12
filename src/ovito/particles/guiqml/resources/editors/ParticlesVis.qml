import QtQuick
import QtQuick.Controls
import QtQuick.Layouts 6.0

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("Particles display")
	
	GridLayout {
		anchors.fill: parent
		columns: 2

		Label { text: qsTr("Shape:"); }
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

		Label { text: qsTr("Default radius:") }
		Ui.FloatParameter { 
			propertyField: "defaultParticleRadius"
			Layout.fillWidth: true 
		}
	}
}