import QtQuick 6.0
import QtQuick.Controls 6.0
import org.ovito 1.0

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
