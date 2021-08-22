import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "qrc:/gui/ui" as Ui

Column {
	spacing: 2

	Ui.RolloutPanel {
		title: qsTr("Create bonds")
		helpTopicId: "manual:particles.modifiers.create_bonds"
		anchors.left: parent.left
		anchors.right: parent.right
		
		GridLayout {
			anchors.fill: parent
			columns: 2

			GroupBox {
				title: qsTr("Mode:")
				Layout.columnSpan: 2
				Layout.fillWidth: true 

				ColumnLayout {
					anchors.fill: parent

					Ui.IntegerRadioButtonParameter { 
						id: modeUniformCutoff
						propertyField: "cutoffMode"
						value: 0 // CreateBondsModifier::UniformCutoff
						text: qsTr("Uniform cutoff:")
					}
					Ui.FloatParameter {
						propertyField: "uniformCutoff"
						Layout.fillWidth: true
						Layout.leftMargin: 30
						enabled: modeUniformCutoff.checked
					}

					Ui.IntegerRadioButtonParameter { 
						id: modeTypeRadiusCutoff
						propertyField: "cutoffMode"
						value: 2 // CreateBondsModifier::TypeRadiusCutoff
						text: qsTr("Van der Waals radii:")
					}
					Ui.BooleanCheckBoxParameter {
						propertyField: "skipHydrogenHydrogenBonds"
						Layout.fillWidth: true
						Layout.leftMargin: 30
						enabled: modeTypeRadiusCutoff.checked
					}
				}
			}

			Label { text: qsTr("Lower cutoff:"); }
			Ui.FloatParameter { 
				propertyField: "minimumCutoff"
				Layout.fillWidth: true 
			}

			Ui.BooleanCheckBoxParameter { 
				propertyField: "onlyIntraMoleculeBonds"
				Layout.columnSpan: 2
			}

			Ui.ObjectStatusWidget {
				Layout.columnSpan: 2
				Layout.fillWidth: true
			}				
		}
	}

	Ui.SubobjectEditor {
		propertyField: "bondType"
		parentEditObject: propertyEditor.editObject
		anchors.left: parent.left
		anchors.right: parent.right
	}

	Ui.SubobjectEditor {
		propertyField: "bondsVis"
		parentEditObject: propertyEditor.editObject
		anchors.left: parent.left
		anchors.right: parent.right
	}
}