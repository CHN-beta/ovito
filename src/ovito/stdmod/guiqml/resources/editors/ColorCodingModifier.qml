import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.ovito

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("Color coding")
	helpTopicId: "manual:particles.modifiers.color_coding"

	ColumnLayout {
		anchors.fill: parent

		Label { text: qsTr("Operate on:") }
		Ui.ModifierDelegateParameter {
			delegateType: "ColorCodingModifierDelegate"
			Layout.fillWidth: true 
		}

		Label { text: qsTr("Input property:") }
		Ui.PropertyReferenceParameter {
			propertyContainer: propertyEditor.editObject.delegate.inputContainerRef
			propertyField: "sourceProperty" // PROPERTY_FIELD(ColorCodingModifier::sourceProperty)
			componentsMode: PropertyReferenceParameterUI.ShowOnlyComponents
			Layout.fillWidth: true
		}

		Label { text: qsTr("Color gradient:") }
		ComboBox {
			Layout.fillWidth: true
			textRole: "text"
			valueRole: "typeName"
			model: ListModel {
				ListElement { text: "Grayscale"; typeName: "ColorCodingGrayscaleGradient" }
				ListElement { text: "Hot"; typeName: "ColorCodingHotGradient" }
				ListElement { text: "Jet"; typeName: "ColorCodingJetGradient" }
				ListElement { text: "Blue-White-Red"; typeName: "ColorCodingBlueWhiteRedGradient" }
				ListElement { text: "Rainbow"; typeName: "ColorCodingHSVGradient" }
				ListElement { text: "Viridis"; typeName: "ColorCodingViridisGradient" }
				ListElement { text: "Magma"; typeName: "ColorCodingMagmaGradient" }
			}
			property string selectedTypeName
			ParameterUI on selectedTypeName {
				id: parameterUI
				propertyName: "colorGradientType"
				editObject: propertyEditor.editObject
			}
			Component.onCompleted: { currentIndex = indexOfValue(selectedTypeName) }
			onSelectedTypeNameChanged: { currentIndex = indexOfValue(selectedTypeName) }
			onActivated: { parameterUI.propertyValue = currentValue; }
		}

		GridLayout {
			Layout.fillWidth: true
			columns: 2
			rowSpacing: 0

			Ui.ParameterLabel { propertyField: "endValueController" }
			Ui.FloatParameter {
				propertyField: "endValueController" // PROPERTY_FIELD(ColorCodingModifier::endValueController)
				Layout.fillWidth: true
			}

			// Color gradient display
			ColumnLayout {
				Layout.row: 1
				Layout.column: 1
				Layout.fillWidth: true
				spacing: 0

				Repeater {
					id: repeater
					model: 60
					delegate: Rectangle {
						implicitHeight: 1
						Layout.fillWidth: true
						color: propertyEditor.editObject.colorGradient.valueToColor(index / repeater.model)
					}
				}
			}

			Ui.ParameterLabel { propertyField: "startValueController" }
			Ui.FloatParameter {
				propertyField: "startValueController" // PROPERTY_FIELD(ColorCodingModifier::startValueController)
				Layout.fillWidth: true
			}

//			Ui.BooleanCheckBoxParameter { 
//				propertyField: "autoAdjustRange" // PROPERTY_FIELD(ColorCodingModifier::autoAdjustRange)
//				Layout.row: 2
//				Layout.column: 1
//			}
		}

		Button {
			Layout.fillWidth: true
			text: "Adjust range"
			onClicked: {
				mainWindow.undoableOperation("Adjust range", () => {
					propertyEditor.editObject.adjustRange()
				}); 
			}
		}

		Button {
			Layout.fillWidth: true
			text: "Reverse range"
			onClicked: {
				mainWindow.undoableOperation("Reverse range", () => {
					propertyEditor.editObject.reverseRange()
				}); 
			}
		}

		Ui.BooleanCheckBoxParameter { 
			id: colorOnlySelectedUI
			propertyField: "colorOnlySelected"
			Layout.fillWidth: true
		}

		Ui.BooleanCheckBoxParameter { 
			propertyField: "keepSelection"
			enabled: colorOnlySelectedUI.checked
			Layout.fillWidth: true
		}
	}
}
