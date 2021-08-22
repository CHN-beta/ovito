import QtQuick
import QtQuick.Controls
import org.ovito

ComboBox {
	id: control

	property alias propertyField: parameterUI.propertyName
	property alias propertyContainer: parameterUI.propertyContainer
	property alias componentsMode: parameterUI.componentsMode
	property alias acceptablePropertyType: parameterUI.acceptablePropertyType
	model: parameterUI.model

	PropertyReferenceParameterUI on currentIndex {
		id: parameterUI
		editObject: propertyEditor.editObject
	}

	onActivated: { parameterUI.propertyValue = currentIndex; }
}
