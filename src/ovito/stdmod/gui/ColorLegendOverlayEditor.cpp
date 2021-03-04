////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2021 OVITO GmbH, Germany
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify it either under the
//  terms of the GNU General Public License version 3 as published by the Free Software
//  Foundation (the "GPL") or, at your option, under the terms of the MIT License.
//  If you do not alter this notice, a recipient may use your version of this
//  file under either the GPL or the MIT License.
//
//  You should have received a copy of the GPL along with this program in a
//  file LICENSE.GPL.txt.  You should have received a copy of the MIT License along
//  with this program in a file LICENSE.MIT.txt
//
//  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND,
//  either express or implied. See the GPL or the MIT License for the specific language
//  governing rights and limitations.
//
////////////////////////////////////////////////////////////////////////////////////////

#include <ovito/stdmod/gui/StdModGui.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include <ovito/gui/desktop/properties/StringParameterUI.h>
#include <ovito/gui/desktop/properties/ColorParameterUI.h>
#include <ovito/gui/desktop/properties/FontParameterUI.h>
#include <ovito/gui/desktop/properties/FloatParameterUI.h>
#include <ovito/gui/desktop/properties/Vector3ParameterUI.h>
#include <ovito/gui/desktop/properties/VariantComboBoxParameterUI.h>
#include <ovito/gui/desktop/viewport/overlays/MoveOverlayInputMode.h>
#include <ovito/gui/desktop/widgets/general/ViewportModeButton.h>
#include <ovito/gui/desktop/widgets/general/PopupUpdateComboBox.h>
#include <ovito/gui/desktop/mainwin/MainWindow.h>
#include <ovito/gui/base/actions/ViewportModeAction.h>
#include <ovito/gui/base/viewport/ViewportInputManager.h>
#include <ovito/core/dataset/scene/RootSceneNode.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/stdmod/viewport/ColorLegendOverlay.h>
#include <ovito/stdobj/properties/PropertyObject.h>
#include "ColorLegendOverlayEditor.h"

namespace Ovito { namespace StdMod {

IMPLEMENT_OVITO_CLASS(ColorLegendOverlayEditor);
SET_OVITO_OBJECT_EDITOR(ColorLegendOverlay, ColorLegendOverlayEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void ColorLegendOverlayEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	OVITO_ASSERT(QMetaType::hasRegisteredComparators<PropertyDataObjectReference>());

	// Create a rollout.
	QWidget* rollout = createRollout(tr("Color legend"), rolloutParams, "viewport_layers.color_legend.html");

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1, 1);
	int row = 0;

	_sourcesComboBox = new PopupUpdateComboBox();
	connect(this, &PropertiesEditor::contentsChanged, this, &ColorLegendOverlayEditor::updateUI);
	connect(_sourcesComboBox, &PopupUpdateComboBox::dropDownActivated, this, &ColorLegendOverlayEditor::updateSourcesList); 
	connect(_sourcesComboBox, QOverload<int>::of(&QComboBox::activated), this, &ColorLegendOverlayEditor::colorSourceSelected);
	layout->addWidget(new QLabel(tr("Color source:")), row, 0);
	layout->addWidget(_sourcesComboBox, row++, 1);

	QGroupBox* positionBox = new QGroupBox(tr("Position"));
	layout->addWidget(positionBox, row++, 0, 1, 2);
	QGridLayout* sublayout = new QGridLayout(positionBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(1, 1);
	int subrow = 0;

	VariantComboBoxParameterUI* alignmentPUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::alignment));
	sublayout->addWidget(alignmentPUI->comboBox(), subrow, 0);
	alignmentPUI->comboBox()->addItem(tr("Top"), QVariant::fromValue((int)(Qt::AlignTop | Qt::AlignHCenter)));
	alignmentPUI->comboBox()->addItem(tr("Top left"), QVariant::fromValue((int)(Qt::AlignTop | Qt::AlignLeft)));
	alignmentPUI->comboBox()->addItem(tr("Top right"), QVariant::fromValue((int)(Qt::AlignTop | Qt::AlignRight)));
	alignmentPUI->comboBox()->addItem(tr("Bottom"), QVariant::fromValue((int)(Qt::AlignBottom | Qt::AlignHCenter)));
	alignmentPUI->comboBox()->addItem(tr("Bottom left"), QVariant::fromValue((int)(Qt::AlignBottom | Qt::AlignLeft)));
	alignmentPUI->comboBox()->addItem(tr("Bottom right"), QVariant::fromValue((int)(Qt::AlignBottom | Qt::AlignRight)));
	alignmentPUI->comboBox()->addItem(tr("Left"), QVariant::fromValue((int)(Qt::AlignVCenter | Qt::AlignLeft)));
	alignmentPUI->comboBox()->addItem(tr("Right"), QVariant::fromValue((int)(Qt::AlignVCenter | Qt::AlignRight)));

	VariantComboBoxParameterUI* orientationPUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::orientation));
	sublayout->addWidget(orientationPUI->comboBox(), subrow++, 1);
	orientationPUI->comboBox()->addItem(tr("Vertical"), QVariant::fromValue((int)Qt::Vertical));
	orientationPUI->comboBox()->addItem(tr("Horizontal"), QVariant::fromValue((int)Qt::Horizontal));

	FloatParameterUI* offsetXPUI = new FloatParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::offsetX));
	sublayout->addWidget(offsetXPUI->label(), subrow, 0);
	sublayout->addLayout(offsetXPUI->createFieldLayout(), subrow++, 1);

	FloatParameterUI* offsetYPUI = new FloatParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::offsetY));
	sublayout->addWidget(offsetYPUI->label(), subrow, 0);
	sublayout->addLayout(offsetYPUI->createFieldLayout(), subrow++, 1);

	ViewportInputMode* moveOverlayMode = new MoveOverlayInputMode(this);
	connect(this, &QObject::destroyed, moveOverlayMode, &ViewportInputMode::removeMode);
	ViewportModeAction* moveOverlayAction = new ViewportModeAction(mainWindow(), tr("Move using mouse"), this, moveOverlayMode);
	sublayout->addWidget(new ViewportModeButton(moveOverlayAction), subrow++, 0, 1, 2);

	QGroupBox* sizeBox = new QGroupBox(tr("Size and border"));
	layout->addWidget(sizeBox, row++, 0, 1, 2);
	sublayout = new QGridLayout(sizeBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(1, 1);
	subrow = 0;

	FloatParameterUI* sizePUI = new FloatParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::legendSize));
	sublayout->addWidget(sizePUI->label(), subrow, 0);
	sublayout->addLayout(sizePUI->createFieldLayout(), subrow++, 1);

	FloatParameterUI* aspectRatioPUI = new FloatParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::aspectRatio));
	sublayout->addWidget(aspectRatioPUI->label(), subrow, 0);
	sublayout->addLayout(aspectRatioPUI->createFieldLayout(), subrow++, 1);

	BooleanParameterUI* borderEnabledPUI = new BooleanParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::borderEnabled));
	sublayout->addWidget(borderEnabledPUI->checkBox(), subrow, 0);
	borderEnabledPUI->checkBox()->setText(tr("Border:"));

	ColorParameterUI* borderColorPUI = new ColorParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::borderColor));
	sublayout->addWidget(borderColorPUI->colorPicker(), subrow++, 1);

	QGroupBox* labelBox = new QGroupBox(tr("Labels"));
	layout->addWidget(labelBox, row++, 0, 1, 2);
	sublayout = new QGridLayout(labelBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(1, 3);
	sublayout->setColumnStretch(2, 1);
	subrow = 0;

	StringParameterUI* titlePUI = new StringParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::title));
	sublayout->addWidget(new QLabel(tr("Custom title:")), subrow, 0);
	sublayout->addWidget(titlePUI->textBox(), subrow++, 1, 1, 2);

	StringParameterUI* label1PUI = new StringParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::label1));
	sublayout->addWidget(new QLabel(tr("Custom label 1:")), subrow, 0);
	sublayout->addWidget(label1PUI->textBox(), subrow++, 1, 1, 2);

	StringParameterUI* label2PUI = new StringParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::label2));
	sublayout->addWidget(new QLabel(tr("Custom label 2:")), subrow, 0);
	sublayout->addWidget(label2PUI->textBox(), subrow++, 1, 1, 2);

	StringParameterUI* valueFormatStringPUI = new StringParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::valueFormatString));
	sublayout->addWidget(new QLabel(tr("Format string:")), subrow, 0);
	sublayout->addWidget(valueFormatStringPUI->textBox(), subrow++, 1, 1, 2);

	FloatParameterUI* fontSizePUI = new FloatParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::fontSize));
	sublayout->addWidget(new QLabel(tr("Text size/color:")), subrow, 0);
	sublayout->addLayout(fontSizePUI->createFieldLayout(), subrow, 1);

	ColorParameterUI* textColorPUI = new ColorParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::textColor));
	sublayout->addWidget(textColorPUI->colorPicker(), subrow++, 2);

	BooleanParameterUI* outlineEnabledPUI = new BooleanParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::outlineEnabled));
	sublayout->addWidget(outlineEnabledPUI->checkBox(), subrow, 1);

	ColorParameterUI* outlineColorPUI = new ColorParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::outlineColor));
	sublayout->addWidget(outlineColorPUI->colorPicker(), subrow++, 2);

	FontParameterUI* labelFontPUI = new FontParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::font));
	sublayout->addWidget(labelFontPUI->label(), subrow, 0);
	sublayout->addWidget(labelFontPUI->fontPicker(), subrow++, 1, 1, 2);
}

/******************************************************************************
* Updates the combobox list showing the available data sources.
******************************************************************************/
void ColorLegendOverlayEditor::updateSourcesList()
{
	_sourcesComboBox->clear();
	if(ColorLegendOverlay* overlay = static_object_cast<ColorLegendOverlay>(editObject())) {

		// Find all ColorCodingModifiers in the scene. For this we have to visit all
		// pipelines and iterate over their modifier applications.		
		overlay->dataset()->sceneRoot()->visitObjectNodes([&](PipelineSceneNode* pipeline) {
			// Walk along the pipeline:
			PipelineObject* obj = pipeline->dataProvider();
			while(obj) {
				if(ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(obj)) {
					if(ColorCodingModifier* mod = dynamic_object_cast<ColorCodingModifier>(modApp->modifier())) {
						// Prepend Color Coding modifier to the front of the list.
						_sourcesComboBox->insertItem(0, mod->sourceProperty().nameWithComponent(), QVariant::fromValue(mod));
					}
					obj = modApp->input();
				}
				else break;
			}

			// Now evaluate the pipeline and look for typed properties in its output data collection.
			const PipelineFlowState& state = pipeline->evaluatePipelineSynchronous(false);
			for(const ConstDataObjectPath& dataPath : state.getObjectsRecursive(PropertyObject::OOClass())) {
				const PropertyObject* property = static_object_cast<PropertyObject>(dataPath.back());

				// Check if the property is a typed property, i.e. it has one or more ElementType objects attached to it.
				if(property->isTypedProperty() && dataPath.size() >= 2) {
					QVariant ref = QVariant::fromValue(PropertyDataObjectReference(dataPath));
					
					// Append typed properties at the end of the list.
					if(_sourcesComboBox->findData(ref) < 0)
						_sourcesComboBox->addItem(dataPath.toUIString(), std::move(ref));
				}
			}

			return true;
		});

		// Select the item in the list that corresponds to the current parameter value.
		if(overlay->modifier())
			_sourcesComboBox->setCurrentIndex(_sourcesComboBox->findData(QVariant::fromValue(overlay->modifier())));
		else if(overlay->sourceProperty())
			_sourcesComboBox->setCurrentIndex(_sourcesComboBox->findData(QVariant::fromValue(overlay->sourceProperty())));
	}
	if(_sourcesComboBox->count() == 0) 
		_sourcesComboBox->addItem(QIcon(":/gui/mainwin/status/status_warning.png"), tr("<none>"));
}

/******************************************************************************
* Is called when the user selects a new source object for the color legend.
******************************************************************************/
void ColorLegendOverlayEditor::colorSourceSelected()
{
	if(ColorLegendOverlay* overlay = static_object_cast<ColorLegendOverlay>(editObject())) {
		undoableTransaction(tr("Select color source"), [&]() {
			QVariant selectedData = static_cast<QComboBox*>(sender())->currentData();

			if(selectedData.canConvert<ColorCodingModifier*>()) {
				overlay->setModifier(selectedData.value<ColorCodingModifier*>());
				overlay->setSourceProperty({});
			}
			else if(selectedData.canConvert<PropertyDataObjectReference>()) {
				overlay->setModifier(nullptr);
				overlay->setSourceProperty(selectedData.value<PropertyDataObjectReference>());
			}
		});
	}
}

/******************************************************************************
* Updates the values displayed in the editor's widgets.
******************************************************************************/
void ColorLegendOverlayEditor::updateUI()
{
	_sourcesComboBox->clear();
	if(ColorLegendOverlay* overlay = static_object_cast<ColorLegendOverlay>(editObject())) {
		if(ColorCodingModifier* mod = overlay->modifier()) {
			_sourcesComboBox->addItem(mod->sourceProperty().nameWithComponent(), QVariant::fromValue(mod));
		}
		else if(overlay->sourceProperty()) {
			_sourcesComboBox->addItem(overlay->sourceProperty().dataTitle(), QVariant::fromValue(overlay->sourceProperty()));
		}
		else {
			_sourcesComboBox->addItem(QIcon(":/gui/mainwin/status/status_warning.png"), tr("<none>"));
		}
	}
	_sourcesComboBox->setCurrentIndex(0);
}

}	// End of namespace
}	// End of namespace
