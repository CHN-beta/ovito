import QtQuick
import QtQuick.Controls

Dialog {
	modal: true
	anchors.centerIn: parent
	standardButtons: Dialog.Close
	contentWidth: 0.6 * parent.width
	contentHeight: 0.8 * parent.height

	ScrollView {
		anchors.fill: parent
		TextArea {
			id: label
			wrapMode: TextArea.Wrap
			readOnly: true
		}
	}

	onAboutToShow: label.text = mainWindow.systemReport
}
