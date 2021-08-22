import QtQuick
import QtQuick.Controls
import org.ovito

Column {
	id: propertyEditor
	spacing: 2

	/// The RefTarget being edited in this components parent editor.
	property RefTarget parentEditObject

	/// The referenced sub-object being edited in this editor component.
	property RefTarget editObject

	/// Indicates whether there exists an editor implementation has been loaded for the sub-object.
	property bool hasEditorComponent: subEditorList.model.length != 0

	/// The reference field of the parent object which references the sub-object being edited in this editor.
	property alias propertyField: parameterUI.propertyName

	/// The C++ helper object, which handles events from the RefTarget sub-object being edited.
	readonly property ParameterUI subParameterUI: ParameterUI { 
		editObject: propertyEditor.editObject 
		onEditObjectModified: propertyEditor.editObjectModified()
	}

	/// This signal is emitted by the editor for a Modifier when the pipeline results of the modifier 
	/// have been recalculated or when the modifier is loaded into the editor for the first time.
	signal modifierResultsComplete

	/// This signal is emitted by the editor for a Modifier when the upstream pipeline results for the modifier have changed.
	signal modifierInputChanged

	/// This signal is emitted by the editor whenever the object in the editor changes state.
	signal editObjectModified

	/// This C++ helper object updates the component's internal reference to the sub-object
	/// to keep it in sync with the reference held by the parent object.
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

	// Listen for signals sent by the ModifierApplication and update the displayed modifier results whenever the pipeline is reevaluated.
	Connections {
		ignoreUnknownSignals: true
		target: parentEditObject // Monitor the ModifierApplication associated with the modifier. The ModifierApplication is loaded in the parent editor.
		function onModifierResultsComplete() { modifierResultsComplete() } // Emit the editor's own signal when the ModifierApplication emitted its signal.
		function onModifierInputChanged() { modifierInputChanged() } // Emit the editor's own signal when the ModifierApplication has new inputs.
	}
	// Also emit the modifierResultsComplete signal whenever a new ModifierApplication/Modifier pair is loaded into the parent/child editor
	// as well as upon first-time initialization of the editor.
	onEditObjectChanged: modifierResultsComplete()
}
