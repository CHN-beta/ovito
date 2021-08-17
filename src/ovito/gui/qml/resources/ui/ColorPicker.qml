import QtQuick
import QtQuick.Controls

AbstractButton {
    id: control
    property alias color: rect.color

    Universal.theme: activeFocus ? Universal.Light : undefined

	signal colorModified()   //< interactive change only, NOT emitted if \e color property is set directly)

    contentItem: Rectangle {
        id: rect
        implicitWidth: 64
        implicitHeight: 32

        border.width: 2
        border.color: !control.enabled ? control.Universal.baseLowColor :
                       control.activeFocus ? control.Universal.accent :
                       control.hovered ? control.Universal.baseMediumColor : control.Universal.chromeDisabledLowColor
    }

    onClicked: {
        colorPickerPopup.setColor(color);
        popup.open();
    }

    Popup {
        id: popup

        parent: Overlay.overlay

        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        ColorPickerPopup {
            id: colorPickerPopup
            onColorValueChanged: {
                if(popup.opened) {
                    control.color = colorValue;
                    colorModified();
                }
            }
        }
    }
}