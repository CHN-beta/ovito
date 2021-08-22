import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("Polyhedral template matching")
	helpTopicId: "manual:particles.modifiers.polyhedral_template_matching"
	
	ColumnLayout {
		anchors.fill: parent

		GroupBox {
			title: qsTr("Parameters")
			Layout.fillWidth: true

			GridLayout {
				anchors.fill: parent
				columns: 2

				Ui.ParameterLabel { propertyField: "rmsdCutoff" }
				Ui.FloatParameter { 
					propertyField: "rmsdCutoff"
					standardValue: 0.0
					placeholderText: qsTr("‹none›")
					Layout.fillWidth: true 
				}

				Ui.BooleanCheckBoxParameter { 
					propertyField: "onlySelectedParticles"
					Layout.columnSpan: 2
				}
			}
		}

		GroupBox {
			title: qsTr("Output")
			Layout.fillWidth: true

			ColumnLayout {
				anchors.fill: parent

				Ui.BooleanCheckBoxParameter { 
					propertyField: "outputRmsd"
					text: qsTr("RMSD values")
				}
				Ui.BooleanCheckBoxParameter { 
					propertyField: "outputInteratomicDistance"
					text: qsTr("Interatomic distances")
				}
				Ui.BooleanCheckBoxParameter { 
					propertyField: "outputOrderingTypes"
					text: qsTr("Chemical ordering types")
				}
				Ui.BooleanCheckBoxParameter { 
					propertyField: "outputDeformationGradient"
					text: qsTr("Elastic deformation gradients")
				}
				Ui.BooleanCheckBoxParameter { 
					propertyField: "outputOrientation"
					text: qsTr("Lattice orientations")
				}
				Ui.BooleanCheckBoxParameter { 
					propertyField: "colorByType"
				}
			}
		}

		StructureListParameter {
			id: elementTypeList
			Layout.fillWidth: true
			implicitHeight: 250
		}

		Ui.ObjectStatusWidget {
			Layout.fillWidth: true
		}	
	}
}