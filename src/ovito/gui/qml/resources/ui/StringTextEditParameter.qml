import QtQuick
import QtQuick.Controls
import org.ovito

MultilineTextEdit {
	id: control
	property alias propertyField: parameterUI.propertyName

	ParameterUI on text {
		id: parameterUI
		editObject: propertyEditor.editObject
	}

	onEditingFinished: { 
		parameterUI.propertyValue = text; 
	}
}