import QtQuick
import QtQuick.Controls
import QtQuick.Layouts 6.0
import org.ovito

ScrollView {
	property alias model: listView.model
	property RefTarget selectedObject: listView.model ? listView.model.selectedObject : null

	clip: true

	ListView {	
		id: listView
		focus: true
		highlightMoveDuration: 0
		highlightMoveVelocity: -1

		// Keep the selected index of the model and the ListView in sync without creating a binding loop.
		onCurrentIndexChanged: model.selectedIndex = currentIndex;
		Connections {
			target: model
			function onSelectedIndexChanged() { listView.currentIndex = model.selectedIndex; }
		}

		Component {
			id: itemDelegate
			MouseArea {
				id: mouseArea
				anchors { 
					left: parent ? parent.left : undefined; 
					right: parent ? parent.right : undefined; 
				}
				height: itemInfo.height
				hoverEnabled: true
				onClicked: { 
					if(type < PipelineListItem.VisualElementsHeader) 
						ListView.view.currentIndex = index; 
				}
				states: State {
					name: "HOVERING"
					when: (containsMouse && type == PipelineListItem.Modifier)
					PropertyChanges { target: deleteButton; opacity: 0.85 }
				}
				transitions: Transition {
					to: "*"
					NumberAnimation { target: deleteButton; property: "opacity"; duration: 100; }
				}			
				Rectangle { 
					id: itemInfo
					height: (type >= PipelineListItem.VisualElementsHeader) ? textItem.implicitHeight : Math.max(textItem.implicitHeight, checkboxItem.implicitHeight)
					width: parent.width
					color: (type >= PipelineListItem.VisualElementsHeader) ? "lightgray" : "#00000000"; 
					CheckBox {
						id: checkboxItem
						visible: (type <= PipelineListItem.Modifier)
						checked: model.ischecked
						anchors.verticalCenter: parent.verticalCenter
						anchors.left: parent.left
						onToggled: {
							model.ischecked = checked;
						}
						indicator: Rectangle {
							implicitWidth: 18
							implicitHeight: 18
							x: checkboxItem.leftPadding
							y: parent.height / 2 - height / 2
							border.color: checkboxItem.down ? "#000000" : "#444444"
							radius: 2

							Rectangle {
								width: 12
								height: 12
								x: 3
								y: 3
								radius: 2
								color: checkboxItem.down ? "#000000" : "#444444"
								visible: checkboxItem.checked
							}
						}
					}
					Text {
						id: textItem
						text: title
						horizontalAlignment: (type < PipelineListItem.VisualElementsHeader) ? Text.AlignLeft : Text.AlignHCenter
						anchors.verticalCenter: parent.verticalCenter
						anchors.left: (type <= PipelineListItem.Modifier) ? checkboxItem.right : parent.left
						anchors.right: parent.right
						anchors.leftMargin: (type <= PipelineListItem.Modifier) ? 0 : 6
					}
					Image {
						id: deleteButton
						anchors.verticalCenter: parent.verticalCenter
						anchors.right: parent.right
						anchors.rightMargin: 4.0
						opacity: 0
						source: "qrc:/guibase/actions/modify/delete_modifier.bw.svg"
						MouseArea {
							anchors.fill: parent
							onClicked: { 
								listView.model.deleteItemIndex(index)
							}
						}
					}
				}
			}
		}
		delegate: itemDelegate
		highlight: Rectangle { color: "lightsteelblue"; }
	}
}
