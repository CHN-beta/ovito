import QtQuick 2.12
import QtQuick.Controls 2.5
import org.ovito 1.0

CustomComboBox {
	id: control
	textRole: "title"
	model: mainWindow.modifierListModel

	// Item delegate:
	delegate: ItemDelegate {
		width: control.width
		height: textItem.implicitHeight

		contentItem: Text {
			id: textItem
			anchors.fill: parent
			text: title
			color: isheader ? "#4444FF" : control.palette.text
			font.bold: isheader
			elide: Text.ElideRight
			horizontalAlignment: isheader ? Text.AlignHCenter : undefined
			verticalAlignment: Text.AlignVCenter
			leftPadding: 3
			rightPadding: 3
			topPadding: 1
			bottomPadding: 1
		}

		highlighted: control.highlightedIndex === index
	}
}