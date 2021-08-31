import QtQuick
import QtQuick.Controls
import org.ovito

CustomRadioButton {
	id: control

	property bool inverted: false
	property alias propertyField: parameterUI.propertyName
	property bool parameterValue: false

	autoExclusive: false
	checked: parameterValue !== inverted

	ParameterUI on parameterValue {
		id: parameterUI
		editObject: propertyEditor.editObject
	}

	onToggled: { parameterUI.propertyValue = (checked != inverted); }
}
