import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("Unwrap trajectories")
	helpTopicId: "manual:particles.modifiers.unwrap_trajectories"
	
	ColumnLayout {
		anchors.fill: parent

		Ui.ObjectStatusWidget {
			Layout.fillWidth: true
		}
	}
}