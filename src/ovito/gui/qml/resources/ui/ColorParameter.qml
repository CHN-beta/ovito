import QtQuick
import org.ovito

ColorPicker {
	id: control

	property alias propertyField: parameterUI.propertyName
	property vector3d colorAsVector
	color: Qt.rgba(colorAsVector.x, colorAsVector.y, colorAsVector.z, 1)

	ParameterUI on colorAsVector {
		id: parameterUI
		editObject: propertyEditor.editObject
	}

	onColorModified: parameterUI.propertyValue = Qt.vector3d(color.r, color.g, color.b)
	onColorAsVectorChanged: color = Qt.rgba(colorAsVector.x, colorAsVector.y, colorAsVector.z, 1)
}
