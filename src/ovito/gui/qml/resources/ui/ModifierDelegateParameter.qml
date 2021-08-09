import QtQuick 6.0
import QtQuick.Controls 6.0
import org.ovito 1.0

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
