import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("Bonds display")
	
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