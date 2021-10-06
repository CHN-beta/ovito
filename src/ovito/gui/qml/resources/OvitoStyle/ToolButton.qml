import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import QtQuick.Controls.Universal

T.ToolButton {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: 6
    spacing: 8

    icon.width: 20
    icon.height: 20
    icon.color: Color.transparent(Universal.foreground, enabled ? 1.0 : 0.2)

    property bool useSystemFocusVisuals: true

    contentItem: IconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display

        icon: control.icon
        text: control.text
        font: control.font
        color: Color.transparent(control.Universal.foreground, enabled ? 1.0 : 0.2)
    }

    background: Rectangle {
        implicitWidth: 40
        implicitHeight: 40

        color: control.enabled && (control.highlighted || control.checked) ? "lightsteelblue" : "transparent"

        Rectangle {
            width: parent.width
            height: parent.height
            visible: control.down || control.hovered
            color: control.down ? control.Universal.listMediumColor : control.Universal.listLowColor
        }
    }
}