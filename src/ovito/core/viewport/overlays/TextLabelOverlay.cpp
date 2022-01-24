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

#include <ovito/core/Core.h>
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/rendering/RenderSettings.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/scene/SelectionSet.h>
#include <ovito/core/utilities/concurrent/SharedFuture.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include <ovito/core/app/Application.h>
#include "TextLabelOverlay.h"

namespace Ovito {

IMPLEMENT_OVITO_CLASS(TextLabelOverlay);
DEFINE_PROPERTY_FIELD(TextLabelOverlay, alignment);
DEFINE_PROPERTY_FIELD(TextLabelOverlay, font);
DEFINE_PROPERTY_FIELD(TextLabelOverlay, fontSize);
DEFINE_PROPERTY_FIELD(TextLabelOverlay, labelText);
DEFINE_PROPERTY_FIELD(TextLabelOverlay, offsetX);
DEFINE_PROPERTY_FIELD(TextLabelOverlay, offsetY);
DEFINE_PROPERTY_FIELD(TextLabelOverlay, textColor);
DEFINE_PROPERTY_FIELD(TextLabelOverlay, outlineColor);
DEFINE_PROPERTY_FIELD(TextLabelOverlay, outlineEnabled);
DEFINE_REFERENCE_FIELD(TextLabelOverlay, sourceNode);
DEFINE_PROPERTY_FIELD(TextLabelOverlay, valueFormatString);
SET_PROPERTY_FIELD_LABEL(TextLabelOverlay, alignment, "Position");
SET_PROPERTY_FIELD_LABEL(TextLabelOverlay, font, "Font");
SET_PROPERTY_FIELD_LABEL(TextLabelOverlay, fontSize, "Font size");
SET_PROPERTY_FIELD_LABEL(TextLabelOverlay, offsetX, "Offset X");
SET_PROPERTY_FIELD_LABEL(TextLabelOverlay, offsetY, "Offset Y");
SET_PROPERTY_FIELD_LABEL(TextLabelOverlay, textColor, "Text color");
SET_PROPERTY_FIELD_LABEL(TextLabelOverlay, outlineColor, "Outline color");
SET_PROPERTY_FIELD_LABEL(TextLabelOverlay, outlineEnabled, "Enable outline");
SET_PROPERTY_FIELD_LABEL(TextLabelOverlay, sourceNode, "Attributes source");
SET_PROPERTY_FIELD_LABEL(TextLabelOverlay, valueFormatString, "Number format");
SET_PROPERTY_FIELD_UNITS(TextLabelOverlay, offsetX, PercentParameterUnit);
SET_PROPERTY_FIELD_UNITS(TextLabelOverlay, offsetY, PercentParameterUnit);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(TextLabelOverlay, fontSize, FloatParameterUnit, 0);

/******************************************************************************
* Constructor.
******************************************************************************/
TextLabelOverlay::TextLabelOverlay(DataSet* dataset) : ViewportOverlay(dataset),
		_alignment(Qt::AlignLeft | Qt::AlignTop),
		_offsetX(0), 
		_offsetY(0),
		_fontSize(0.02),
		_labelText(tr("Text label")),
		_textColor(0,0,0.5),
		_outlineColor(1,1,1),
		_outlineEnabled(false),
		_valueFormatString("%.6g")
{
	// Automatically connect to the currently selected pipeline.
	setSourceNode(dynamic_object_cast<PipelineSceneNode>(dataset->selection()->firstNode()));
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void TextLabelOverlay::propertyChanged(const PropertyFieldDescriptor* field)
{
	if(field == PROPERTY_FIELD(alignment) && !isBeingLoaded() && !isAboutToBeDeleted() && !dataset()->undoStack().isUndoingOrRedoing() && ExecutionContext::isInteractive()) {
		// Automatically reset offset to zero when user changes the alignment of the overlay in the viewport.
		setOffsetX(0);
		setOffsetY(0);
	}
	ViewportOverlay::propertyChanged(field);
}

/******************************************************************************
* Lets the overlay paint its contents into the framebuffer.
******************************************************************************/
void TextLabelOverlay::render(SceneRenderer* renderer, const QRect& logicalViewportRect, const QRect& physicalViewportRect, MainThreadOperation& operation)
{
	if(renderer->isInteractive()) {
		const PipelineFlowState& flowState = sourceNode() ? sourceNode()->evaluatePipelineSynchronous(true) : PipelineFlowState();
		renderImplementation(renderer, physicalViewportRect, flowState);
	}
	else {
		if(sourceNode()) {
			PipelineEvaluationFuture pipelineEvaluation = sourceNode()->evaluatePipeline(PipelineEvaluationRequest(renderer->time()));
			if(!operation.waitForFuture(pipelineEvaluation))
				return;
			renderImplementation(renderer, physicalViewportRect, pipelineEvaluation.result());
		}
		else {
			renderImplementation(renderer, physicalViewportRect, {});
		}
	}
}

/******************************************************************************
* This method paints the overlay contents onto the given canvas.
******************************************************************************/
void TextLabelOverlay::renderImplementation(SceneRenderer* renderer, const QRect& viewportRect, const PipelineFlowState& flowState)
{
	// Resolve the label text.
	QString textString = labelText();

	// Resolve global attributes referenced by placeholders in the text string.
	if(flowState && textString.contains('[')) {
		const QVariantMap& attributes = flowState.buildAttributesMap();

		// Prepare the floating-point format string.
		QByteArray format = valueFormatString().toUtf8();
		if(format.isEmpty() || format.contains("%s")) format = QByteArrayLiteral("###");

		for(auto a = attributes.cbegin(); a != attributes.cend(); ++a) {

			QString valueString;
			if(getQVariantTypeId(a.value()) == QMetaType::Double || getQVariantTypeId(a.value()) == QMetaType::Float) {
				valueString = QString::asprintf(format.constData(), a.value().toDouble());
			}
			else {
				valueString = a.value().toString();
			}

			textString.replace(QStringLiteral("[") + a.key() + QStringLiteral("]"), valueString);
		}
	}
	if(textString.isEmpty())
		return;

	// Prepare the text rendering primitive.
	TextPrimitive textPrimitive;
	textPrimitive.setColor(textColor());
	if(outlineEnabled()) textPrimitive.setOutlineColor(outlineColor());
	textPrimitive.setAlignment(alignment());
	textPrimitive.setText(std::move(textString));
	textPrimitive.setTextFormat(Qt::AutoText);

	// Resolve the font used by the label.
	FloatType fontSize = this->fontSize() * viewportRect.height();
	if(fontSize <= 0) return;
	QFont font = this->font();
	font.setPointSizeF(fontSize / renderer->devicePixelRatio()); // Font size if always in logical coordinates.
	textPrimitive.setFont(std::move(font));

	// Add an inset to the framebuffer rect.
	int margins = (int)fontSize;
	QRectF marginRect = viewportRect.marginsRemoved(QMargins(margins, margins, margins, margins));

	// Determine alignment of the text box in the framebuffer rect.
	Point2 pos;

	if(alignment() & Qt::AlignRight) pos.x() = marginRect.left() + marginRect.width();	
	else if(alignment() & Qt::AlignHCenter) pos.x() = marginRect.left() + marginRect.width() / 2.0;
	else pos.x() = marginRect.left();

	if(alignment() & Qt::AlignBottom) pos.y() = marginRect.top() + marginRect.height();	
	else if(alignment() & Qt::AlignVCenter) pos.y() = marginRect.top() + marginRect.height() / 2.0;
	else pos.y() = marginRect.top();

	// Compute final positions.
	textPrimitive.setPositionWindow(pos + Vector2(offsetX() * viewportRect.width(), -offsetY() * viewportRect.height()));

	// Paint the image into the output framebuffer.
	renderer->setDepthTestEnabled(false);
	renderer->renderText(textPrimitive);
}

}	// End of namespace
