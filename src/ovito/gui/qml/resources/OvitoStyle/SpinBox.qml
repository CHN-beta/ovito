import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import QtQuick.Controls.Universal

T.SpinBox {
    id: control

	property alias placeholderText: textField.placeholderText

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentItem.implicitWidth + 2 * padding +
                            Math.max(up.implicitIndicatorWidth,
                                     down.implicitIndicatorWidth))
    implicitHeight: Math.max(implicitContentHeight + topPadding + bottomPadding,
                             implicitBackgroundHeight,
                             up.implicitIndicatorHeight +
                             down.implicitIndicatorHeight)

    Universal.theme: activeFocus ? Universal.Light : undefined

    padding: 12
    topPadding: padding - 7
    bottomPadding: padding - 5
    rightPadding: padding + (control.mirrored ? (down.indicator ? down.indicator.width : 0) : (up.indicator ? up.indicator.width : 0))

    validator: IntValidator {
        locale: control.locale.name
        bottom: Math.min(control.from, control.to)
        top: Math.max(control.from, control.to)
    }

    contentItem: TextField {
        id: textField
        z: 2
//        text: control.displayText

        font: control.font
        color: control.palette.text
        selectionColor: control.palette.highlight
        selectedTextColor: control.palette.highlightedText
        verticalAlignment: Qt.AlignVCenter

        padding: 0
        topPadding: 0
        rightPadding: 0
        bottomPadding: 0
        background: null

        readOnly: !control.editable
        validator: control.validator
        inputMethodHints: control.inputMethodHints
    }

    up.indicator: PaddedRectangle {
        x: control.mirrored ? 1 : parent.width - width - 1
        y: 1
        height: parent.height / 2 - 1
        implicitWidth: 16
        implicitHeight: 10

        clip: true
        topPadding: -2
        leftPadding: -2
        color: control.up.pressed ? control.palette.mid : "transparent"

        ColorImage {
            scale: -1
            width: parent.width
            height: parent.height
            opacity: enabled ? 1.0 : 0.5
            color: control.palette.buttonText
            source: "qrc:/gui/OvitoStyle/images/arrow.png"
            fillMode: Image.Pad
        }
    }

    down.indicator: PaddedRectangle {
        x: control.mirrored ? 1 : parent.width - width - 1
        y: parent.height - height - 1
        height: parent.height / 2 - 1
        implicitWidth: 16
        implicitHeight: 10

        clip: true
        topPadding: -2
        leftPadding: -2
        color: control.down.pressed ? control.palette.mid : "transparent"

        ColorImage {
            width: parent.width
            height: parent.height
            opacity: enabled ? 1.0 : 0.5
            color: control.palette.buttonText
            source: "qrc:/gui/OvitoStyle/images/arrow.png"
            fillMode: Image.Pad
        }
    }

    background: Rectangle {
        implicitWidth: 60
        implicitHeight: 24

        border.width: 2 // TextControlBorderThemeThickness
        border.color: !control.enabled ? control.Universal.baseLowColor :
                       control.activeFocus ? control.Universal.accent :
                       control.hovered ? control.Universal.baseMediumColor : control.Universal.chromeDisabledLowColor
        color: control.enabled ? control.Universal.background : control.Universal.baseLowColor
    }
}
