import QtQuick
import org.ovito

ColorPicker {
	id: control

	property alias propertyField: parameterUI.propertyName
	property vector3d colorAsVector

	// Slave the color picker's displayed value to this object's property:
	color: Qt.rgba(colorAsVector.x, colorAsVector.y, colorAsVector.z, 1)

	// Synchronize the colorAsVector property's value with the underlying C++ object property.
	ParameterUI on colorAsVector {
		id: parameterUI
		editObject: propertyEditor.editObject
	}

	// This signal handler is called when the user picks a different color value:
	onColorModified: (c) => { parameterUI.propertyValue = Qt.vector3d(c.r, c.g, c.b); }
}