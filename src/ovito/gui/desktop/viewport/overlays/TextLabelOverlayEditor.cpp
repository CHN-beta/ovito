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

#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/desktop/properties/StringParameterUI.h>
#include <ovito/gui/desktop/properties/ColorParameterUI.h>
#include <ovito/gui/desktop/properties/FontParameterUI.h>
#include <ovito/gui/desktop/properties/FloatParameterUI.h>
#include <ovito/gui/desktop/properties/VariantComboBoxParameterUI.h>
#include <ovito/gui/desktop/properties/CustomParameterUI.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include <ovito/gui/desktop/viewport/overlays/MoveOverlayInputMode.h>
#include <ovito/gui/desktop/widgets/general/AutocompleteTextEdit.h>
#include <ovito/gui/desktop/widgets/general/ViewportModeButton.h>
#include <ovito/gui/desktop/widgets/general/PopupUpdateComboBox.h>
#include <ovito/gui/desktop/mainwin/MainWindow.h>
#include <ovito/gui/base/actions/ViewportModeAction.h>
#include <ovito/core/viewport/overlays/TextLabelOverlay.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/dataset/scene/RootSceneNode.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include "TextLabelOverlayEditor.h"

namespace Ovito {

IMPLEMENT_OVITO_CLASS(TextLabelOverlayEditor);
DEFINE_REFERENCE_FIELD(TextLabelOverlayEditor, sourceNode);
SET_OVITO_OBJECT_EDITOR(TextLabelOverlay, TextLabelOverlayEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void TextLabelOverlayEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Text label"), rolloutParams, "manual:viewport_layers.text_label");

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1, 3);
	layout->setColumnStretch(2, 1);
	int row = 0;

	_nodeComboBox = new PopupUpdateComboBox();
	connect(_nodeComboBox, &PopupUpdateComboBox::dropDownActivated, this, &TextLabelOverlayEditor::updateSourcesList); 

	CustomParameterUI* sourcePUI = new CustomParameterUI(this, "sourceNode", _nodeComboBox,
			// updateWidgetFunction:
			[this](const QVariant& value) {
				_nodeComboBox->clear();
				PipelineSceneNode* node = dynamic_object_cast<PipelineSceneNode>(value.value<PipelineSceneNode*>());
				if(node) {
					_nodeComboBox->addItem(node->objectTitle(), QVariant::fromValue(node));
				}
				else {
					_nodeComboBox->addItem(tr("<none>"));
				}
				_nodeComboBox->setCurrentIndex(0);
			},
			// updatePropertyFunction:
			[this]() {
				return _nodeComboBox->currentData();
			},
			// resetUIFunction:
			{});
	connect(_nodeComboBox, QOverload<int>::of(&QComboBox::activated), sourcePUI, &CustomParameterUI::updatePropertyValue);
	layout->addWidget(new QLabel(tr("Data source:")), row, 0);
	layout->addWidget(sourcePUI->widget(), row++, 1, 1, 2);

	// Label text.
	StringParameterUI* labelTextPUI = new StringParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::labelText));
	layout->addWidget(new QLabel(tr("Text:")), row, 0);
	_textEdit = new AutocompleteTextEdit();
	labelTextPUI->setTextBox(_textEdit);
	layout->addWidget(labelTextPUI->textBox(), row++, 1, 1, 2);

	StringParameterUI* valueFormatStringPUI = new StringParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::valueFormatString));
	layout->addWidget(new QLabel(tr("Value format string:")), row, 0);
	layout->addWidget(valueFormatStringPUI->textBox(), row++, 1, 1, 2);

	VariantComboBoxParameterUI* alignmentPUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::alignment));
	layout->addWidget(new QLabel(tr("Position:")), row, 0);
	layout->addWidget(alignmentPUI->comboBox(), row++, 1);
	alignmentPUI->comboBox()->addItem(tr("Top"), QVariant::fromValue((int)(Qt::AlignTop | Qt::AlignHCenter)));
	alignmentPUI->comboBox()->addItem(tr("Top left"), QVariant::fromValue((int)(Qt::AlignTop | Qt::AlignLeft)));
	alignmentPUI->comboBox()->addItem(tr("Top right"), QVariant::fromValue((int)(Qt::AlignTop | Qt::AlignRight)));
	alignmentPUI->comboBox()->addItem(tr("Bottom"), QVariant::fromValue((int)(Qt::AlignBottom | Qt::AlignHCenter)));
	alignmentPUI->comboBox()->addItem(tr("Bottom left"), QVariant::fromValue((int)(Qt::AlignBottom | Qt::AlignLeft)));
	alignmentPUI->comboBox()->addItem(tr("Bottom right"), QVariant::fromValue((int)(Qt::AlignBottom | Qt::AlignRight)));
	alignmentPUI->comboBox()->addItem(tr("Left"), QVariant::fromValue((int)(Qt::AlignVCenter | Qt::AlignLeft)));
	alignmentPUI->comboBox()->addItem(tr("Right"), QVariant::fromValue((int)(Qt::AlignVCenter | Qt::AlignRight)));

	FloatParameterUI* offsetXPUI = new FloatParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::offsetX));
	layout->addWidget(offsetXPUI->label(), row, 0);
	layout->addLayout(offsetXPUI->createFieldLayout(), row++, 1, 1, 2);

	FloatParameterUI* offsetYPUI = new FloatParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::offsetY));
	layout->addWidget(offsetYPUI->label(), row, 0);
	layout->addLayout(offsetYPUI->createFieldLayout(), row++, 1, 1, 2);

	ViewportInputMode* moveOverlayMode = new MoveOverlayInputMode(this);
	connect(this, &QObject::destroyed, moveOverlayMode, &ViewportInputMode::removeMode);
	ViewportModeAction* moveOverlayAction = new ViewportModeAction(mainWindow(), tr("Move using mouse"), this, moveOverlayMode);
	layout->addWidget(new ViewportModeButton(moveOverlayAction), row++, 1, 1, 2);

	FloatParameterUI* fontSizePUI = new FloatParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::fontSize));
	layout->addWidget(new QLabel(tr("Text size/color:")), row, 0);
	layout->addLayout(fontSizePUI->createFieldLayout(), row, 1);

	// Text color.
	ColorParameterUI* textColorPUI = new ColorParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::textColor));
	layout->addWidget(textColorPUI->colorPicker(), row++, 2);

	BooleanParameterUI* outlineEnabledPUI = new BooleanParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::outlineEnabled));
	layout->addWidget(outlineEnabledPUI->checkBox(), row, 1);

	ColorParameterUI* outlineColorPUI = new ColorParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::outlineColor));
	layout->addWidget(outlineColorPUI->colorPicker(), row++, 2);

	FontParameterUI* labelFontPUI = new FontParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::font));
	layout->addWidget(labelFontPUI->label(), row, 0);
	layout->addWidget(labelFontPUI->fontPicker(), row++, 1, 1, 2);

	QWidget* variablesRollout = createRollout(tr("Variables"), rolloutParams.after(rollout), "manual:viewport_layers.text_label");
    QVBoxLayout* variablesLayout = new QVBoxLayout(variablesRollout);
    variablesLayout->setContentsMargins(4,4,4,4);
    _attributeNamesList = new QLabel();
    _attributeNamesList->setWordWrap(true);
    _attributeNamesList->setTextInteractionFlags(Qt::TextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard | Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard));
	variablesLayout->addWidget(_attributeNamesList);

	// Update input variables list if another overlay is loaded into the editor.
	connect(this, &TextLabelOverlayEditor::contentsReplaced, this, &TextLabelOverlayEditor::updateEditorFields);
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool TextLabelOverlayEditor::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(source == editObject() && event.type() == ReferenceEvent::TargetChanged && static_cast<const TargetChangedEvent&>(event).field() == &PROPERTY_FIELD(TextLabelOverlay::sourceNode)) {
		updateEditorFields();
	}
	else if(source == sourceNode() && (event.type() == ReferenceEvent::PreliminaryStateAvailable || event.type() == ReferenceEvent::TargetChanged)) {
		updateEditorFieldsLater(this);
	}
	else if(source == sourceNode() && event.type() == ReferenceEvent::TitleChanged) {
		updateSourcesList();
	}
	return PropertiesEditor::referenceEvent(source, event);
}

/******************************************************************************
* Updates the combobox list showing the available data sources.
******************************************************************************/
void TextLabelOverlayEditor::updateSourcesList()
{
	_nodeComboBox->clear();
	if(TextLabelOverlay* overlay = static_object_cast<TextLabelOverlay>(editObject())) {
		// Enumerate all pipelines in the scene.
		overlay->dataset()->sceneRoot()->visitObjectNodes([&](PipelineSceneNode* pipeline) {
			_nodeComboBox->addItem(pipeline->objectTitle(), QVariant::fromValue(pipeline));
			return true;
		});
		_nodeComboBox->setCurrentIndex(_nodeComboBox->findData(QVariant::fromValue(overlay->sourceNode())));
	}
	if(_nodeComboBox->count() == 0) 
		_nodeComboBox->addItem(tr("<none>"));
}

/******************************************************************************
* Updates the UI.
******************************************************************************/
void TextLabelOverlayEditor::updateEditorFields()
{
	QString str;
	QStringList variableNames;
	PipelineSceneNode* node = nullptr;
	if(TextLabelOverlay* overlay = static_object_cast<TextLabelOverlay>(editObject())) {
		if(node = overlay->sourceNode()) {
			const PipelineFlowState& flowState = node->evaluatePipelineSynchronous(false);
			str.append(tr("<p>Dynamic attributes that can be referenced in the label text:</b><ul>"));
			if(flowState.data()) {
				for(const QString& attrName : flowState.buildAttributesMap().keys()) {
					str.append(QStringLiteral("<li>[%1]</li>").arg(attrName.toHtmlEscaped()));
					variableNames.push_back(QStringLiteral("[") + attrName + QStringLiteral("]"));
				}
			}
			str.append(QStringLiteral("</ul></p><p></p>"));
		}
	}
	setSourceNode(node);

	_attributeNamesList->setText(str);
	_attributeNamesList->updateGeometry();
	_textEdit->setWordList(variableNames);
	container()->updateRolloutsLater();
}

}	// End of namespace
