import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.ovito

Column {
	id: propertyEditor

	property RefTarget editObject
	readonly property ParameterUI parameterUI: ParameterUI { editObject: propertyEditor.editObject }

	topPadding: 2
	bottomPadding: 2
	spacing: 2

	/// Displays the editors for the current object.
	Repeater {
		id: editorList 
		model: parameterUI.editorComponentList

		/// This Loader loads the QML component containing the UI for the current RefTarget.
		Loader { 
			source: modelData
			anchors.left: parent.left
			anchors.right: parent.right
		}
	}

	/// Displays the sub-object editors for RefMaker reference fields that have the PROPERTY_FIELD_OPEN_SUBEDITOR flag set.
	Repeater {
		id: subobjectEditorList
		model: parameterUI.subobjectFieldList

		SubobjectEditor {
			propertyField: modelData
			parentEditObject: propertyEditor.editObject
			anchors.left: parent.left
			anchors.right: parent.right
			visible: hasEditorComponent
		}
	}
}
