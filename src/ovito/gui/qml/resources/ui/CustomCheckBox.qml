import QtQuick 2.12
import QtQuick.Controls 2.5

CheckBox {
	id: control

	// A custom indicator item for the checkbox.
	// It is only needed when using Qt5. For Qt6 we can rely on the built-in indicator.
	Component {
		id: indicatorComponent
		Rectangle {
			implicitWidth: 18
			implicitHeight: 18
			x: control.leftPadding
			y: parent.height / 2 - height / 2
			border.color: control.down ? "#000000" : "#444444"
			color: control.pressed ? "#F0F0F0" : "#FFFFFF"
			radius: 2

			Rectangle {
				width: 12
				height: 12
				x: 3
				y: 3
				radius: 2
				color: control.down ? "#000000" : "#444444"
				visible: control.checked
			}
		}
	}

	// Conditionally activate the custom checkbox indicator when using Qt5: 
	Component.onCompleted: {
		if(QT_VERSION < 0x060000)
			indicator = indicatorComponent;
	}
}
