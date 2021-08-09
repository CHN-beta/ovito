import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("Cluster analysis")
	
	ColumnLayout {
		anchors.fill: parent

		Label { text: qsTr("Neighbor mode:") }

		GridLayout {
			columns: 2
			Layout.fillWidth: true
			Layout.leftMargin: 10

			Ui.IntegerRadioButtonParameter { 
				propertyField: "neighborMode"
				value: 0
				text: qsTr("Cutoff distance:")
			}

			Ui.FloatParameter { 
				propertyField: "cutoff"
				Layout.fillWidth: true 
			}

			Ui.IntegerRadioButtonParameter { 
				propertyField: "neighborMode"
				value: 1
				text: qsTr("Bonds")
			}
		}

		Ui.BooleanCheckBoxParameter { 
			propertyField: "sortBySize"
		}
		Ui.BooleanCheckBoxParameter { 
			propertyField: "computeCentersOfMass"
		}
		Ui.BooleanCheckBoxParameter { 
			propertyField: "unwrapParticleCoordinates"
		}
		Ui.BooleanCheckBoxParameter { 
			propertyField: "colorParticlesByCluster"
		}
		Ui.BooleanCheckBoxParameter { 
			propertyField: "onlySelectedParticles"
		}
	}
}