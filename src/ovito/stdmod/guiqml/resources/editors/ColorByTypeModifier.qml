import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.qmlmodels 1.0
import org.ovito

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("Color by type")
	helpTopicId: "manual:particles.modifiers.color_by_type"

	// Update the displayed the list of types.
	Connections {
		target: propertyEditor
		function onModifierInputChanged() {
			if(propertyEditor.editObject)
				control.model.rows = propertyEditor.editObject.getElementTypesFromInputState(propertyEditor.parentEditObject);
		}
		function onEditObjectModified() { onModifierInputChanged(); } // Update table whenever the modifier changes.
		Component.onCompleted: onModifierInputChanged() // First time initialization when editor is loaded.
	}

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

		Label { text: qsTr("Property:") }
		Ui.PropertyReferenceParameter {
			propertyContainer: operateOn.selectedDataObject
			propertyField: "sourceProperty" // PROPERTY_FIELD(ColorByTypeModifier::sourceProperty)
			acceptablePropertyType: PropertyReferenceParameterUI.OnlyTypedProperties
			componentsMode: PropertyReferenceParameterUI.ShowNoComponents
			Layout.fillWidth: true
		}

		Ui.BooleanCheckBoxParameter { 
			id: colorOnlySelected
			propertyField: "colorOnlySelected" // PROPERTY_FIELD(ColorByTypeModifier::colorOnlySelected)
			Layout.columnSpan: 2
		}

		Ui.BooleanCheckBoxParameter { 
			propertyField: "clearSelection" // PROPERTY_FIELD(ColorByTypeModifier::clearSelection)
			Layout.columnSpan: 2
			enabled: colorOnlySelected.checked
		}

		Label { 
			text: qsTr("Types:") 
			Layout.fillWidth: true
			Layout.columnSpan: 2
		}
		Ui.TableViewWithHeader {
			id: control
			Layout.fillWidth: true
			Layout.columnSpan: 2
			columnList: ["Name", "Id"]
			columnWidths: [80,20]

			model: TableModel {
				TableModelColumn { display: "name"; decoration: "color"; }
				TableModelColumn { display: "id" }
			}

			delegate: DelegateChooser {
				DelegateChoice { 
					column: 0
					RowLayout {
						spacing: 2
						// Type color
						Rectangle {
							implicitWidth: 16
							implicitHeight: 16
							color: model.decoration
							Layout.margins: 4
						}
						// Type name
						Text { 
							Layout.fillWidth: true
							text: model.display 
							elide: Text.ElideRight
							horizontalAlignment: Text.AlignLeft
							verticalAlignment: Text.AlignVCenter
							padding: control.margins
						} 
					}
				}
				DelegateChoice { 
					column: 1
					// Type ID
					Text { 
						text: model.display 
						elide: Text.ElideRight
						horizontalAlignment: Text.AlignLeft
						verticalAlignment: Text.AlignVCenter
						padding: control.margins
					}
				}
			}			
		}

		Ui.ObjectStatusWidget {
			Layout.fillWidth: true
			Layout.columnSpan: 2
		}
	}
}
