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
#include "VulkanTextPrimitive.h"
#include "VulkanSceneRenderer.h"

namespace Ovito {

/******************************************************************************
* Constructor.
******************************************************************************/
VulkanTextPrimitive::VulkanTextPrimitive(VulkanSceneRenderer* renderer) :
	_imageBuffer(renderer->createImagePrimitive())
{
}

/******************************************************************************
* Renders the text string.
******************************************************************************/
void VulkanTextPrimitive::render(VulkanSceneRenderer* renderer)
{
	if(text().isEmpty() || renderer->isPicking())
		return;
	
	if(_imageUpdateNeeded) {
		_imageUpdateNeeded = false;

		// Measure text size.
		QRect rect;
		qreal devicePixelRatio = renderer->devicePixelRatio();
		{
			QImage textureImage(1, 1, QImage::Format_RGB32);
			textureImage.setDevicePixelRatio(devicePixelRatio);
			QPainter painter(&textureImage);
			painter.setFont(font());
			rect = painter.boundingRect(QRect(), Qt::AlignLeft | Qt::AlignTop, text());
		}

		// Generate texture image.
		QImage textureImage((rect.width() * devicePixelRatio)+1, (rect.height() * devicePixelRatio)+1, QImage::Format_ARGB32_Premultiplied);
		textureImage.setDevicePixelRatio(devicePixelRatio);
		textureImage.fill((QColor)backgroundColor());
		{
			QPainter painter(&textureImage);
			painter.setFont(font());
			painter.setPen((QColor)color());
			painter.drawText(rect, Qt::AlignLeft | Qt::AlignTop, text());
		}
		_textOffset = rect.topLeft();

		_imageBuffer->setImage(textureImage);
	}

	Point2 alignedPos = position();
	Vector2 size = Vector2(_imageBuffer->image().width(), _imageBuffer->image().height()) * (FloatType)renderer->antialiasingLevel();
	if(alignment() & Qt::AlignRight) alignedPos.x() += -size.x();
	else if(alignment() & Qt::AlignHCenter) alignedPos.x() += -size.x() / 2;
	if(alignment() & Qt::AlignBottom) alignedPos.y() += -size.y();
	else if(alignment() & Qt::AlignVCenter) alignedPos.y() += -size.y() / 2;
	_imageBuffer->setRectWindow(Box2(alignedPos, alignedPos + size));
	renderer->renderImage(_imageBuffer);
}

}	// End of namespace
