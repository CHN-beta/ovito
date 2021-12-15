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
#include "OpenGLSceneRenderer.h"
#include "OpenGLResourceManager.h"

namespace Ovito {

/******************************************************************************
* Renders a text string.
******************************************************************************/
void OpenGLSceneRenderer::renderTextImplementation(const TextPrimitive& primitive)
{
	if(primitive.text().isEmpty() || isPicking())
		return;
	
    // Look up the image primitive for the text label in the cache.
	auto& [imagePrimitive, offset] = OpenGLResourceManager::instance()->lookup<std::tuple<ImagePrimitive, QPointF>>(
		RendererResourceKey<struct TextImageCache, QString, ColorA, ColorA, ColorA, FloatType, qreal, QString, bool, int>{ 
			primitive.text(), 
			primitive.color(), 
			primitive.backgroundColor(), 
			primitive.outlineColor(),
			primitive.outlineWidth(),
			this->devicePixelRatio(),
			primitive.font().key(),
			primitive.useTightBox(),
			primitive.alignment()
		}, currentResourceFrame());

	if(imagePrimitive.image().isNull()) {

		// Measure text size in device pixel units.
		QRectF bounds = primitive.queryBounds(this);

		// Add margin for the outline.
		qreal devicePixelRatio = this->devicePixelRatio();
		qreal outlineWidth = std::max(0.0, (primitive.outlineColor().a() > 0.0) ? (qreal)primitive.outlineWidth() : 0.0) * devicePixelRatio;

		// Convert to physical units.
		QRect pixelBounds = bounds.adjusted(-outlineWidth, -outlineWidth, outlineWidth, outlineWidth).toAlignedRect();

		// Generate texture image.
		QImage textureImage(pixelBounds.width(), pixelBounds.height(), glcontext()->isOpenGLES() ? QImage::Format_ARGB32 : QImage::Format_ARGB32_Premultiplied);
		textureImage.setDevicePixelRatio(devicePixelRatio);
		textureImage.fill((QColor)primitive.backgroundColor());
		{
			QPainter painter(&textureImage);
			painter.setRenderHint(QPainter::Antialiasing);
			painter.setRenderHint(QPainter::TextAntialiasing);
			painter.setFont(primitive.font());

			QPointF textOffset(outlineWidth, outlineWidth);
			textOffset.rx() -= bounds.left();
			textOffset.ry() -= bounds.top();
			textOffset.rx() /= devicePixelRatio;
			textOffset.ry() /= devicePixelRatio;

			if(outlineWidth != 0) {
				QPainterPath textPath;
				textPath.addText(textOffset, primitive.font(), primitive.text());
				painter.setPen(QPen(QBrush(primitive.outlineColor()), primitive.outlineWidth()));
				painter.drawPath(textPath);
			}

			painter.setPen((QColor)primitive.color());
			painter.drawText(textOffset, primitive.text());
		}

		imagePrimitive.setImage(std::move(textureImage));
		if(!primitive.useTightBox())
			offset = QPointF(bounds.left() - outlineWidth, -outlineWidth);
		else
			offset = QPointF(-outlineWidth, -outlineWidth);

		if(primitive.alignment() & Qt::AlignRight) offset.rx() += -bounds.width();
		else if(primitive.alignment() & Qt::AlignHCenter) offset.rx() += -bounds.width() / 2;
		if(primitive.alignment() & Qt::AlignBottom) offset.ry() += -bounds.height();
		else if(primitive.alignment() & Qt::AlignVCenter) offset.ry() += -bounds.height() / 2;
	}

	QPoint alignedPos = (QPointF(primitive.position().x(), primitive.position().y()) + offset).toPoint();
	imagePrimitive.setRectWindow(QRect(alignedPos, imagePrimitive.image().size()));
	renderImage(imagePrimitive);
}

}	// End of namespace
