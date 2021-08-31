import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("Atomic strain")
	helpTopicId: "manual:particles.modifiers.atomic_strain"
	
	ColumnLayout {
		anchors.fill: parent

		RowLayout {
			Layout.fillWidth: true

			Ui.ParameterLabel { propertyField: "cutoff" }
			Ui.FloatParameter { 
				propertyField: "cutoff" // PROPERTY_FIELD(AtomicStrainModifier::cutoff)
				Layout.fillWidth: true 
			}
		}

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
			}
		}

		Ui.BooleanCheckBoxParameter { 
			propertyField: "useMinimumImageConvention" // PROPERTY_FIELD(ReferenceConfigurationModifier::useMinimumImageConvention)
		}
		Ui.BooleanCheckBoxParameter { 
			propertyField: "calculateDeformationGradients" // PROPERTY_FIELD(AtomicStrainModifier::calculateDeformationGradients)
		}
		Ui.BooleanCheckBoxParameter { 
			propertyField: "calculateStrainTensors" // PROPERTY_FIELD(AtomicStrainModifier::calculateStrainTensors)
		}
		Ui.BooleanCheckBoxParameter { 
			propertyField: "calculateNonaffineSquaredDisplacements" // PROPERTY_FIELD(AtomicStrainModifier::calculateNonaffineSquaredDisplacements)
		}
		Ui.BooleanCheckBoxParameter { 
			propertyField: "calculateRotations" // PROPERTY_FIELD(AtomicStrainModifier::calculateRotations)
		}
		Ui.BooleanCheckBoxParameter { 
			propertyField: "calculateStretchTensors" // PROPERTY_FIELD(AtomicStrainModifier::calculateStretchTensors)
		}
		Ui.BooleanCheckBoxParameter { 
			propertyField: "selectInvalidParticles" // PROPERTY_FIELD(AtomicStrainModifier::selectInvalidParticles)
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