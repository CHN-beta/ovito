import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GroupBox {
	id: control

	/// The anchor of the help page in the OVITO user manual, which is opened by the help button of the panel.
	property string helpTopicId;

	property bool frameVisible: !(propertyEditor instanceof EmbeddedPropertiesEditor)
	leftPadding: frameVisible ? 6 : 0
	rightPadding: frameVisible ? 6 : 0
	topPadding: control.frameVisible ? 6 + titleLabel.height : 0
	bottomPadding: control.frameVisible ? 6 : 0
	
	label: RowLayout {
		x: 2
		width: control.width - 4
		spacing: 0
		visible: control.frameVisible

		/// Title bar.
		Label {
			id: titleLabel
			Layout.fillWidth: true
			Layout.fillHeight: true
			text: control.title
			font: control.font
			horizontalAlignment: Text.AlignHCenter
			color: "white"
			elide: Text.ElideRight
			topPadding: 2
			bottomPadding: 2
			bottomInset: -1
			background: Rectangle {
				color: control.palette.dark
				border.width: 1
				border.color: "#060606"
			}
		}

		/// Help button.
		AbstractButton {
			id: helpButton
			visible: control.helpTopicId
			hoverEnabled: true
			contentItem: Label {
				text: "?"
				font: control.font
				horizontalAlignment: Text.AlignHCenter
				color: titleLabel.color
				leftPadding: 6
				rightPadding: 6
				topPadding: titleLabel.topPadding
				bottomPadding: titleLabel.bottomPadding
				leftInset: -1
				bottomInset: -1
				background: Rectangle {
					color: helpButton.hovered ? "#008000" : "#006000"
					border.width: 1
					border.color: helpButton.down ? "#080808" : "#060606"
				}
			}
			onClicked: mainWindow.openHelpTopic(control.helpTopicId)
		}
	}

	background: Rectangle {
		visible: control.frameVisible
		anchors.top: label.bottom 
		anchors.left: label.left
		anchors.right: label.right
		height: parent.height - control.topPadding + control.bottomPadding
		color: "transparent"
		border.color: "#909090"
	}
}
