import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("Construct surface mesh")
	helpTopicId: "manual:particles.modifiers.construct_surface_mesh"
	
	ColumnLayout {
		anchors.fill: parent

		GroupBox {
			title: qsTr("Method")
			Layout.fillWidth: true 

			GridLayout {
				id: layout
				anchors.fill: parent
				columns: 2
				property int inset: 30

				Ui.IntegerRadioButtonParameter { 
					id: alphaShapeMethod
					Layout.columnSpan: 2
					propertyField: "method" // PROPERTY_FIELD(ConstructSurfaceModifier::method)
					value: 0 // ConstructSurfaceModifier::AlphaShape
					text: qsTr("Alpha-shape method (default):")
				}

				Ui.ParameterLabel { Layout.leftMargin: layout.inset; propertyField: "probeSphereRadius" }
				Ui.FloatParameter {
					propertyField: "probeSphereRadius" // PROPERTY_FIELD(ConstructSurfaceModifier::probeSphereRadius)
					Layout.fillWidth: true
					enabled: alphaShapeMethod.checked
				}

				Ui.ParameterLabel { Layout.leftMargin: layout.inset; propertyField: "smoothingLevel" }
				Ui.IntegerParameter {
					propertyField: "smoothingLevel" // PROPERTY_FIELD(ConstructSurfaceModifier::smoothingLevel)
					Layout.fillWidth: true
					enabled: alphaShapeMethod.checked
				}

				Ui.BooleanCheckBoxParameter { 
					Layout.columnSpan: 2
					Layout.leftMargin: layout.inset
					propertyField: "selectSurfaceParticles" // PROPERTY_FIELD(ConstructSurfaceModifier::selectSurfaceParticles)
					enabled: alphaShapeMethod.checked
				}

				Ui.BooleanCheckBoxParameter { 
					Layout.columnSpan: 2
					Layout.leftMargin: layout.inset
					propertyField: "identifyRegions" // PROPERTY_FIELD(ConstructSurfaceModifier::identifyRegions)
					enabled: alphaShapeMethod.checked
				}

				Ui.IntegerRadioButtonParameter { 
					id: gaussianDensityMethod
					Layout.columnSpan: 2
					propertyField: "method" // PROPERTY_FIELD(ConstructSurfaceModifier::method)
					value: 1 // ConstructSurfaceModifier::GaussianDensity
					text: qsTr("Gaussian density method:")
				}

				Ui.ParameterLabel { Layout.leftMargin: layout.inset; propertyField: "gridResolution" }
				Ui.IntegerParameter {
					propertyField: "gridResolution" // PROPERTY_FIELD(ConstructSurfaceModifier::gridResolution)
					Layout.fillWidth: true
					enabled: gaussianDensityMethod.checked
				}

				Ui.ParameterLabel { Layout.leftMargin: layout.inset; propertyField: "radiusFactor" }
				Ui.FloatParameter {
					propertyField: "radiusFactor" // PROPERTY_FIELD(ConstructSurfaceModifier::radiusFactor)
					Layout.fillWidth: true
					enabled: gaussianDensityMethod.checked
				}

				Ui.ParameterLabel { Layout.leftMargin: layout.inset; propertyField: "isoValue" }
				Ui.FloatParameter {
					propertyField: "isoValue" // PROPERTY_FIELD(ConstructSurfaceModifier::isoValue)
					Layout.fillWidth: true
					enabled: gaussianDensityMethod.checked
				}
			}
		}

		GroupBox {
			title: qsTr("Options")
			Layout.fillWidth: true

			ColumnLayout {
				anchors.fill: parent

				Ui.BooleanCheckBoxParameter { 
					propertyField: "onlySelectedParticles" // PROPERTY_FIELD(ConstructSurfaceModifier::onlySelectedParticles)
				}
				Ui.BooleanCheckBoxParameter { 
					propertyField: "transferParticleProperties" // PROPERTY_FIELD(ConstructSurfaceModifier::transferParticleProperties)
				}
				Ui.BooleanCheckBoxParameter { 
					propertyField: "computeSurfaceDistance" // PROPERTY_FIELD(ConstructSurfaceModifier::computeSurfaceDistance)
				}
			}
		}

		Ui.ObjectStatusWidget {
			Layout.fillWidth: true
		}
	}
}