import QtQuick 6.0
import QtQuick.Controls 6.0
import org.ovito 1.0

CustomRadioButton {
	id: control

	property bool inverted: false
	property alias propertyField: parameterUI.propertyName
	property bool parameterValue: false

	checked: parameterValue != inverted

	ParameterUI on parameterValue {
		id: parameterUI
		editObject: propertyEditor.editObject
	}

	onToggled: { parameterUI.propertyValue = (checked != inverted); }
}
