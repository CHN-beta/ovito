import QtQuick
import QtQuick.Controls
import org.ovito

Spinner {
	id: control

	from: Math.max(parameterUI.minParameterValue, -(Math.pow(2, 31) - 1))
	to: Math.min(parameterUI.maxParameterValue, Math.pow(2, 31) - 1)

	property alias propertyField: parameterUI.propertyName
	property vector3d vectorValue
	property int vectorComponent: 0

	value: (vectorComponent == 0) ? vectorValue.x : ((vectorComponent == 1) ? vectorValue.y : vectorValue.z)
	onVectorValueChanged: value = (vectorComponent == 0) ? vectorValue.x : ((vectorComponent == 1) ? vectorValue.y : vectorValue.z)

	ParameterUI on vectorValue {
		id: parameterUI
		editObject: propertyEditor.editObject
	}

	onValueModified: { 
		var newVec = vectorValue;
		if(vectorComponent == 0) newVec.x = value;
		if(vectorComponent == 1) newVec.y = value;
		if(vectorComponent == 2) newVec.z = value;
		parameterUI.propertyValue = newVec;
	}
}
