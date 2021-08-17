import QtQuick
import QtQuick.Controls
import org.ovito

Control {
	id: control
	property alias propertyField: parameterUI.propertyName
	property alias delegate: listView.delegate
	property alias header: listView.header

	property RefTarget selectedObject: listView.currentIndex >= 0 ? parameterUI.objectAtIndex(listView.currentIndex) : null

	implicitHeight: 150
	hoverEnabled: true
	padding: background.border.width

	RefTargetListParameterUI {
		id: parameterUI
		editObject: propertyEditor.editObject
	}

	contentItem: ScrollView {	
		id: scrollView
		clip: true
		rightPadding: ScrollBar.vertical.visible ? ScrollBar.vertical.width : 0

		// Avoid horizontal scrollbar:
		ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

		ListView {
			id: listView
			model: parameterUI.model
			highlight: Rectangle { color: "lightsteelblue"; }
			highlightMoveDuration: 0
			highlightMoveVelocity: -1
			headerPositioning: ListView.OverlayHeader
		}

		ScrollBar.vertical: ScrollBar {
			parent: scrollView
			x: scrollView.mirrored ? scrollView.leftPadding : scrollView.width - scrollView.rightPadding
			y: scrollView.topPadding
			height: scrollView.availableHeight
			policy: ScrollBar.AlwaysOn
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