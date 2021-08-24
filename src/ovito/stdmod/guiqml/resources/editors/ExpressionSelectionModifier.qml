import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "qrc:/gui/ui" as Ui

ColumnLayout {
	spacing: 2

	Ui.RolloutPanel {
		title: qsTr("Expression selection")
		helpTopicId: "manual:particles.modifiers.expression_select"
		Layout.fillWidth: true

		GridLayout {
			anchors.fill: parent
			columns: 2

			Label { text: qsTr("Operate on:") }
			Ui.ModifierDelegateParameter {
				delegateType: "ExpressionSelectionModifierDelegate"
				Layout.fillWidth: true
			}

			Label { 
				text: qsTr("Boolean expression:") 
				Layout.columnSpan: 2
			}
			Ui.StringTextEditParameter { 
				propertyField: "expression"
				Layout.columnSpan: 2
				Layout.fillWidth: true
			}

			Ui.ObjectStatusWidget {
				Layout.columnSpan: 2
				Layout.fillWidth: true
			}		
		}
	}

	Ui.RolloutPanel {
		title: qsTr("Variables")
		helpTopicId: "manual:particles.modifiers.expression_select"
		Layout.fillWidth: true

		Text {
			anchors.fill: parent
			wrapMode: Text.Wrap
			text: propertyEditor.editObject.inputVariableTable
		}
	}
}