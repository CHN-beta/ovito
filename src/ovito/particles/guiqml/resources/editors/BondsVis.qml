import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("Bonds display")
	helpTopicId: "manual:visual_elements.bonds"
	
	GridLayout {
		anchors.fill: parent
		columns: 2

		Label { text: qsTr("Shading mode:"); }
		Ui.VariantComboBoxParameter {
			propertyField: "shadingMode"
			Layout.fillWidth: true
			model: ListModel {
				id: model
				ListElement { text: qsTr("Normal") }
				ListElement { text: qsTr("Flat") }
			}
		}

		Label { text: qsTr("Default bond width:") }
		Ui.FloatParameter { 
			propertyField: "bondWidth"
			Layout.fillWidth: true 
		}

		Label { text: qsTr("Default bond color:") }
		Ui.ColorParameter {
			propertyField: "bondColor"
			Layout.fillWidth: true 
		}

		Ui.BooleanCheckBoxParameter { 
			propertyField: "useParticleColors"
			Layout.columnSpan: 2
		}
	}
}