import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
	Layout.fillHeight: true
	
	header: TabBar {
		id: bar
		visible: false
		TabButton {
			text: qsTr("Pipeline")
			icon.name: "command_panel_tab_modify"
		}
		TabButton {
			text: qsTr("Render")
			icon.name: "command_panel_tab_render"
		}
		TabButton {
			text: qsTr("Layers")
			icon.name: "command_panel_tab_overlays"
		}
	}

	StackLayout {
		anchors.fill: parent
		currentIndex: bar.currentIndex

		PipelineTab {
			id: pipelineTab
		}
		Item {
			id: renderingTab
			Text {
				anchors.centerIn: parent
				text: "Not implemented yet"
			}
		}
		Item {
			id: layersTab
			Text {
				anchors.centerIn: parent
				text: "Not implemented yet"
			}
		}
	}	
}
