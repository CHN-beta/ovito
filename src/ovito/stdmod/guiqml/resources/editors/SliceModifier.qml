import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.ovito
import "qrc:/gui/ui" as Ui

ColumnLayout {
	spacing: 2

	Ui.RolloutPanel {
		title: qsTr("Slice")
		helpTopicId: "manual:particles.modifiers.slice"
		Layout.fillWidth: true

		ParameterUI {
			id: normalParameterUI
			editObject: propertyEditor.editObject
			propertyName: "normalController"
		}

		ColumnLayout {
			anchors.fill: parent

			GroupBox {
				title: qsTr("Plane")
				Layout.fillWidth: true
				GridLayout {
					anchors.fill: parent
					columns: 2

					Label { text: qsTr("Distance:") }
					Ui.FloatParameter {
						propertyField: "distanceController"
						Layout.fillWidth: true 
					}

					Label { 
						text: qsTr("<a href=\"x\">Normal (X)</a>:") 
						onLinkActivated: {
							mainWindow.undoableOperation("Set plane normal", () => {
								normalParameterUI.propertyValue = Qt.vector3d(1,0,0);
							}); 
						}
					}
					Ui.Vector3Parameter {
						propertyField: "normalController"
						vectorComponent: 0
						Layout.fillWidth: true 
					}

					Label { 
						text: qsTr("<a href=\"y\">Normal (Y)</a>:") 
						onLinkActivated: {
							mainWindow.undoableOperation("Set plane normal", () => {
								normalParameterUI.propertyValue = Qt.vector3d(0,1,0);
							}); 
						}
					}
					Ui.Vector3Parameter { 
						propertyField: "normalController"
						vectorComponent: 1
						Layout.fillWidth: true 
					}

					Label { 
						text: qsTr("<a href=\"z\">Normal (Z)</a>:") 
						onLinkActivated: {
							mainWindow.undoableOperation("Set plane normal", () => {
								normalParameterUI.propertyValue = Qt.vector3d(0,0,1);
							}); 
						}
					}
					Ui.Vector3Parameter { 
						propertyField: "normalController"
						vectorComponent: 2
						Layout.fillWidth: true 
					}

					Label { text: qsTr("Slab width:") }
					Ui.FloatParameter {
						propertyField: "widthController"
						Layout.fillWidth: true 
					}
				}
			}

			GroupBox {
				title: qsTr("Options")
				Layout.fillWidth: true
				ColumnLayout {
					anchors.fill: parent

					Ui.BooleanCheckBoxParameter {
						propertyField: "inverse"
						text: qsTr("Reverse orientation")
					}

					Ui.BooleanCheckBoxParameter {
						propertyField: "createSelection"
						text: qsTr("Create selection")
					}

					Ui.BooleanCheckBoxParameter {
						propertyField: "applyToSelection"
						text: qsTr("Apply to selection only")
					}

					Ui.BooleanCheckBoxParameter {
						propertyField: "enablePlaneVisualization"
						text: qsTr("Visualize plane")
					}
				}
			}

			Button {
				Layout.fillWidth: true
				text: "Center in simulation cell"
				onClicked: {
					mainWindow.undoableOperation("Center plane in box", () => {
						propertyEditor.editObject.centerPlaneInSimulationCell(propertyEditor.parentEditObject)
					}); 
				}
			}

			Ui.ObjectStatusWidget {
				Layout.fillWidth: true
			}
		}
	}

	Ui.RolloutPanel {
		title: qsTr("Operate on")
		helpTopicId: "manual:particles.modifiers.slice"
		Layout.fillWidth: true

		ModifierDelegateFixedListParameter {
			anchors.fill: parent
		}		
	}
}