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
			Layout.column: 1
		}

		Ui.ParameterLabel { propertyField: "cellLineWidth" }
		Ui.FloatParameter {
			propertyField: "cellLineWidth"
			Layout.fillWidth: true
		}

		Ui.ParameterLabel { propertyField: "cellColor" }
		Ui.ColorParameter {
			propertyField: "cellColor"
			Layout.fillWidth: true
		}
	}
}
