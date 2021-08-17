import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: propertyEditor.editObject ? propertyEditor.editObject.objectTitle : "Property"
	helpTopicId: "manual:scene_objects.particles"
	
	ColumnLayout {
		anchors.fill: parent

		Ui.RefTargetListParameter {
			id: elementTypeList
			Layout.fillWidth: true
			propertyField: "elementTypes"
			Component {
				id: itemDelegate
				MouseArea {
			        id: mouseArea
					width: ListView.view.width
					height: layout.implicitHeight + layout.anchors.topMargin + layout.anchors.bottomMargin
					onClicked: {
						mouseArea.ListView.view.currentIndex = index
						mouseArea.forceActiveFocus()
					}
					RowLayout {
						id: layout
						anchors.fill: parent
						anchors.margins: 4
						spacing: 3
						Rectangle {
							implicitWidth: 16
							implicitHeight: 16
							color: display.color
						}
						Text {
							Layout.fillWidth: true
							Layout.fillHeight: true
							text: display.objectTitle
							elide: Text.ElideRight
							horizontalAlignment: Text.AlignLeft
							verticalAlignment: Text.AlignVCenter
						}
						Text {
							Layout.fillHeight: true
							text: display.numericId
							horizontalAlignment: Text.AlignRight
							verticalAlignment: Text.AlignVCenter
						}
					}
				}
			}
			Component {
				id: headerDelegate
				Rectangle {
					z: 2
					width: ListView.view.width
					height: layout.implicitHeight + layout.anchors.topMargin + layout.anchors.bottomMargin
					color: Universal.chromeMediumLowColor
					RowLayout {
						id: layout
						anchors.fill: parent
						anchors.margins: 4
						spacing: 3
						Text {
							Layout.fillWidth: true
							Layout.fillHeight: true
							text: "Name"
							elide: Text.ElideRight
							font.bold: true
							horizontalAlignment: Text.AlignLeft
							verticalAlignment: Text.AlignVCenter
						}
						Text {
							Layout.fillHeight: true
							text: "Id"
							font.bold: true
							horizontalAlignment: Text.AlignRight
							verticalAlignment: Text.AlignVCenter
						}
					}
				}
			}
			delegate: itemDelegate
			header: headerDelegate
		}

		// Properties editor for the selected element type:
		Ui.EmbeddedPropertiesEditor {
			id: elementTypeEditor
			Layout.fillWidth: true
			Layout.fillHeight: true
			editObject: elementTypeList.selectedObject
		}
	}
}