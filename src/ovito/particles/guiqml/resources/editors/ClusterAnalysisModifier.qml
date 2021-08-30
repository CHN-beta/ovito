import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("Cluster analysis")
	helpTopicId: "manual:particles.modifiers.cluster_analysis"
	
	ColumnLayout {
		anchors.fill: parent

		Label { text: qsTr("Neighbor mode:") }

		GridLayout {
			columns: 2
			Layout.fillWidth: true
			Layout.leftMargin: 10

			Ui.IntegerRadioButtonParameter { 
				propertyField: "neighborMode" // PROPERTY_FIELD(ClusterAnalysisModifier::neighborMode)
				value: 0 // ClusterAnalysisModifier::CutoffRange
				text: qsTr("Cutoff distance:")
			}

			Ui.FloatParameter { 
				propertyField: "cutoff" // PROPERTY_FIELD(ClusterAnalysisModifier::cutoff)
				Layout.fillWidth: true 
			}

			Ui.IntegerRadioButtonParameter { 
				propertyField: "neighborMode" // PROPERTY_FIELD(ClusterAnalysisModifier::neighborMode)
				value: 1 // ClusterAnalysisModifier::Bonding
				text: qsTr("Bonds")
			}
		}

		Ui.BooleanCheckBoxParameter { 
			propertyField: "sortBySize" // PROPERTY_FIELD(ClusterAnalysisModifier::sortBySize)
		}
		Ui.BooleanCheckBoxParameter { 
			propertyField: "computeCentersOfMass" // PROPERTY_FIELD(ClusterAnalysisModifier::computeCentersOfMass)
		}
		Ui.BooleanCheckBoxParameter { 
			propertyField: "computeRadiusOfGyration" // PROPERTY_FIELD(ClusterAnalysisModifier::computeRadiusOfGyration)
		}
		Ui.BooleanCheckBoxParameter { 
			propertyField: "unwrapParticleCoordinates" // PROPERTY_FIELD(ClusterAnalysisModifier::unwrapParticleCoordinates)
		}
		Ui.BooleanCheckBoxParameter { 
			propertyField: "colorParticlesByCluster" // PROPERTY_FIELD(ClusterAnalysisModifier::colorParticlesByCluster)
		}
		Ui.BooleanCheckBoxParameter { 
			propertyField: "onlySelectedParticles" // PROPERTY_FIELD(ClusterAnalysisModifier::onlySelectedParticles)
		}

		Ui.ObjectStatusWidget {
			Layout.fillWidth: true
		}				
	}
}