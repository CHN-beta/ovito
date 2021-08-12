import QtQuick
import QtQuick.Controls
import QtQuick.Layouts 6.0

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("Simulation cell")

	GridLayout {
		anchors.fill: parent
		columns: 2

		Ui.BooleanCheckBoxParameter { 
			propertyField: "renderCellEnabled"
			Layout.columnSpan: 2
		}

		Label { text: qsTr("Line width:") }
		Ui.FloatParameter {
			propertyField: "cellLineWidth"
			Layout.fillWidth: true
		}

		Label { text: qsTr("Line color:") }
		Ui.ColorParameter {
			propertyField: "cellColor"
			Layout.fillWidth: true
		}
	}
}
