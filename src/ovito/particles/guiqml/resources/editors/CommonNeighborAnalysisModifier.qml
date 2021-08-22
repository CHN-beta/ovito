import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("Common neighbor analysis")
	helpTopicId: "manual:particles.modifiers.common_neighbor_analysis"
	
	ColumnLayout {
		anchors.fill: parent

		GroupBox {
			title: qsTr("Algorithm")
			Layout.fillWidth: true

			ColumnLayout {
				anchors.fill: parent
				spacing: 0

				Ui.IntegerRadioButtonParameter { 
					propertyField: "mode"
					value: 4 // CommonNeighborAnalysisModifier::BondMode
					text: qsTr("Bond-based CNA (without cutoff)")
				}

				Ui.IntegerRadioButtonParameter { 
					propertyField: "mode"
					value: 1 // CommonNeighborAnalysisModifier::AdaptiveCutoffMode
					text: qsTr("Adaptive CNA (variable cutoff)")
				}

				Ui.IntegerRadioButtonParameter { 
					propertyField: "mode"
					value: 2 // CommonNeighborAnalysisModifier::IntervalCutoffMode
					text: qsTr("Interval CNA (variable cutoff)")
				}

				Ui.IntegerRadioButtonParameter { 
					id: conventionalCNAOption
					propertyField: "mode"
					value: 0 // CommonNeighborAnalysisModifier::FixedCutoffMode
					text: qsTr("Conventional CNA (fixed cutoff)")
				}

				RowLayout {
					spacing: 2
					Layout.fillWidth: true
					Layout.leftMargin: 30

					Ui.ParameterLabel { propertyField: "cutoff" }
					Ui.FloatParameter { 
						propertyField: "cutoff"
						Layout.fillWidth: true 
						enabled: conventionalCNAOption.checked
					}
				}
			}
		}

		GroupBox {
			title: qsTr("Options")
			Layout.fillWidth: true

			ColumnLayout {
				anchors.fill: parent

				Ui.BooleanCheckBoxParameter { 
					propertyField: "onlySelectedParticles"
				}
				Ui.BooleanCheckBoxParameter { 
					propertyField: "colorByType"
				}
			}
		}

		StructureListParameter {
			id: elementTypeList
			Layout.fillWidth: true
			implicitHeight: 250
		}

		Ui.ObjectStatusWidget {
			Layout.fillWidth: true
		}		
	}
}