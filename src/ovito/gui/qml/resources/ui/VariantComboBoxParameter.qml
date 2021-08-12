import QtQuick
import QtQuick.Controls
import org.ovito

ComboBox {
	id: control

	property alias propertyField: parameterUI.propertyName
	property bool parameterValue: false

	ParameterUI on currentIndex {
		id: parameterUI
		editObject: propertyEditor.editObject
	}

	onActivated: { parameterUI.propertyValue = currentIndex; }
}
