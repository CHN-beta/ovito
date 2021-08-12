import QtQuick
import QtQuick.Controls
import org.ovito

ComboBox {
	id: control
	textRole: "title"
	model: mainWindow.modifierListModel
	flat: false

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