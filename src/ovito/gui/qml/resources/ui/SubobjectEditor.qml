import QtQuick
import QtQuick.Controls
import org.ovito

Column {
	spacing: 2

	id: propertyEditor
	property RefTarget parentEditObject
	property RefTarget editObject
	property bool hasEditorComponent: subEditorList.model.length != 0
	property alias propertyField: parameterUI.propertyName
	readonly property ParameterUI subParameterUI: ParameterUI { editObject: propertyEditor.editObject }

	ParameterUI on editObject {
		id: parameterUI
		editObject: parentEditObject
	}

	/// Displays the editor components for the current edit object.
	Repeater {
		id: subEditorList 
		model: subParameterUI.editorComponentList

		Loader {
			source: modelData
			anchors.left: parent.left
			anchors.right: parent.right
		}
	}
}
