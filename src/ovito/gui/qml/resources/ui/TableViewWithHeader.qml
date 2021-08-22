import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.qmlmodels 1.0

Control {
	id: control
	padding: background.border.width
	hoverEnabled: true

	property alias model: tableView.model
	property alias delegate: tableView.delegate
	property alias columnList: horizontalHeader.model
	property var columnWidths: []
	property int margins: 4	

	onWidthChanged: tableView.forceLayout()

	contentItem: ColumnLayout {	

		Rectangle {
			Layout.fillWidth: true
			color: Universal.chromeMediumLowColor
			implicitHeight: horizontalHeader.implicitHeight
			HorizontalHeaderView {
				id: horizontalHeader
				anchors.fill: parent
				syncView: tableView
				clip: true
				delegate: Text { 
					text: modelData
					font.bold: true
					padding: control.margins
					horizontalAlignment: Text.AlignLeft
					verticalAlignment: Text.AlignVCenter
				} 
			}
		}

		ScrollView {
			id: scrollView
			Layout.fillWidth: true
			Layout.fillHeight: true
			implicitHeight: 150
			clip: true

			// Avoid horizontal scrollbar:
			ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

			TableView {
				id: tableView
				columnSpacing: 2
				rowSpacing: 1
				columnWidthProvider: function(column) {
					var totalWidth = 0
					for(let i = 0; i < columnWidths.length; i++)
						totalWidth += columnWidths[i];					
					return control.availableWidth * columnWidths[column] / totalWidth;
				}
			}
		}
	}

	background: Rectangle {
		id: background
		color: control.enabled ? control.Universal.background : control.Universal.baseLowColor
		border.width: 2
        border.color: !control.enabled ? control.Universal.baseLowColor :
                       scrollView.activeFocus ? control.Universal.accent :
                       control.hovered ? control.Universal.baseMediumColor : control.Universal.chromeDisabledLowColor
	}	
}
