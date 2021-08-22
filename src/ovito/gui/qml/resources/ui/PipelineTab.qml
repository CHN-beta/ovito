import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
	spacing: 0

	// Available modifiers list:
	ModifierListBox {
		Layout.fillWidth: true

		// When the user selects a modifier from the list box, insert that modifier into the pipeline
		// and reset the selection back to the first item ("Add modification...").
		onActivated: function (index) {
			mainWindow.modifierListModel.insertModifierByIndex(index)
			currentIndex = 0;
		}
	}

	SplitView {
		orientation: Qt.Vertical
		Layout.fillWidth: true
		Layout.fillHeight: true

		// Pipeline editor:
		PipelineEditor {
			id: pipelineEditor
			Layout.preferredHeight: 100
			SplitView.minimumHeight: 30
			model: mainWindow.pipelineListModel

			// Request a viewport update whenever a new item in the pipeline editor is selected, 
			// because the currently selected modifier may be rendering gizmos in the viewports. 
			onSelectedObjectChanged: {
				if(viewportsPanel.viewportConfiguration)
					viewportsPanel.viewportConfiguration.updateViewports();
			}
		}
		
		// Properties editor panel:
		PropertiesEditor {
			SplitView.preferredHeight: 160
			SplitView.minimumHeight: 30
			editObject: pipelineEditor.selectedObject
		}
	}
}
