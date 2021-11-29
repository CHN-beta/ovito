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

#include <ovito/stdmod/StdMod.h>
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/rendering/RenderSettings.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/scene/RootSceneNode.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include "ColorLegendOverlay.h"

namespace Ovito::StdMod {

IMPLEMENT_OVITO_CLASS(ColorLegendOverlay);
DEFINE_REFERENCE_FIELD(ColorLegendOverlay, modifier);
DEFINE_REFERENCE_FIELD(ColorLegendOverlay, colorMapping);
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, alignment, "Position");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, orientation, "Orientation");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, legendSize, "Size factor");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, font, "Font");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, fontSize, "Font size");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, offsetX, "Offset X");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, offsetY, "Offset Y");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, aspectRatio, "Aspect ratio");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, textColor, "Font color");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, outlineColor, "Outline color");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, outlineEnabled, "Enable outline");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, title, "Title");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, label1, "Label 1");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, label2, "Label 2");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, valueFormatString, "Number format");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, sourceProperty, "Source property");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, borderEnabled, "Draw border");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, borderColor, "Border color");
SET_PROPERTY_FIELD_UNITS(ColorLegendOverlay, offsetX, PercentParameterUnit);
SET_PROPERTY_FIELD_UNITS(ColorLegendOverlay, offsetY, PercentParameterUnit);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ColorLegendOverlay, legendSize, FloatParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ColorLegendOverlay, aspectRatio, FloatParameterUnit, 1);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ColorLegendOverlay, fontSize, FloatParameterUnit, 0);

/******************************************************************************
* Constructor.
******************************************************************************/
ColorLegendOverlay::ColorLegendOverlay(DataSet* dataset) : ViewportOverlay(dataset),
	_alignment(Qt::AlignHCenter | Qt::AlignBottom),
	_orientation(Qt::Horizontal),
	_legendSize(0.3),
	_offsetX(0),
	_offsetY(0),
	_fontSize(0.1),
	_valueFormatString("%g"),
	_aspectRatio(8.0),
	_textColor(0,0,0),
	_outlineColor(1,1,1),
	_outlineEnabled(false),
	_borderEnabled(false),
	_borderColor(0,0,0)
{
	// Find a ColorCodingModifier in the scene that we can connect to.
	dataset->sceneRoot()->visitObjectNodes([&](PipelineSceneNode* pipeline) {
		PipelineObject* obj = pipeline->dataProvider();
		while(obj) {
			if(ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(obj)) {
				if(ColorCodingModifier* mod = dynamic_object_cast<ColorCodingModifier>(modApp->modifier())) {
					setModifier(mod);
					if(mod->isEnabled())
						return false;	// Stop search.
				}
				obj = modApp->input();
			}
			else break;
		}
		return true;
	});
}

/******************************************************************************
* Initializes the object's parameter fields with default values and loads 
* user-defined default values from the application's settings store (GUI only).
******************************************************************************/
void ColorLegendOverlay::initializeObject(ExecutionContext executionContext)
{
	// If there is no ColorCodingModifier in the scene, initialize the overlay to use 
	// the first available typed property as color source.
	if(executionContext == ExecutionContext::Interactive && modifier() == nullptr && !sourceProperty()) {
		dataset()->sceneRoot()->visitObjectNodes([&](PipelineSceneNode* pipeline) {
			const PipelineFlowState& state = pipeline->evaluatePipelineSynchronous(false);
			for(const ConstDataObjectPath& dataPath : state.getObjectsRecursive(PropertyObject::OOClass())) {
				const PropertyObject* property = static_object_cast<PropertyObject>(dataPath.back());
				// Check if the property is a typed property, i.e. it has one or more ElementType objects attached to it.
				if(property->isTypedProperty() && dataPath.size() >= 2) {
					setSourceProperty(dataPath);
					return false;
				}
			}
			return true;
		});
	}

	ViewportOverlay::initializeObject(executionContext);
}

/******************************************************************************
* This method paints the overlay contents onto the given canvas.
******************************************************************************/
void ColorLegendOverlay::renderImplementation(TimePoint time, QPainter& painter, const ViewProjectionParameters& projParams, bool isInteractive, SynchronousOperation operation)
{
	DataOORef<const PropertyObject> typedProperty;

	// Check whether a source has been set for this color legend:
	if(modifier() || colorMapping()) {
		// Reset status of overlay.
		setStatus(PipelineStatus::Success);
	}
	else if(sourceProperty()) {
		// Look up the typed property in one of the scene's pipeline outputs.
		dataset()->sceneRoot()->visitObjectNodes([&](PipelineSceneNode* pipeline) {

			// Evaulate pipeline and obtain output data collection.
			if(!isInteractive) {
				PipelineEvaluationFuture pipelineEvaluation = pipeline->evaluatePipeline(time);
				if(!operation.waitForFuture(pipelineEvaluation))
					return false;
				// Look up the typed property.
				typedProperty = pipelineEvaluation.result().getLeafObject(sourceProperty());
			}
			else {
				const PipelineFlowState& state = pipeline->evaluatePipelineSynchronous(false);
				// Look up the typed property.
				typedProperty = state.getLeafObject(sourceProperty());
			}
			if(typedProperty)
				return false;

			return true;
		});
		if(operation.isCanceled())
			return;
		
		// Verify that the typed property, which has been selected as the source of the color legend, is available.
		if(!typedProperty) {
			// Set warning status to be displayed in the GUI.
			setStatus(PipelineStatus(PipelineStatus::Warning, tr("The property '%1' is not available in the pipeline output.").arg(sourceProperty().dataTitleOrString())));

			// Escalate to an error state if in batch mode.
			if(Application::instance()->consoleMode())
				throwException(tr("The property '%1' set as source of the color legend is not present in the data pipeline output.").arg(sourceProperty().dataTitleOrString()));
			else
				return;
		}
		else if(!typedProperty->isTypedProperty()) {
			// Set warning status to be displayed in the GUI.
			setStatus(PipelineStatus(PipelineStatus::Warning, tr("The property '%1' is not a typed property.").arg(sourceProperty().dataTitleOrString())));

			// Escalate to an error state if in batch mode.
			if(Application::instance()->consoleMode())
				throwException(tr("The property '%1' set as source of the color legend is not a typed property, i.e., it has no ElementType(s) attached.").arg(sourceProperty().dataTitleOrString()));
			else
				return;
		}

		// Reset status of overlay.
		setStatus(PipelineStatus::Success);
	}
	else {
		// Set warning status to be displayed in the GUI.
		setStatus(PipelineStatus(PipelineStatus::Warning, tr("No source Color Coding modifier has been selected for this color legend.")));

		// Escalate to an error state if in batch mode.
		if(Application::instance()->consoleMode()) {
			throwException(tr("You are trying to render a Viewport with a ColorLegendOverlay whose 'modifier' property has "
							  "not been set to any ColorCodingModifier. Did you forget to assign a source for the color legend?"));
		}
		else {
			// Ignore invalid configuration in GUI mode by not rendering the legend.
			return;
		}
	}

	// Calculate position and size of color legend rectangle.
	const QRect& windowRect = painter.window();
	FloatType legendSize = this->legendSize() * windowRect.height();
	if(legendSize <= 0) return;

	FloatType colorBarWidth = legendSize;
	FloatType colorBarHeight = colorBarWidth / std::max(FloatType(0.01), aspectRatio());
	bool vertical = (orientation() == Qt::Vertical);
	if(vertical)
		std::swap(colorBarWidth, colorBarHeight);

	QPointF origin(offsetX() * windowRect.width(), -offsetY() * windowRect.height());
	FloatType hmargin = FloatType(0.01) * windowRect.width();
	FloatType vmargin = FloatType(0.01) * windowRect.height();

	if(alignment() & Qt::AlignLeft) origin.rx() += hmargin;
	else if(alignment() & Qt::AlignRight) origin.rx() += windowRect.width() - hmargin - colorBarWidth;
	else if(alignment() & Qt::AlignHCenter) origin.rx() += FloatType(0.5) * windowRect.width() - FloatType(0.5) * colorBarWidth;

	if(alignment() & Qt::AlignTop) origin.ry() += vmargin;
	else if(alignment() & Qt::AlignBottom) origin.ry() += windowRect.height() - vmargin - colorBarHeight;
	else if(alignment() & Qt::AlignVCenter) origin.ry() += FloatType(0.5) * windowRect.height() - FloatType(0.5) * colorBarHeight;

	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::TextAntialiasing);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

	QRectF colorBarRect(origin, QSizeF(colorBarWidth, colorBarHeight));

	if(modifier()) {

		// Get modifier's parameters.
		FloatType startValue = modifier()->startValue();
		FloatType endValue = modifier()->endValue();
		if(modifier()->autoAdjustRange() && (label1().isEmpty() || label2().isEmpty())) {
			// Get the automatically adjusted range of the color coding modifier.
			// This requires a partial pipeline evaluation up to the color coding modifier.
			startValue = std::numeric_limits<FloatType>::quiet_NaN();
			endValue = std::numeric_limits<FloatType>::quiet_NaN();
			if(ModifierApplication* modApp = modifier()->someModifierApplication()) {
				QVariant minValue, maxValue;
				if(!isInteractive) {
					SharedFuture<PipelineFlowState> stateFuture = modApp->evaluate(PipelineEvaluationRequest(time));
					if(!operation.waitForFuture(stateFuture))
						return;
					const PipelineFlowState& state = stateFuture.result();
					minValue = state.getAttributeValue(modApp, QStringLiteral("ColorCoding.RangeMin"));
					maxValue = state.getAttributeValue(modApp, QStringLiteral("ColorCoding.RangeMax"));
				}
				else {
					const PipelineFlowState& state = modApp->evaluateSynchronous(time);
					minValue = state.getAttributeValue(modApp, QStringLiteral("ColorCoding.RangeMin"));
					maxValue = state.getAttributeValue(modApp, QStringLiteral("ColorCoding.RangeMax"));
				}
				if(minValue.isValid() && maxValue.isValid()) {
					startValue = minValue.value<FloatType>();
					endValue = maxValue.value<FloatType>();
				}
			}
		}

		drawContinuousColorMap(time, painter, colorBarRect, legendSize, isInteractive, PseudoColorMapping(startValue, endValue, modifier()->colorGradient()), modifier()->sourceProperty().nameWithComponent());
	}
	else if(colorMapping()) {
		drawContinuousColorMap(time, painter, colorBarRect, legendSize, isInteractive, colorMapping()->pseudoColorMapping(), colorMapping()->sourceProperty().nameWithComponent());
	}
	else if(typedProperty) {
		drawDiscreteColorMap(painter, colorBarRect, legendSize, typedProperty);
	}
}

/******************************************************************************
* Draws the color legend for a Color Coding modifier.
******************************************************************************/
void ColorLegendOverlay::drawContinuousColorMap(TimePoint time, QPainter& painter, const QRectF& colorBarRect, FloatType legendSize, bool isInteractive, const PseudoColorMapping& mapping, const QString& propertyName)
{
	if(!mapping.gradient())
		return;

	// Create the color scale image.
	int imageSize = 256;
	QImage image((orientation() == Qt::Vertical) ? 1 : imageSize, (orientation() == Qt::Vertical) ? imageSize : 1, QImage::Format_RGB32);
	for(int i = 0; i < imageSize; i++) {
		FloatType t = (FloatType)i / (FloatType)(imageSize - 1);
		Color color = mapping.gradient()->valueToColor((orientation() == Qt::Vertical) ? (FloatType(1) - t) : t);
		image.setPixel((orientation() == Qt::Vertical) ? 0 : i, (orientation() == Qt::Vertical) ? i : 0, QColor(color).rgb());
	}
	painter.drawImage(colorBarRect, image);

	if(borderEnabled()) {
		qreal borderWidth = 2.0 / painter.combinedTransform().m11();
		painter.setPen(QPen(QBrush(borderColor()), borderWidth, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
		painter.setBrush({});
		painter.drawRect(colorBarRect);
	}

	qreal fontSize = legendSize * std::max(FloatType(0), this->fontSize());
	if(fontSize == 0) return;
	QFont font = this->font();

	QByteArray format = valueFormatString().toUtf8();
	if(format.contains("%s")) format.clear();

	QString titleLabel, topLabel, bottomLabel;
	if(label1().isEmpty())
		topLabel = std::isfinite(mapping.maxValue()) ? QString::asprintf(format.constData(), mapping.maxValue()) : QStringLiteral("###");
	else
		topLabel = label1();
	if(label2().isEmpty())
		bottomLabel = std::isfinite(mapping.minValue()) ? QString::asprintf(format.constData(), mapping.minValue()) : QStringLiteral("###");
	else
		bottomLabel = label2();
	if(title().isEmpty())
		titleLabel = propertyName;
	else
		titleLabel = title();

	font.setPointSizeF(fontSize);
	painter.setFont(font);

	qreal textMargin = 0.2 * legendSize / std::max(FloatType(0.01), aspectRatio());

	// Move the text path to the correct location based on color bar direction and position
	int titleFlags = Qt::AlignBottom | Qt::TextDontClip;
	QRectF titleRect = colorBarRect;
	titleRect.setBottom(titleRect.top() - QFontMetricsF(font).descent());
	if(orientation() != Qt::Vertical || (alignment() & Qt::AlignHCenter)) {
		titleFlags |= Qt::AlignHCenter;
		titleRect.translate(0, -0.5 * textMargin);
	}
	else {
		if(alignment() & Qt::AlignLeft) {
			titleFlags |= Qt::AlignLeft;
			titleRect.translate(0, -textMargin);
		}
		else if(alignment() & Qt::AlignRight) {
			titleFlags |= Qt::AlignRight;
			titleRect.translate(0, -textMargin);
		}
		else {
			titleFlags |= Qt::AlignHCenter;
		}
	}

	drawTextOutlined(painter, titleRect, titleFlags, titleLabel, textColor(), outlineEnabled(), outlineColor());

	font.setPointSizeF(fontSize * 0.8);
	painter.setFont(font);

	int topFlags = Qt::TextDontClip;
	int bottomFlags = Qt::TextDontClip;
	QRectF topRect = colorBarRect;
	QRectF bottomRect = colorBarRect;

	if(orientation() != Qt::Vertical) {
		bottomFlags |= Qt::AlignRight | Qt::AlignVCenter;
		topFlags |= Qt::AlignLeft | Qt::AlignVCenter;
		bottomRect.setRight(bottomRect.left() - textMargin);
		topRect.setLeft(topRect.right() + textMargin);
	}
	else {		
		if((alignment() & Qt::AlignLeft) || (alignment() & Qt::AlignHCenter)) {
			bottomFlags |= Qt::AlignLeft | Qt::AlignBottom;
			topFlags |= Qt::AlignLeft | Qt::AlignTop;
			topRect.setLeft(topRect.right() + textMargin);
			bottomRect.setLeft(bottomRect.right() + textMargin);
		}
		else if(alignment() & Qt::AlignRight) {
			bottomFlags |= Qt::AlignRight | Qt::AlignBottom;
			topFlags |= Qt::AlignRight | Qt::AlignTop;
			topRect.setRight(topRect.left() - textMargin);
			bottomRect.setRight(bottomRect.left() - textMargin);
		}
	}

	drawTextOutlined(painter, topRect, topFlags, topLabel, textColor(), outlineEnabled(), outlineColor());
	drawTextOutlined(painter, bottomRect, bottomFlags, bottomLabel, textColor(), outlineEnabled(), outlineColor());	
}

/******************************************************************************
* Draws the color legend for a typed property.
******************************************************************************/
void ColorLegendOverlay:: drawDiscreteColorMap(QPainter& painter, const QRectF& colorBarRect, FloatType legendSize, const PropertyObject* property)
{
	// Count the number of element types that are enabled.
	int numTypes = boost::count_if(property->elementTypes(), [](const ElementType* type) { return type && type->enabled(); });

	qreal borderWidth = 2.0 / painter.combinedTransform().m11();
	if(borderEnabled())
		painter.setPen(QPen(QBrush(borderColor()), borderWidth, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
	else
		painter.setPen({});

	if(numTypes != 0) {
		QRectF rect = colorBarRect;
		if(orientation() == Qt::Vertical)
			rect.setHeight(rect.height() / numTypes);
		else
			rect.setWidth(rect.width() / numTypes);

		for(const ElementType* type : property->elementTypes()) {
			if(type && type->enabled()) {
				painter.setBrush(QBrush(type->color()));
				painter.drawRect(rect);
				if(orientation() == Qt::Vertical)
					rect.moveTop(rect.bottom());
				else
					rect.moveLeft(rect.right());
			}
		}
	}
	else {
		painter.setBrush({});
		painter.drawRect(colorBarRect);
	}

	qreal fontSize = legendSize * std::max(FloatType(0), this->fontSize());
	if(fontSize == 0) return;
	QFont font = this->font();
	font.setPointSizeF(fontSize);

	QString titleLabel;
	if(title().isEmpty())
		titleLabel = property->objectTitle();
	else
		titleLabel = title();

	painter.setFont(font);

	qreal textMargin = 0.2 * legendSize / std::max(FloatType(0.01), aspectRatio());

	// Move the text path to the correct location based on color bar direction and position
	int titleFlags = Qt::AlignBottom | Qt::TextDontClip;
	QRectF titleRect = colorBarRect;
	if(orientation() != Qt::Vertical || (alignment() & Qt::AlignHCenter)) {
		titleFlags |= Qt::AlignHCenter;
		titleRect.translate(0, -0.5 * textMargin);
	}
	if(orientation() == Qt::Vertical) {
		titleRect.setBottom(titleRect.top() - QFontMetricsF(font).descent());
		if(alignment() & Qt::AlignLeft) {
			titleFlags |= Qt::AlignLeft | Qt::AlignBottom;
			titleRect.translate(0, -textMargin);
		}
		else if(alignment() & Qt::AlignRight) {
			titleFlags |= Qt::AlignRight | Qt::AlignBottom;
			titleRect.translate(0, -textMargin);
		}
		else {
			titleFlags |= Qt::AlignHCenter | Qt::AlignBottom;
		}
	}
	else {
		if((alignment() & Qt::AlignTop) || (alignment() & Qt::AlignVCenter)) {
			titleFlags |= Qt::AlignHCenter | Qt::AlignBottom;
			titleRect.setBottom(titleRect.top() - QFontMetricsF(font).descent() - textMargin);
		}
		else {
			titleFlags |= Qt::AlignHCenter | Qt::AlignTop;
			titleRect.setTop(titleRect.bottom() + textMargin);
		}
	}

	drawTextOutlined(painter, titleRect, titleFlags, titleLabel, textColor(), outlineEnabled(), outlineColor());	

	// Draw type name labels.
	if(numTypes == 0)
		return;

	int labelFlags = Qt::TextDontClip;
	QRectF labelRect = colorBarRect;

	if(orientation() == Qt::Vertical) {
		if((alignment() & Qt::AlignLeft) || (alignment() & Qt::AlignHCenter)) {
			labelFlags |= Qt::AlignLeft | Qt::AlignVCenter;
			labelRect.setLeft(labelRect.right() + textMargin);
		}
		else {
			labelFlags |= Qt::AlignRight | Qt::AlignVCenter;
			labelRect.setRight(labelRect.left() - textMargin);
		}
		labelRect.setHeight(labelRect.height() / numTypes);
	}
	else {
		if((alignment() & Qt::AlignTop) || (alignment() & Qt::AlignVCenter)) {
			labelFlags |= Qt::AlignHCenter | Qt::AlignTop;
			labelRect.setTop(labelRect.bottom() + textMargin);
		}
		else {
			labelFlags |= Qt::AlignHCenter | Qt::AlignBottom;
			labelRect.setBottom(labelRect.top() - textMargin - QFontMetricsF(font).descent());
		}
		labelRect.setWidth(labelRect.width() / numTypes);
	}

	for(const ElementType* type : property->elementTypes()) {
		if(!type || !type->enabled()) 
			continue;

		const QString& typeLabelString = type->objectTitle();
		drawTextOutlined(painter, labelRect, labelFlags, typeLabelString, textColor(), outlineEnabled(), outlineColor());			

		if(orientation() == Qt::Vertical)
			labelRect.moveTop(labelRect.bottom());
		else
			labelRect.moveLeft(labelRect.right());
	}
}

}	// End of namespace
