import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "qrc:/gui/ui" as Ui

ColumnLayout {
	spacing: 2

	Ui.RolloutPanel {
		title: qsTr("Replicate")
		helpTopicId: "manual:particles.modifiers.show_periodic_images"
		Layout.fillWidth: true

		GridLayout {
			columns: 2
			columnSpacing: 2
			rowSpacing: 2
			anchors.fill: parent

			Label { text: qsTr("Number of images X:") }
			Ui.IntegerParameter { 
				propertyField: "numImagesX"
				Layout.fillWidth: true 
			}

			Label { text: qsTr("Number of images Y:") }
			Ui.IntegerParameter { 
				propertyField: "numImagesY"
				Layout.fillWidth: true 
			}

			Label { text: qsTr("Number of images Z:") }
			Ui.IntegerParameter { 
				propertyField: "numImagesZ"
				Layout.fillWidth: true 
			}

			Ui.BooleanCheckBoxParameter { 
				propertyField: "adjustBoxSize"
				Layout.columnSpan: 2
				Layout.fillWidth: true
			}

			Ui.BooleanCheckBoxParameter { 
				propertyField: "uniqueIdentifiers"
				Layout.columnSpan: 2
				Layout.fillWidth: true
			}
		}
	}

	Ui.RolloutPanel {
		title: qsTr("Operate on")
		helpTopicId: "manual:particles.modifiers.show_periodic_images"
		Layout.fillWidth: true

		ModifierDelegateFixedListParameter {
			anchors.fill: parent
		}		
	}
}