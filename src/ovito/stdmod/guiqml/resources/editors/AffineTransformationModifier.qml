import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "qrc:/gui/ui" as Ui

ColumnLayout {
	spacing: 2

	Ui.RolloutPanel {
		title: qsTr("Transformation")
		helpTopicId: "manual:particles.modifiers.affine_transformation"
		Layout.fillWidth: true

		GridLayout {
			id: layout
			anchors.fill: parent
			columns: 4
			property int inset: 30

			Ui.BooleanRadioButtonParameter { 
				id: relativeMode
				Layout.columnSpan: layout.columns
				propertyField: "relativeMode" // PROPERTY_FIELD(AffineTransformationModifier::relativeMode)
				inverted: false
				text: qsTr("Transformation matrix:")
			}

			Text { 
				Layout.column: 1
				Layout.row: 1
				Layout.columnSpan: 3
				text: "Rotate/Scale/Shear:"
			}
			Item {
				Layout.row: 2
				Layout.minimumWidth: layout.inset
				Layout.maximumWidth: layout.inset
			}
			// 1st row:
			Ui.MatrixParameter {
				matrixRow: 0
				matrixColumn: 0
				propertyField: "transformationTM" // PROPERTY_FIELD(AffineTransformationModifier::transformationTM)
				Layout.fillWidth: true
				enabled: relativeMode.checked
			}
			Ui.MatrixParameter {
				matrixRow: 0
				matrixColumn: 1
				propertyField: "transformationTM" // PROPERTY_FIELD(AffineTransformationModifier::transformationTM)
				Layout.fillWidth: true
				enabled: relativeMode.checked
			}
			Ui.MatrixParameter {
				matrixRow: 0
				matrixColumn: 2
				propertyField: "transformationTM" // PROPERTY_FIELD(AffineTransformationModifier::transformationTM)
				Layout.fillWidth: true
				enabled: relativeMode.checked
			}
			// 2nd row:
			Ui.MatrixParameter {
				Layout.column: 1
				Layout.row: 3
				matrixRow: 1
				matrixColumn: 0
				propertyField: "transformationTM" // PROPERTY_FIELD(AffineTransformationModifier::transformationTM)
				Layout.fillWidth: true
				enabled: relativeMode.checked
			}
			Ui.MatrixParameter {
				matrixRow: 1
				matrixColumn: 1
				propertyField: "transformationTM" // PROPERTY_FIELD(AffineTransformationModifier::transformationTM)
				Layout.fillWidth: true
				enabled: relativeMode.checked
			}
			Ui.MatrixParameter {
				matrixRow: 1
				matrixColumn: 2
				propertyField: "transformationTM" // PROPERTY_FIELD(AffineTransformationModifier::transformationTM)
				Layout.fillWidth: true
				enabled: relativeMode.checked
			}
			// 3rd row:
			Ui.MatrixParameter {
				Layout.column: 1
				Layout.row: 4
				matrixRow: 2
				matrixColumn: 0
				propertyField: "transformationTM" // PROPERTY_FIELD(AffineTransformationModifier::transformationTM)
				Layout.fillWidth: true
				enabled: relativeMode.checked
			}
			Ui.MatrixParameter {
				matrixRow: 2
				matrixColumn: 1
				propertyField: "transformationTM" // PROPERTY_FIELD(AffineTransformationModifier::transformationTM)
				Layout.fillWidth: true
				enabled: relativeMode.checked
			}
			Ui.MatrixParameter {
				matrixRow: 2
				matrixColumn: 2
				propertyField: "transformationTM" // PROPERTY_FIELD(AffineTransformationModifier::transformationTM)
				Layout.fillWidth: true
				enabled: relativeMode.checked
			}

			Text {
				Layout.column: 1
				Layout.row: 5
				Layout.columnSpan: 3
				text: "Translation:"
			}
			// Translation:
			Ui.MatrixParameter {
				Layout.column: 1
				Layout.row: 6
				matrixRow: 0
				matrixColumn: 3
				propertyField: "transformationTM" // PROPERTY_FIELD(AffineTransformationModifier::transformationTM)
				Layout.fillWidth: true
				enabled: relativeMode.checked
			}
			Ui.MatrixParameter {
				matrixRow: 1
				matrixColumn: 3
				propertyField: "transformationTM" // PROPERTY_FIELD(AffineTransformationModifier::transformationTM)
				Layout.fillWidth: true
				enabled: relativeMode.checked
			}
			Ui.MatrixParameter {
				matrixRow: 2
				matrixColumn: 3
				propertyField: "transformationTM" // PROPERTY_FIELD(AffineTransformationModifier::transformationTM)
				Layout.fillWidth: true
				enabled: relativeMode.checked
			}

			Ui.BooleanRadioButtonParameter { 
				id: targetCellMode
				Layout.row: 7
				Layout.columnSpan: layout.columns
				propertyField: "relativeMode" // PROPERTY_FIELD(AffineTransformationModifier::relativeMode)
				inverted: true
				text: qsTr("Transform to target cell:")
			}

			// 1st vector:
			Text { 
				Layout.column: 1
				Layout.row: 8
				Layout.columnSpan: 3
				text: "Cell vector 1:"
			}
			Ui.MatrixParameter {
				Layout.column: 1
				Layout.row: 9
				matrixRow: 0
				matrixColumn: 0
				propertyField: "targetCell" // PROPERTY_FIELD(AffineTransformationModifier::targetCell)
				Layout.fillWidth: true
				enabled: targetCellMode.checked
			}
			Ui.MatrixParameter {
				matrixRow: 1
				matrixColumn: 0
				propertyField: "targetCell" // PROPERTY_FIELD(AffineTransformationModifier::targetCell)
				Layout.fillWidth: true
				enabled: targetCellMode.checked
			}
			Ui.MatrixParameter {
				matrixRow: 2
				matrixColumn: 0
				propertyField: "targetCell" // PROPERTY_FIELD(AffineTransformationModifier::targetCell)
				Layout.fillWidth: true
				enabled: targetCellMode.checked
			}

			// 2nd vector:
			Text { 
				Layout.column: 1
				Layout.row: 10
				Layout.columnSpan: 3
				text: "Cell vector 2:"
			}
			Ui.MatrixParameter {
				Layout.column: 1
				Layout.row: 11
				matrixRow: 0
				matrixColumn: 1
				propertyField: "targetCell" // PROPERTY_FIELD(AffineTransformationModifier::targetCell)
				Layout.fillWidth: true
				enabled: targetCellMode.checked
			}
			Ui.MatrixParameter {
				matrixRow: 1
				matrixColumn: 1
				propertyField: "targetCell" // PROPERTY_FIELD(AffineTransformationModifier::targetCell)
				Layout.fillWidth: true
				enabled: targetCellMode.checked
			}
			Ui.MatrixParameter {
				matrixRow: 2
				matrixColumn: 1
				propertyField: "targetCell" // PROPERTY_FIELD(AffineTransformationModifier::targetCell)
				Layout.fillWidth: true
				enabled: targetCellMode.checked
			}

			// 3rd vector:
			Text { 
				Layout.column: 1
				Layout.row: 12
				Layout.columnSpan: 3
				text: "Cell vector 3:"
			}
			Ui.MatrixParameter {
				Layout.column: 1
				Layout.row: 13
				matrixRow: 0
				matrixColumn: 2
				propertyField: "targetCell" // PROPERTY_FIELD(AffineTransformationModifier::targetCell)
				Layout.fillWidth: true
				enabled: targetCellMode.checked
			}
			Ui.MatrixParameter {
				matrixRow: 1
				matrixColumn: 2
				propertyField: "targetCell" // PROPERTY_FIELD(AffineTransformationModifier::targetCell)
				Layout.fillWidth: true
				enabled: targetCellMode.checked
			}
			Ui.MatrixParameter {
				matrixRow: 2
				matrixColumn: 2
				propertyField: "targetCell" // PROPERTY_FIELD(AffineTransformationModifier::targetCell)
				Layout.fillWidth: true
				enabled: targetCellMode.checked
			}

			// Origin:
			Text { 
				Layout.column: 1
				Layout.row: 14
				Layout.columnSpan: 3
				text: "Cell origin:"
			}
			Ui.MatrixParameter {
				Layout.column: 1
				Layout.row: 15
				matrixRow: 0
				matrixColumn: 3
				propertyField: "targetCell" // PROPERTY_FIELD(AffineTransformationModifier::targetCell)
				Layout.fillWidth: true
				enabled: targetCellMode.checked
			}
			Ui.MatrixParameter {
				matrixRow: 1
				matrixColumn: 3
				propertyField: "targetCell" // PROPERTY_FIELD(AffineTransformationModifier::targetCell)
				Layout.fillWidth: true
				enabled: targetCellMode.checked
			}
			Ui.MatrixParameter {
				matrixRow: 2
				matrixColumn: 3
				propertyField: "targetCell" // PROPERTY_FIELD(AffineTransformationModifier::targetCell)
				Layout.fillWidth: true
				enabled: targetCellMode.checked
			}
		}
	}

	Ui.RolloutPanel {
		title: qsTr("Operate on")
		helpTopicId: "manual:particles.modifiers.affine_transformation"
		Layout.fillWidth: true

		ColumnLayout {
			anchors.fill: parent

			ModifierDelegateFixedListParameter {
				Layout.fillWidth: true
			}		

			Ui.BooleanCheckBoxParameter { 
				propertyField: "selectionOnly" // PROPERTY_FIELD(AffineTransformationModifier::selectionOnly)
			}
		}
	}
}