import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("Centrosymmetry parameter")
	
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