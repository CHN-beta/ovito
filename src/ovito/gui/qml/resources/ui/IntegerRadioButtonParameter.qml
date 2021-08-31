import QtQuick
import QtQuick.Controls
import org.ovito

CustomRadioButton {
	id: control

	property int value: 0
	property alias propertyField: parameterUI.propertyName
	property int parameterValue: -1

	autoExclusive: false
	checked: parameterValue == value

	ParameterUI on parameterValue {
		id: parameterUI
		editObject: propertyEditor.editObject
	}

	onToggled: { 
		if(checked)
			parameterUI.propertyValue = value; 
	}
}
