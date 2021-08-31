import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.ovito

ScrollView {
	property alias model: listView.model
	property RefTarget selectedObject: listView.model ? listView.model.selectedObject : null

	clip: true

	// Avoid horizontal scrollbar:
	ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

	ListView {	
		id: listView
		focus: true
		highlightMoveDuration: 0
		highlightMoveVelocity: -1
		highlight: Rectangle { color: "lightsteelblue"; }

		// Keep the selected index of the model and the ListView in sync without creating a binding loop.
		onCurrentIndexChanged: model.selectedIndex = currentIndex;
		Connections {
			target: model
			function onSelectedIndexChanged() { listView.currentIndex = model.selectedIndex; }
		}

		Component {
			id: itemDelegate
			MouseArea {
				required property string title
				required property int type
				required property string tooltip
				required property string decoration
				required property bool ischecked
				required property int index
				id: mouseArea
				anchors	{
					left: parent ? parent.left : undefined
					right: parent ? parent.right : undefined
				}
				height: itemInfo.height
				hoverEnabled: true
				pressAndHoldInterval: 200
				property bool held: false
				drag {
					target: held ? itemInfo : undefined
					axis: Drag.YAxis
					threshold: 4
				}
				onClicked: { 
					if(type < PipelineListItem.VisualElementsHeader) 
						ListView.view.currentIndex = index; 
				}
				onPressAndHold: held = (type == PipelineListItem.Modifier)
	            onReleased: { if(held) itemInfo.Drag.drop(); held = false }
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
					anchors {
                    	verticalCenter: parent.verticalCenter
						left: parent.left
						right: parent.right
                	}
					color: (type >= PipelineListItem.VisualElementsHeader) ? "lightgray" : (mouseArea.held ? "#88f5f5dc" : "transparent");
					border.width: mouseArea.held ? 1 : 0;
					border.color: "black"

					Drag.active: mouseArea.held
					Drag.source: mouseArea
					Drag.hotSpot.x: width / 2
                	Drag.hotSpot.y: height / 2

					states: State {
						when: mouseArea.held

						ParentChange { target: itemInfo; parent: listView }
						AnchorChanges {
							target: itemInfo
							anchors { horizontalCenter: undefined; verticalCenter: undefined }
						}
					}

					// Enabled/disabled checkbox.
					CheckBox {
						id: checkboxItem
						visible: (type <= PipelineListItem.Modifier)
						checked: ischecked
						anchors.verticalCenter: parent.verticalCenter
						anchors.left: parent.left
						onToggled: {
							listView.model.setChecked(index, checked)
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

					// Item text
					IconLabel {
						id: textItem
						text: title
						icon.source: decoration
						icon.width: 22
						icon.height: 22
						spacing: 2
						alignment: (type < PipelineListItem.VisualElementsHeader) ? Qt.AlignLeft : Qt.AlignHCenter
						anchors.verticalCenter: parent.verticalCenter
						anchors.left: (type <= PipelineListItem.Modifier) ? checkboxItem.right : parent.left
						anchors.right: parent.right
						anchors.leftMargin: (type <= PipelineListItem.Modifier) ? -6 : 6
					}

					// Delete item button
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

					ToolTip.text: tooltip
					ToolTip.visible: tooltip ? mouseArea.containsMouse : false
					ToolTip.delay: 500
				}


				DropArea {
					id: dropAreaBefore
					anchors.left: parent.left
					anchors.right: parent.right
					anchors.verticalCenter: parent.top
					height: parent.height

					Rectangle {
						visible: dropAreaBefore.containsDrag
						anchors.centerIn: parent
						width: parent.width
						height: Math.max(2, parent.height / 6)
						color: "blue"
					}

					onDropped: function (drop) {
						model.performDragAndDropOperation([drop.source.index], mouseArea.index, false)
					}

					onEntered: function (drag) {
						if(model.performDragAndDropOperation([drag.source.index], mouseArea.index, true)) {
							drag.accept(Qt.MoveAction)
						}
						else {
							drag.accepted = false
						}
					}
				}
			}
		}
		delegate: itemDelegate
	}
}
