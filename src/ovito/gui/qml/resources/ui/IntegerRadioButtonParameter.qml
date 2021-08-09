import QtQuick 6.0
import QtQuick.Controls 6.0
import org.ovito 1.0

CustomRadioButton {
	id: control

	property int value: 0
	property alias propertyField: parameterUI.propertyName
	property int parameterValue: 0

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
