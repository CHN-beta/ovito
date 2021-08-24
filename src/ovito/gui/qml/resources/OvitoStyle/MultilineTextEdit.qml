import QtQuick
import QtQuick.Controls

Control {
	id: control

	property alias text: edit.text

	signal editingFinished

	implicitHeight: 80
	hoverEnabled: true
	padding: background.border.width

	contentItem: ScrollView {	
		id: scrollView
		clip: true
		anchors.fill: parent

		TextArea {
			id: edit
			focus: true
			wrapMode: TextEdit.Wrap

			Keys.onReturnPressed: control.editingFinished()
			onEditingFinished: control.editingFinished() 

			KeyNavigation.priority: KeyNavigation.BeforeItem
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