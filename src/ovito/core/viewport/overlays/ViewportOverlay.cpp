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
#include <ovito/core/viewport/overlays/ViewportOverlay.h>

namespace Ovito {

IMPLEMENT_OVITO_CLASS(ViewportOverlay);
DEFINE_PROPERTY_FIELD(ViewportOverlay, renderBehindScene);
SET_PROPERTY_FIELD_LABEL(ViewportOverlay, renderBehindScene, "Draw behind scene");

/******************************************************************************
* Constructor.
******************************************************************************/
ViewportOverlay::ViewportOverlay(DataSet* dataset) : ActiveObject(dataset),
    _renderBehindScene(false)
{
}

/******************************************************************************
* Paints a text string with an optional outline.
******************************************************************************/
void ViewportOverlay::drawTextOutlined(QPainter& painter, const QRectF& rect, int flags, const QString& text, const Color& textColor, bool drawOutline, const Color& outlineColor)
{
	QPainterPath textPath;
	textPath.addText(0, 0, painter.font(), text);
	QRectF textBounds = textPath.boundingRect();

	if(flags & Qt::AlignLeft) textPath.translate(rect.left(), 0);
	else if(flags & Qt::AlignRight) textPath.translate(rect.right() - textBounds.width(), 0);
	else if(flags & Qt::AlignHCenter) textPath.translate(rect.left() + rect.width()/2.0 - textBounds.width()/2.0, 0);
	if(flags & Qt::AlignTop) textPath.translate(0, rect.top() + textBounds.height());
	else if(flags & Qt::AlignBottom) textPath.translate(0, rect.bottom());
	else if(flags & Qt::AlignVCenter) textPath.translate(0, rect.top() + rect.height()/2.0 + textBounds.height()/2.0);

	if(drawOutline) {
		// Always render the outline pen 3 pixels wide, irrespective of frame buffer resolution.
		qreal outlineWidth = 3.0 / painter.combinedTransform().m11();
		painter.setPen(QPen(QBrush(outlineColor), outlineWidth));
		painter.drawPath(textPath);
	}
	painter.fillPath(textPath, QBrush(textColor));
}

}	// End of namespace
