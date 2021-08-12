import QtQuick
import QtQuick.Controls
import org.ovito

ComboBox {
	id: control

	property alias propertyField: parameterUI.propertyName
	property alias delegateType: parameterUI.delegateType
	propertyField: "delegate"
	model: parameterUI.delegateList

	ModifierDelegateParameterUI on currentIndex {
		id: parameterUI
		editObject: propertyEditor.editObject
	}

	onActivated: { parameterUI.propertyValue = currentIndex; }
}
