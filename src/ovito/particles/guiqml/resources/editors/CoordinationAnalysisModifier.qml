import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("Coordination analysis")
	helpTopicId: "manual:particles.modifiers.coordination_analysis"
	
	GridLayout {
		anchors.fill: parent
		columns: 2

		Ui.ParameterLabel { propertyField: "cutoff" }
		Ui.FloatParameter { 
			propertyField: "cutoff" // PROPERTY_FIELD(CoordinationAnalysisModifier::cutoff)
			Layout.fillWidth: true 
		}

		Ui.ParameterLabel { propertyField: "numberOfBins" }
		Ui.IntegerParameter { 
			propertyField: "numberOfBins" // PROPERTY_FIELD(CoordinationAnalysisModifier::numberOfBins)
			Layout.fillWidth: true 
		}

		Ui.BooleanCheckBoxParameter {
			propertyField: "computePartialRDF" // PROPERTY_FIELD(CoordinationAnalysisModifier::computePartialRDF)
			Layout.columnSpan: 2
		}

		Ui.BooleanCheckBoxParameter {
			propertyField: "onlySelected" // PROPERTY_FIELD(CoordinationAnalysisModifier::onlySelected)
			Layout.columnSpan: 2
		}

		Ui.ObjectStatusWidget {
			Layout.fillWidth: true
			Layout.columnSpan: 2
		}
	}
}