import QtQuick
import QtQuick.Controls
import org.ovito

CheckBox {
	id: control

	property alias propertyField: parameterUI.propertyName
	property bool parameterValue: false
	text: parameterUI.propertyDisplayName

	ParameterUI on checked {
		id: parameterUI
		editObject: propertyEditor.editObject
	}

	onToggled: { parameterUI.propertyValue = checked; }
}
