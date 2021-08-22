import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.ovito

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("Delete selected")
	helpTopicId: "manual:particles.modifiers.delete_selected_particles"

	ColumnLayout {
		anchors.fill: parent
		spacing: 2

		ModifierDelegateFixedListParameter {
			Layout.fillWidth: true
		}

		Ui.ObjectStatusWidget {
			Layout.fillWidth: true
		}		
	}
}
