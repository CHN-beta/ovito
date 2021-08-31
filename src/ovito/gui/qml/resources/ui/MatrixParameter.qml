import QtQuick
import QtQuick.Controls
import org.ovito

Spinner {
	id: control

	from: Math.max(parameterUI.minParameterValue, -(Math.pow(2, 31) - 1))
	to: Math.min(parameterUI.maxParameterValue, Math.pow(2, 31) - 1)

	property alias propertyField: parameterUI.propertyName
	property matrix4x4 matrixValue
	property int matrixRow: 0
	property int matrixColumn: 0

	// First time initialization of the spinner's value.
	Component.onCompleted: updateUIValue()

	// This signal handler is called when the program changes the parameter value.
//	onMatrixValueChanged: updateUIValue()

	ParameterUI /*on matrixValue*/ {
		id: parameterUI
		editObject: propertyEditor.editObject
		onEditObjectModified: updateUIValue() 		// NOTE: Workaround because the signal handler onMatrixValueChangedis not being called for some reason.
	}

	// This signal handler is called when the user enters a different value:
	onValueModified: { 
		matrixValue = parameterUI.propertyValue;
		if(matrixRow == 0) {
			if(matrixColumn == 0) matrixValue.m11 = value;
			else if(matrixColumn == 1) matrixValue.m12 = value;
			else if(matrixColumn == 2) matrixValue.m13 = value;
			else if(matrixColumn == 3) matrixValue.m14 = value;
		}
		else if(matrixRow == 1) {
			if(matrixColumn == 0) matrixValue.m21 = value;
			else if(matrixColumn == 1) matrixValue.m22 = value;
			else if(matrixColumn == 2) matrixValue.m23 = value;
			else if(matrixColumn == 3) matrixValue.m24 = value;
		}
		else if(matrixRow == 2) {
			if(matrixColumn == 0) matrixValue.m31 = value;
			else if(matrixColumn == 1) matrixValue.m32 = value;
			else if(matrixColumn == 2) matrixValue.m33 = value;
			else if(matrixColumn == 3) matrixValue.m34 = value;
		}
		else if(matrixRow == 3) {
			if(matrixColumn == 0) matrixValue.m41 = value;
			else if(matrixColumn == 1) matrixValue.m42 = value;
			else if(matrixColumn == 2) matrixValue.m43 = value;
			else if(matrixColumn == 3) matrixValue.m44 = value;
		}
		parameterUI.propertyValue = matrixValue;
	}

	function updateUIValue() {
		matrixValue = parameterUI.propertyValue;
		if(matrixRow == 0) {
			if(matrixColumn == 0) value = matrixValue.m11;
			else if(matrixColumn == 1) value = matrixValue.m12;
			else if(matrixColumn == 2) value = matrixValue.m13;
			else if(matrixColumn == 3) value = matrixValue.m14;
		}
		else if(matrixRow == 1) {
			if(matrixColumn == 0) value = matrixValue.m21;
			else if(matrixColumn == 1) value = matrixValue.m22;
			else if(matrixColumn == 2) value = matrixValue.m23;
			else if(matrixColumn == 3) value = matrixValue.m24;
		}
		else if(matrixRow == 2) {
			if(matrixColumn == 0) value = matrixValue.m31;
			else if(matrixColumn == 1) value = matrixValue.m32;
			else if(matrixColumn == 2) value = matrixValue.m33;
			else if(matrixColumn == 3) value = matrixValue.m34;
		}
		else if(matrixRow == 3) {
			if(matrixColumn == 0) value = matrixValue.m41;
			else if(matrixColumn == 1) value = matrixValue.m42;
			else if(matrixColumn == 2) value = matrixValue.m43;
			else if(matrixColumn == 3) value = matrixValue.m44;
		}
	}
}
