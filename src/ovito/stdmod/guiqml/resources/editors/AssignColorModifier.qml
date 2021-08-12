import QtQuick
import QtQuick.Controls
import QtQuick.Layouts 6.0

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("Assign color")

	GridLayout {
		anchors.fill: parent
		columns: 2

		Label { text: qsTr("Operate on:") }
		Ui.ModifierDelegateParameter {
			delegateType: "AssignColorModifierDelegate"
			Layout.fillWidth: true
		}

		Label { text: qsTr("Color:") }
		Ui.ColorParameter {
			propertyField: "colorController"
			Layout.fillWidth: true
		}

		Ui.BooleanCheckBoxParameter { 
			propertyField: "keepSelection"
			Layout.columnSpan: 2
		}
	}
}
