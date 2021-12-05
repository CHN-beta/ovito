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
	
    // The look-up key for the image primtive cache.
	ImagePrimitive& cachedImagePrimitive = OpenGLResourceManager::instance()->lookup<ImagePrimitive>(
		RendererResourceKey<struct TextImageCache, QString, ColorA, ColorA, QString>{ 
			primitive.text(), 
			primitive.color(), 
			primitive.backgroundColor(), 
			primitive.font().key() }, 
		currentResourceFrame());

	if(cachedImagePrimitive.image().isNull()) {

		// Measure text size.
		QRect rect;
		qreal devicePixelRatio = this->devicePixelRatio();
		{
			QImage textureImage(1, 1, QImage::Format_RGB32);
			textureImage.setDevicePixelRatio(devicePixelRatio);
			QPainter painter(&textureImage);
			painter.setFont(primitive.font());
			rect = painter.boundingRect(QRect(), Qt::AlignLeft | Qt::AlignTop, primitive.text());
		}

		// Generate texture image.
		QImage textureImage((rect.width() * devicePixelRatio)+1, (rect.height() * devicePixelRatio)+1, glcontext()->isOpenGLES() ? QImage::Format_ARGB32 : QImage::Format_ARGB32_Premultiplied);
		textureImage.setDevicePixelRatio(devicePixelRatio);
		textureImage.fill((QColor)primitive.backgroundColor());
		{
			QPainter painter(&textureImage);
			painter.setFont(primitive.font());
			painter.setPen((QColor)primitive.color());
			painter.drawText(rect, Qt::AlignLeft | Qt::AlignTop, primitive.text());
		}
//		_textOffset = rect.topLeft();

		cachedImagePrimitive.setImage(std::move(textureImage));
	}

	Point2 alignedPos = primitive.position();
	Vector2 size = Vector2(cachedImagePrimitive.image().width(), cachedImagePrimitive.image().height()) * (FloatType)antialiasingLevel();
	if(primitive.alignment() & Qt::AlignRight) alignedPos.x() += -size.x();
	else if(primitive.alignment() & Qt::AlignHCenter) alignedPos.x() += -size.x() / 2;
	if(primitive.alignment() & Qt::AlignBottom) alignedPos.y() += -size.y();
	else if(primitive.alignment() & Qt::AlignVCenter) alignedPos.y() += -size.y() / 2;
	cachedImagePrimitive.setRectWindow(Box2(alignedPos, alignedPos + size));
	renderImage(cachedImagePrimitive);
}

}	// End of namespace
