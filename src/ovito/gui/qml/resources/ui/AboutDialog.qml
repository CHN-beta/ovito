import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
	modal: true
	anchors.centerIn: parent
	standardButtons: Dialog.Close
	implicitWidth: 450
	implicitHeight: 420

	onAboutToShow: { 
		systemReportLabel.text = "";
		stackLayout.currentIndex = 0; 
	}

	StackLayout {
		id: stackLayout
		anchors.fill: parent
		implicitWidth: aboutMessage.implicitWidth
		implicitHeight: aboutMessage.implicitHeight

		Item {
			Image {
				id: image
				source: "qrc:/guibase/mainwin/window_icon_48.png"
				anchors.top: parent.top
				anchors.left: parent.left
				anchors.topMargin: 10
			}

			Text {
				id: aboutMessage
				textFormat: Text.StyledText
				wrapMode: Text.WordWrap
				anchors.top: image.bottom
				anchors.left: parent.left
				anchors.right: parent.right
				anchors.bottom: parent.bottom
				anchors.topMargin: 10

				text: 
					qsTr("<h3>Ovito (Open Visualization Tool)</h3><p></p><p>Version %1</p>").arg(Qt.application.version)
					+ qsTr("<p>A scientific data visualization and analysis software for particle simulations.</p><p></p><p>Copyright (C) 2021, OVITO GmbH</p><p>This is free, open-source software, and you are welcome to redistribute it according to the terms of the GNU General Public License (v3).</p>")
					+ qsTr("<p></p><p><i>This is an early technology demo of a WebAssembly port of the OVITO code base. If you have any comments or questions, or would like to get involved in the development of this web-based version of OVITO, please <a href=\"https://www.ovito.org/contact/\">get in touch with us</a>.</i></p>")
					+ qsTr("<p></p><p><i>Source code is available at <a href=\"https://gitlab.com/stuko/ovito\">gitlab.com/stuko/ovito</a>.</i></p>")

				onLinkActivated: function (link) {
					Qt.openUrlExternally(link);
				}
			}

			Text {
				textFormat: Text.StyledText
				text: "<a href=\"#\">System information</a>"
				anchors.right: parent.right
				anchors.top: parent.top

				onLinkActivated: function (link) {
					systemReportLabel.text = mainWindow.systemReport
					stackLayout.currentIndex = 1;
				}
			}
		}

		ScrollView {
			TextArea {
				id: systemReportLabel
				wrapMode: TextArea.Wrap
				readOnly: true
			}
		}
	}
}
