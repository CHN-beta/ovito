import QtQuick
import QtQuick.Controls
import org.ovito

ComboBox {
	id: control

	property alias propertyField: parameterUI.propertyName
	property alias propertyContainer: parameterUI.propertyContainer
	property alias componentsMode: parameterUI.componentsMode
	property alias acceptablePropertyType: parameterUI.acceptablePropertyType
	property alias propertyParameterType: parameterUI.propertyParameterType
	model: parameterUI.model
	editable: propertyParameterType == PropertyReferenceParameterUI.OutputProperty
	selectTextByMouse: true

	PropertyReferenceParameterUI on currentIndex {
		id: parameterUI
		editObject: propertyEditor.editObject
		onCurrentPropertyNameChanged: { if(editable) control.editText = currentPropertyName; }
	}
	Component.onCompleted: { parameterUI.onCurrentPropertyNameChanged(); }

	onActivated: { parameterUI.propertyValue = currentIndex; }
	onAccepted: { if(editable) parameterUI.propertyValue = editText; }
}
