import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import QtQuick.Controls.Universal
import org.ovito

T.SplitView {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    // Customized:
    MouseGrabWorkaround { 
        id: mouseGrabWorkaround
        container: control
    }

    handle: Rectangle {
        id: splitHandle
        implicitWidth: control.orientation === Qt.Horizontal ? 5 : control.width
        implicitHeight: control.orientation === Qt.Horizontal ? control.height : 5
        color: T.SplitHandle.pressed ? control.Universal.chromeDisabledLowColor
            : (T.SplitHandle.hovered ? control.Universal.baseMediumLowColor : control.Universal.chromeHighColor)

        // Customized:
        SplitHandle.onPressedChanged: {
            mouseGrabWorkaround.setActive(SplitHandle.pressed, splitHandle)
        }
    }
}
