import QtQuick
import QtQuick.Controls
import org.ovito

ComboBox {
	id: control

	property alias propertyField: parameterUI.propertyName
	property alias dataObjectType: parameterUI.dataObjectType
	property var selectedDataObject: parameterUI.get(currentIndex)
	model: parameterUI.model
	textRole: "label"

	DataObjectReferenceParameterUI on currentIndex {
		id: parameterUI
		editObject: propertyEditor.editObject
	}

	onActivated: { parameterUI.propertyValue = currentIndex; }
}
