import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.ovito

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("Clear selection")
	helpTopicId: "manual:particles.modifiers.clear_selection"

	GridLayout {
		anchors.fill: parent
		columns: 2

		Label { text: qsTr("Operate on:") }
		Ui.DataObjectReferenceParameter {
			id: operateOn
			dataObjectType: "PropertyContainer"
			propertyField: "subject" // PROPERTY_FIELD(GenericPropertyModifier::subject)
			Layout.fillWidth: true
		}
	}
}
