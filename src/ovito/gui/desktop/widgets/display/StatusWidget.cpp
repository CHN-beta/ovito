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
#include "StatusWidget.h"

namespace Ovito {

/******************************************************************************
* Constructor.
******************************************************************************/
StatusWidget::StatusWidget(QWidget* parent) : QScrollArea(parent)
{
	QWidget* container = new QWidget();
	QHBoxLayout* layout = new QHBoxLayout(container);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(2);

	_iconLabel = new QLabel(container);
	_iconLabel->setAlignment(Qt::AlignTop);
	layout->addWidget(_iconLabel, 0, Qt::AlignTop);

	_textLabel = new QLabel(container);
	_textLabel->setAlignment(Qt::AlignTop);
	_textLabel->setTextInteractionFlags(Qt::TextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard | Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard));
	_textLabel->setWordWrap(true);
	layout->addWidget(_textLabel, 1, Qt::AlignTop);

	setWidget(container);
	setWidgetResizable(true);
}

/******************************************************************************
* Sets the status displayed by the widget.
******************************************************************************/
void StatusWidget::setStatus(const PipelineStatus& status)
{
	_status = status;

	_textLabel->setText(status.text());

	const static QPixmap statusWarningIcon(":/guibase/mainwin/status/status_warning.png");
	const static QPixmap statusErrorIcon(":/guibase/mainwin/status/status_error.png");

	if(status.type() == PipelineStatus::Warning)
		_iconLabel->setPixmap(statusWarningIcon);
	else if(status.type() == PipelineStatus::Error)
		_iconLabel->setPixmap(statusErrorIcon);
	else
		_iconLabel->clear();
}

/******************************************************************************
* Returns the minimum size of the widget.
******************************************************************************/
QSize StatusWidget::minimumSizeHint() const
{
	int widgetHeight = widget()->minimumSizeHint().height();
	if(widgetHeight < 20) widgetHeight = 40;
	else if(widgetHeight < 30) widgetHeight *= 2;
	return QSize(QScrollArea::minimumSizeHint().width(),
			frameWidth()*2 + widgetHeight);
}

/******************************************************************************
* Returns the preferred size of the widget.
******************************************************************************/
QSize StatusWidget::sizeHint() const
{
	int widgetHeight = widget()->minimumSizeHint().height();
	if(widgetHeight < 20) widgetHeight = 40;
	else if(widgetHeight < 30) widgetHeight *= 2;
	return QSize(QScrollArea::sizeHint().width(),
			frameWidth()*2 + widgetHeight);
}

}	// End of namespace
