import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("Centrosymmetry parameter")
	helpTopicId: "manual:particles.modifiers.centrosymmetry"

	GridLayout {
		anchors.fill: parent
		columns: 2

		Label { text: qsTr("Number of neighbors:") }
		Ui.IntegerParameter { 
			propertyField: "numNeighbors"
			Layout.fillWidth: true 
		}
	}
}