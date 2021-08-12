import QtQuick
import QtQuick.Controls

Dialog {
	id: dialog
	modal: true
	anchors.centerIn: parent
	standardButtons: Dialog.Ok
	contentWidth: 0.5 * parent.width
	property string text;

	Text {
		text: "<h3>Error</h3><p></p>" + dialog.text
		textFormat: Text.StyledText
		wrapMode: Text.WordWrap
		anchors.fill: parent
	}
}
