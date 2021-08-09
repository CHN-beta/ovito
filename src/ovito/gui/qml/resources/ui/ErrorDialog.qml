import QtQuick 6.0
import QtQuick.Controls 6.0

Dialog {
	id: dialog
	modal: true
	anchors.centerIn: parent
	standardButtons: Dialog.Ok
	contentWidth: 0.5 * parent.width
//	title: "Error"
	property string text;

	Text {
		text: "<h3>Error</h3><p></p>" + dialog.text
		textFormat: Text.StyledText
		wrapMode: Text.WordWrap
		anchors.fill: parent
	}
}
