import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("Calculate displacements")
	helpTopicId: "manual:particles.modifiers.displacement_vectors"
	
	ColumnLayout {
		anchors.fill: parent

		GroupBox {
			title: qsTr("Affine mapping of simulation cell")
			Layout.fillWidth: true

			GridLayout {
				anchors.fill: parent
				columns: 2

				Ui.IntegerRadioButtonParameter { 
					propertyField: "affineMapping" // PROPERTY_FIELD(ReferenceConfigurationModifier::affineMapping)
					value: 0 // ReferenceConfigurationModifier::NO_MAPPING
					text: qsTr("Off")
				}
				Ui.IntegerRadioButtonParameter { 
					propertyField: "affineMapping" // PROPERTY_FIELD(ReferenceConfigurationModifier::affineMapping)
					value: 1 // ReferenceConfigurationModifier::TO_REFERENCE_CELL
					text: qsTr("To reference")
				}
				Ui.IntegerRadioButtonParameter { 
					propertyField: "affineMapping" // PROPERTY_FIELD(ReferenceConfigurationModifier::affineMapping)
					value: 2 // ReferenceConfigurationModifier::TO_CURRENT_CELL
					text: qsTr("To current")
					Layout.column: 1
					Layout.row: 1
				}

				Ui.BooleanCheckBoxParameter { 
					propertyField: "useMinimumImageConvention" // PROPERTY_FIELD(ReferenceConfigurationModifier::useMinimumImageConvention)
					Layout.column: 0
					Layout.row: 2
					Layout.columnSpan: 2
				}
			}
		}

		GroupBox {
			title: qsTr("Reference frame")
			Layout.fillWidth: true

			GridLayout {
				anchors.fill: parent
				columns: 2

				Ui.BooleanRadioButtonParameter { 
					id: constantReference
					Layout.columnSpan: 2
					propertyField: "useReferenceFrameOffset" // PROPERTY_FIELD(ReferenceConfigurationModifier::useReferenceFrameOffset)
					inverted: true
					text: qsTr("Constant reference configuration")
				}

				Ui.ParameterLabel { 
					propertyField: "referenceFrameNumber" 
					Layout.leftMargin: 30
				}
				Ui.IntegerParameter { 
					propertyField: "referenceFrameNumber" // PROPERTY_FIELD(ReferenceConfigurationModifier::referenceFrameNumber)
					Layout.fillWidth: true 
					enabled: constantReference.checked
				}

				Ui.BooleanRadioButtonParameter { 
					id: relativeReference
					Layout.columnSpan: 2
					propertyField: "useReferenceFrameOffset" // PROPERTY_FIELD(ReferenceConfigurationModifier::useReferenceFrameOffset)
					inverted: false
					text: qsTr("Relative to current frame")
				}

				Ui.ParameterLabel { 
					propertyField: "referenceFrameOffset" 
					Layout.leftMargin: 30
				}
				Ui.IntegerParameter { 
					propertyField: "referenceFrameOffset" // PROPERTY_FIELD(ReferenceConfigurationModifier::referenceFrameNumber)
					Layout.fillWidth: true 
					enabled: relativeReference.checked
				}
			}
		}

		Ui.ObjectStatusWidget {
			Layout.fillWidth: true
		}	
	}
}