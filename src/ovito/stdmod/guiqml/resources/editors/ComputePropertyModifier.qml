import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.ovito

import "qrc:/gui/ui" as Ui

ColumnLayout {
	spacing: 2

	Connections {
		target: propertyEditor.editObject
		function onPropertyValueChangedSignal() { 
			target.adjustPropertyComponentCount();
		}
		function onPropertyComponentNamesChanged() {
			repeater.componentNames = target.propertyComponentNames;
		}
	}

	Ui.RolloutPanel {
		title: qsTr("Compute property")
		helpTopicId: "manual:particles.modifiers.compute_property"
		Layout.fillWidth: true

		ColumnLayout {
			anchors.fill: parent

			GroupBox {
				title: qsTr("Operate on:")
				Layout.fillWidth: true 

				Ui.ModifierDelegateParameter {
					anchors.fill: parent
					delegateType: "ComputePropertyModifierDelegate"
				}
			}

			GroupBox {
				title: qsTr("Output property:")
				Layout.fillWidth: true 

				ColumnLayout {
					anchors.fill: parent
					spacing: 2

					Ui.PropertyReferenceParameter {
						propertyContainer: propertyEditor.editObject.delegate.inputContainerRef
						propertyField: "outputProperty" // PROPERTY_FIELD(SelectTypeModifier::outputProperty)
						componentsMode: PropertyReferenceParameterUI.ShowNoComponents
						propertyParameterType: PropertyReferenceParameterUI.OutputProperty
						Layout.fillWidth: true
					}

					Ui.BooleanCheckBoxParameter { 
						propertyField: "onlySelectedElements"
					}
				}
			}

			GroupBox {
				title: qsTr("Expression:")
				Layout.fillWidth: true 

				ColumnLayout {
					anchors.fill: parent
					spacing: 2

					Repeater {
						id: repeater
						property var expressions
						property var componentNames
						model: expressions.length

						ParameterUI on expressions {
							id: expressionParameterUI
							editObject: propertyEditor.editObject
							propertyName: "expressions"
							function storeExpressions() {
								var exprList = [];
								for(var i = 0; i < repeater.count; i++) {
									exprList.push(repeater.itemAt(i).text);
								}
								propertyValue = exprList; 
							}
						}
						onExpressionsChanged: {
							for(var i = 0; i < repeater.count; i++) {
								repeater.itemAt(i).text = (i < expressions.length) ? expressions[i] : "";
							}
						}

						delegate: RowLayout {
							property alias text: editField.text
							Layout.fillWidth: true
							Text {
								text: ((repeater.componentNames && modelData < repeater.componentNames.length) ? repeater.componentNames[modelData] : modelData) + ":"
								visible: (repeater.componentNames && repeater.componentNames.length > 0) || repeater.expressions.length > 1
								Layout.minimumWidth: 14
								Layout.alignment: Qt.AlignTop
								Layout.topMargin: 4
							}
							MultilineTextEdit {
								id: editField
								Layout.fillWidth: true
								onEditingFinished: expressionParameterUI.storeExpressions()
								Component.onCompleted: { text = repeater.expressions[modelData]; }
							}
						}
					}
				}
			}

			Ui.ObjectStatusWidget {
				Layout.columnSpan: 2
				Layout.fillWidth: true
			}		
		}
	}

	Ui.RolloutPanel {
		title: qsTr("Variables")
		helpTopicId: "manual:particles.modifiers.compute_property"
		Layout.fillWidth: true

		Text {
			anchors.fill: parent
			wrapMode: Text.Wrap
			text: propertyEditor.parentEditObject.inputVariableTable
		}
	}
}