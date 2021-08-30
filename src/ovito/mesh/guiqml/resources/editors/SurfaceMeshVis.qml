import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.ovito

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("Surface mesh")
	helpTopicId: "manual:visual_elements.surface_mesh"
	
	ColumnLayout {
		anchors.fill: parent

		GroupBox {
			title: qsTr("Surface")
			Layout.fillWidth: true 

			GridLayout {
				anchors.fill: parent
				columns: 2

				Ui.ParameterLabel { propertyField: "surfaceColor" }
				Ui.ColorParameter {
					propertyField: "surfaceColor" // PROPERTY_FIELD(SurfaceMeshVis::surfaceColor)
					Layout.fillWidth: true 
				}

				Ui.ParameterLabel { propertyField: "surfaceTransparencyController" }
				Ui.FloatParameter { 
					propertyField: "surfaceTransparencyController" // PROPERTY_FIELD(SurfaceMeshVis::surfaceTransparencyController)
					Layout.fillWidth: true 
				}

				Ui.BooleanCheckBoxParameter { 
					propertyField: "smoothShading" // PROPERTY_FIELD(SurfaceMeshVis::smoothShading)
					Layout.columnSpan: 2
				}
				Ui.BooleanCheckBoxParameter { 
					propertyField: "reverseOrientation" // PROPERTY_FIELD(SurfaceMeshVis::reverseOrientation)
					Layout.columnSpan: 2
				}
				Ui.BooleanCheckBoxParameter { 
					propertyField: "highlightEdges" // PROPERTY_FIELD(SurfaceMeshVis::highlightEdges)
					Layout.columnSpan: 2
				}
			}
		}

		GroupBox {
			title: qsTr("Cap polygons")
			Layout.fillWidth: true 
			ParameterUI on visible {
				propertyName: "surfaceIsClosed" // PROPERTY_FIELD(SurfaceMeshVis::surfaceIsClosed)
				editObject: propertyEditor.editObject
			}

			label: Ui.BooleanCheckBoxParameter { 
				id: showCapPolygons
				propertyField: "showCap" // PROPERTY_FIELD(SurfaceMeshVis::showCap)
			}

			GridLayout {
				anchors.fill: parent
				enabled: showCapPolygons.checked
				columns: 2

				Ui.ParameterLabel { propertyField: "capColor" }
				Ui.ColorParameter {
					propertyField: "capColor" // PROPERTY_FIELD(SurfaceMeshVis::capColor)
					Layout.fillWidth: true 
				}

				Ui.ParameterLabel { propertyField: "capTransparencyController" }
				Ui.FloatParameter { 
					propertyField: "capTransparencyController" // PROPERTY_FIELD(SurfaceMeshVis::capTransparencyController)
					Layout.fillWidth: true 
				}		
			}
		}
	}
}