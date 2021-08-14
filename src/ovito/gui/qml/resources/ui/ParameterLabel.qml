import QtQuick
import QtQuick.Controls
import org.ovito

Label {
	id: control

	property alias propertyField: parameterUI.propertyName
	text: parameterUI.propertyDisplayName + ":"

	ParameterUI {
		id: parameterUI
		editObject: propertyEditor.editObject
	}
}
