import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("Vector display")
	helpTopicId: "manual:visual_elements.vectors"
	
	GridLayout {
		anchors.fill: parent
		columns: 2

		Label { text: qsTr("Shading mode:"); }
		Ui.VariantComboBoxParameter {
			propertyField: "shadingMode" // PROPERTY_FIELD(VectorVis::shadingMode)
			Layout.fillWidth: true
			model: ListModel {
				ListElement { text: qsTr("Normal") } // CylinderPrimitive::NormalShading
				ListElement { text: qsTr("Flat") } // CylinderPrimitive::FlatShading
			}
		}

		Ui.ParameterLabel { propertyField: "scalingFactor" }
		Ui.FloatParameter { 
			propertyField: "scalingFactor" // PROPERTY_FIELD(VectorVis::scalingFactor)
			Layout.fillWidth: true 
		}

		Ui.ParameterLabel { propertyField: "arrowWidth" }
		Ui.FloatParameter { 
			propertyField: "arrowWidth" // PROPERTY_FIELD(VectorVis::arrowWidth)
			Layout.fillWidth: true 
		}

		Label { text: qsTr("Alignment:"); }
		Ui.VariantComboBoxParameter {
			propertyField: "arrowPosition" // PROPERTY_FIELD(VectorVis::arrowPosition)
			Layout.fillWidth: true
			model: ListModel {
				ListElement { text: qsTr("Base") } // VectorVis::Base
				ListElement { text: qsTr("Center") } // VectorVis::Center
				ListElement { text: qsTr("Head") } // VectorVis::Head
			}
		}

		Ui.BooleanCheckBoxParameter { 
			propertyField: "reverseArrowDirection" // PROPERTY_FIELD(VectorVis::reverseArrowDirection)
			Layout.row: 4
			Layout.column: 1
		}

		Ui.ParameterLabel { propertyField: "arrowColor" }
		Ui.ColorParameter { 
			propertyField: "arrowColor" // PROPERTY_FIELD(VectorVis::arrowColor)
			Layout.fillWidth: true 
		}

		Ui.ParameterLabel { propertyField: "transparencyController" }
		Ui.FloatParameter { 
			propertyField: "transparencyController" // PROPERTY_FIELD(VectorVis::transparencyController)
			Layout.fillWidth: true 
		}

		Ui.ParameterLabel { propertyField: "offset"; Layout.columnSpan: 2 }
		RowLayout {
			Layout.fillWidth: true 
			Layout.columnSpan: 2

			Ui.Vector3Parameter { 
				vectorComponent: 0
				propertyField: "offset" // PROPERTY_FIELD(VectorVis::offset)
				Layout.fillWidth: true 
			}
			Ui.Vector3Parameter { 
				vectorComponent: 1
				propertyField: "offset" // PROPERTY_FIELD(VectorVis::offset)
				Layout.fillWidth: true 
			}
			Ui.Vector3Parameter { 
				vectorComponent: 2
				propertyField: "offset" // PROPERTY_FIELD(VectorVis::offset)
				Layout.fillWidth: true 
			}
		}
	}
}