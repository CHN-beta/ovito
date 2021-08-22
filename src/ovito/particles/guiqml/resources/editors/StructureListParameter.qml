import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "qrc:/gui/ui" as Ui

Ui.RefTargetListParameter {
	id: control
	propertyField: "structureTypes"

	property int countColumnPos
	property int fractionColumnPos
	property int idColumnPos
	property int margins: 4

	property var structureCounts: []
	property int totalCount: 0

	// Update the displayed results whenever the modifier is reevaluated.
	Connections {
		target: propertyEditor
		function onModifierResultsComplete() {
			try {
				if(propertyEditor.editObject)
					structureCounts = propertyEditor.editObject.getStructureCountsFromModifierResults(propertyEditor.parentEditObject);
				totalCount = 0
				for(let i = 0; i < structureCounts.length; i++)
					totalCount += structureCounts[i];
			}
			catch(e) {}
		}
		Component.onCompleted: onModifierResultsComplete() // First time initialization
	}

	delegate: MouseArea {
		id: delegate
		width: ListView.view.width
		height: layout.implicitHeight + control.margins * 2
		onClicked: {
			delegate.ListView.view.currentIndex = index
			delegate.forceActiveFocus()
		}
		// Column: Structure
		RowLayout {
			id: layout
			anchors.left: parent.left
			anchors.top: parent.top
			anchors.bottom: parent.bottom
			width: control.countColumnPos
			spacing: 5
			clip: true
			CheckBox {
				padding: 0
				checked: reftarget.enabled
				onToggled: {
					mainWindow.undoableOperation("Enable/disable structure type", () => {
						model.reftarget.enabled = checked;
					}); 
				}
			}
			Ui.ColorPicker {
				implicitWidth: 16
				implicitHeight: 16
				flat: true
				color: reftarget.color
				onColorModified: (c) => {
					mainWindow.undoableOperation("Change structure color", () => {
						model.reftarget.color = c;
					}); 
				}
			}
			// Structure name
			Text {
				Layout.fillWidth: true
				Layout.fillHeight: true
				text: reftarget.objectTitle
				elide: Text.ElideRight
				horizontalAlignment: Text.AlignLeft
				verticalAlignment: Text.AlignVCenter
			}
		}
		// Column: Count
		Text {
			anchors.top: parent.top
			anchors.bottom: parent.bottom
			x: control.countColumnPos
			width: control.fractionColumnPos - control.countColumnPos
			text: (control.structureCounts && reftarget.enabled && reftarget.numericId < control.structureCounts.length) ? control.structureCounts[reftarget.numericId] : ""
			elide: Text.ElideRight
			horizontalAlignment: Text.AlignLeft
			verticalAlignment: Text.AlignVCenter
		}
		// Column: Fraction
		Text {
			anchors.top: parent.top
			anchors.bottom: parent.bottom
			x: control.fractionColumnPos
			width: control.idColumnPos - control.fractionColumnPos
			text: (control.structureCounts && reftarget.enabled && reftarget.numericId < control.structureCounts.length) ? 
					(control.structureCounts[reftarget.numericId] / Math.max(1, control.totalCount) * 100).toLocaleString(Qt.locale(), 'f', 1) + "%" : ""
			elide: Text.ElideRight
			horizontalAlignment: Text.AlignLeft
			verticalAlignment: Text.AlignVCenter
		}
		// Column: Id
		Text {
			anchors.top: parent.top
			anchors.bottom: parent.bottom
			x: control.idColumnPos
			width: parent.right - x
			text: reftarget.numericId
			elide: Text.ElideRight
			horizontalAlignment: Text.AlignLeft
			verticalAlignment: Text.AlignVCenter
		}
	}

	header: Rectangle {
		z: 2
		color: Universal.chromeMediumLowColor
		width: ListView.view ? ListView.view.width : 0
		height: structureColumn.implicitHeight

		SplitView {
			anchors.fill: parent
			orientation: Qt.Horizontal
			Text {
				id: structureColumn
				SplitView.fillWidth: true
				SplitView.minimumWidth: 10
				text: "Structure"
				elide: Text.ElideRight
				font.bold: true
				padding: control.margins
				horizontalAlignment: Text.AlignLeft
				verticalAlignment: Text.AlignVCenter
			}
			Text {
				id: countColumn
				SplitView.minimumWidth: 10
				text: "Count"
				elide: Text.ElideRight
				font.bold: true
				padding: control.margins
				horizontalAlignment: Text.AlignLeft
				verticalAlignment: Text.AlignVCenter
				onXChanged: control.countColumnPos = x
			}
			Text {
				id: fractionColumn
				SplitView.minimumWidth: 10
				text: "Fraction"
				elide: Text.ElideRight
				font.bold: true
				padding: control.margins
				horizontalAlignment: Text.AlignLeft
				verticalAlignment: Text.AlignVCenter
				onXChanged: control.fractionColumnPos = x
			}
			Text {
				id: idColumn
				SplitView.minimumWidth: 10
				text: "Id"
				font.bold: true
				padding: control.margins
				horizontalAlignment: Text.AlignLeft
				verticalAlignment: Text.AlignVCenter
				onXChanged: control.idColumnPos = x
			}
		}
	}
}