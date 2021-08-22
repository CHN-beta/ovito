import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("Voronoi analysis")
	helpTopicId: "manual:particles.modifiers.voronoi_analysis"
	
	GridLayout {
		anchors.fill: parent
		columns: 2

		Label { text: qsTr("Absolute face area threshold:") }
		Ui.FloatParameter { 
			propertyField: "faceThreshold"
			standardValue: 0.0
			placeholderText: qsTr("‹none›")
			Layout.fillWidth: true 
		}

		Label { text: qsTr("Relative face area threshold:") }
		Ui.FloatParameter { 
			propertyField: "relativeFaceThreshold"
			standardValue: 0.0
			placeholderText: qsTr("‹none›")
			Layout.fillWidth: true 
		}

		Ui.BooleanCheckBoxParameter {
			id: computeIndicesUI
			propertyField: "computeIndices"
			Layout.columnSpan: 2
		}

		Label { text: qsTr("Edge length threshold:") }
		Ui.FloatParameter { 
			enabled: computeIndicesUI.checked
			propertyField: "edgeThreshold"
			standardValue: 0.0
			placeholderText: qsTr("‹none›")
			Layout.fillWidth: true 
		}

		Ui.BooleanCheckBoxParameter { 
			propertyField: "computeBonds"
			Layout.columnSpan: 2
		}

		Ui.BooleanCheckBoxParameter { 
			propertyField: "computePolyhedra"
			Layout.columnSpan: 2
		}

		Ui.BooleanCheckBoxParameter { 
			propertyField: "useRadii"
			Layout.columnSpan: 2
		}

		Ui.BooleanCheckBoxParameter { 
			propertyField: "onlySelected"
			Layout.columnSpan: 2
		}

		Ui.ObjectStatusWidget {
			Layout.fillWidth: true
			Layout.columnSpan: 2
		}
	}
}