import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.ovito

ScrollView {
	id: propertyEditor

	/// The object currently being edited.
	property RefTarget editObject

	/// This property is only defined for compatibility with the SubobjectEditor component.
	readonly property RefTarget parentEditObject: null

	/// The C++ helper object, which handles events from the RefTarget being edited.
	readonly property ParameterUI parameterUI: ParameterUI { editObject: propertyEditor.editObject }

	// Do display elements outside of the scroll view.
	clip: true

	Flickable {
		contentHeight: column.childrenRect.height + column.topPadding + column.bottomPadding
		boundsBehavior: Flickable.StopAtBounds

		Column {
			id: column
			anchors.fill: parent
			topPadding: 2
			bottomPadding: 2
			spacing: 2

			/// Displays the editor panels for the current object.
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
	}

	background: Rectangle {
		color: propertyEditor.palette.window
/*
		Text {
			anchors.centerIn: parent
			visible: editObject != null && column.visibleChildren.length <= 2 
			text: "UI not implemented yet for this object"
		}
*/
	}
}
