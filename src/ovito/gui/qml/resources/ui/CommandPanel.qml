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
			icon.source: "qrc:/gui/command_panel/tab_modify.bw.svg"
		}
		TabButton {
			text: qsTr("Render")
			icon.source: "qrc:/gui/command_panel/tab_render.bw.svg"
		}
		TabButton {
			text: qsTr("Layers")
			icon.source: "qrc:/gui/command_panel/tab_overlays.bw.svg"
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
