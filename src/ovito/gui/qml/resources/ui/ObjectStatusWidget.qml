import QtQuick
import QtQuick.Controls
import org.ovito

Control {
	id: control
	padding: background.border.width

	readonly property int statusSuccess: 0   //< Indicates that the evaluation was successfull.
	readonly property int statusWarning: 1   //< Indicates that a modifier has issued a warning.
	readonly property int statusError: 2     //< Indicates that the evaluation failed.
	property int statusType

    Universal.theme: activeFocus ? Universal.Light : undefined

	Connections {
		target: propertyEditor.parentEditObject ? propertyEditor.parentEditObject : propertyEditor.editObject
		function onObjectStatusChanged() {
			label.text = target.status.text
			control.statusType = target.status.type
		}
		Component.onCompleted: onObjectStatusChanged()
	}

	contentItem: ScrollView {
		id: scrollView
		implicitHeight: 40
		clip: true
		ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

		Label {
			id: label
			width: control.availableWidth
			wrapMode: Text.Wrap
			padding: 2
		}
	}

	background: Rectangle {
		id: background
		color: "transparent"
		border.width: 2
        border.color: (statusType == statusSuccess) ? control.Universal.baseLowColor : (statusType == statusError ? "red" : "orange")
    }
}
