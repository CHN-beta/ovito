import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.ovito
import "qrc:/gui/ui" as Ui

Ui.RefTargetListParameter {
    propertyField: "delegates" // PROPERTY_FIELD(MultiDelegatingModifier::delegates)

    delegate: MouseArea {
        id: mouseArea
        width: ListView.view.width
        height: layout.implicitHeight + layout.anchors.topMargin + layout.anchors.bottomMargin
        onClicked: {
            mouseArea.ListView.view.currentIndex = index
            mouseArea.forceActiveFocus()
        }
        RowLayout {
            id: layout
            anchors.fill: parent
            anchors.margins: 4
            spacing: 5
            CheckBox {
                padding: 0
                checked: reftarget.enabled
                onToggled: {
                    mainWindow.undoableOperation("Enable/disable object type", () => {
                        model.reftarget.enabled = checked;
                    }); 
                }
            }
            Text {
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: reftarget.objectTitle
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
}	