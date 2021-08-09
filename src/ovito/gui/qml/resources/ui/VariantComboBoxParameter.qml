import QtQuick 6.0
import QtQuick.Controls 6.0
import org.ovito 1.0

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
