////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 OVITO GmbH, Germany
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
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/gui/desktop/mainwin/MainWindow.h>
#include <ovito/gui/base/viewport/ViewportInputMode.h>
#include "AnimationTimeSlider.h"

namespace Ovito {

using namespace std;

/******************************************************************************
* The constructor of the AnimationTimeSlider class.
******************************************************************************/
AnimationTimeSlider::AnimationTimeSlider(MainWindow* mainWindow, QWidget* parent) :
	QFrame(parent), _mainWindow(mainWindow)
{
	updateColorPalettes();

	setFrameShape(QFrame::NoFrame);
	setAutoFillBackground(true);
	setMouseTracking(true);
	setFocusPolicy(Qt::ClickFocus);

	connect(&mainWindow->datasetContainer(), &DataSetContainer::animationSettingsReplaced, this, &AnimationTimeSlider::onAnimationSettingsReplaced);
}

/******************************************************************************
* Creates the color palettes used by the widget.
******************************************************************************/
void AnimationTimeSlider::updateColorPalettes()
{
	_normalPalette = QGuiApplication::palette();
	_autoKeyModePalette = QGuiApplication::palette();
	_autoKeyModePalette.setColor(QPalette::Window, QColor(240, 60, 60));
	_sliderPalette = QGuiApplication::palette();
	_sliderPalette.setColor(QPalette::Button, 
		_mainWindow->darkTheme() ?
		_sliderPalette.color(QPalette::Button).lighter(150) :
		_sliderPalette.color(QPalette::Button).darker(110));
}

/******************************************************************************
* Handles widget state changes.
******************************************************************************/
void AnimationTimeSlider::changeEvent(QEvent* event)
{
	if(event->type() == QEvent::PaletteChange) {
		updateColorPalettes();
	}
	QFrame::changeEvent(event);
}

/******************************************************************************
* This is called when new animation settings have been loaded.
******************************************************************************/
void AnimationTimeSlider::onAnimationSettingsReplaced(AnimationSettings* newAnimationSettings)
{
	disconnect(_autoKeyModeChangedConnection);
	disconnect(_animIntervalChangedConnection);
	disconnect(_timeFormatChangedConnection);
	disconnect(_timeChangedConnection);
	_animSettings = newAnimationSettings;
	if(newAnimationSettings) {
		_autoKeyModeChangedConnection = connect(newAnimationSettings, &AnimationSettings::autoKeyModeChanged, this, &AnimationTimeSlider::onAutoKeyModeChanged);
		_animIntervalChangedConnection = connect(newAnimationSettings, &AnimationSettings::intervalChanged, this, (void (AnimationTimeSlider::*)())&AnimationTimeSlider::update);
		_timeFormatChangedConnection = connect(newAnimationSettings, &AnimationSettings::timeFormatChanged, this, (void (AnimationTimeSlider::*)())&AnimationTimeSlider::update);
		_timeChangedConnection = connect(newAnimationSettings, &AnimationSettings::timeChanged, this, (void (AnimationTimeSlider::*)())&AnimationTimeSlider::repaint);
		onAutoKeyModeChanged(_animSettings->autoKeyMode());
	}
	else onAutoKeyModeChanged(false);
	update();
}

/******************************************************************************
* Handles paint events.
******************************************************************************/
void AnimationTimeSlider::paintEvent(QPaintEvent* event)
{
	QFrame::paintEvent(event);
	if(!_animSettings) return;

	// Show slider only if there is more than one animation frame.
	int numFrames = (int)(_animSettings->animationInterval().duration() / _animSettings->ticksPerFrame()) + 1;
	if(numFrames > 1) {
		QStylePainter painter(this);

		QRect clientRect = frameRect();
		clientRect.adjust(frameWidth(), frameWidth(), -frameWidth(), -frameWidth());
		int thumbWidth = this->thumbWidth();
		TimePoint startTime, timeStep, endTime;
		std::tie(startTime, timeStep, endTime) = tickRange(maxTickLabelWidth());

		painter.setPen(QPen(QColor(180,180,220)));
		for(TimePoint time = startTime; time <= endTime; time += timeStep) {
			QString labelText = QString::number(_animSettings->timeToFrame(time));
			painter.drawText(timeToPos(time) - thumbWidth/2, clientRect.y(), thumbWidth, clientRect.height(), Qt::AlignCenter, labelText);
		}

		QStyleOptionButton btnOption;
		btnOption.initFrom(this);
		btnOption.rect = thumbRectangle();
		btnOption.text = _animSettings->timeToString(_animSettings->time());
		if(_animSettings->animationInterval().start() == 0)
			btnOption.text += " / " + _animSettings->timeToString(_animSettings->animationInterval().end());
		btnOption.state = ((_dragPos >= 0) ? QStyle::State_Sunken : QStyle::State_Raised) | QStyle::State_Enabled;
		btnOption.palette = _sliderPalette;
		painter.drawPrimitive(QStyle::PE_PanelButtonCommand, btnOption);
		btnOption.palette = _animSettings->autoKeyMode() ? _autoKeyModePalette : _normalPalette;
		painter.drawControl(QStyle::CE_PushButtonLabel, btnOption);
	}
}

/******************************************************************************
* Computes the maximum width of a frame tick label.
******************************************************************************/
int AnimationTimeSlider::maxTickLabelWidth()
{
	if(!_animSettings) return 0;
	QString label = QString::number(_animSettings->timeToFrame(_animSettings->animationInterval().end()));
	return fontMetrics().boundingRect(label).width() + 20;
}

/******************************************************************************
* Computes the time ticks to draw.
******************************************************************************/
std::tuple<TimePoint,TimePoint,TimePoint> AnimationTimeSlider::tickRange(int tickWidth)
{
	if(_animSettings) {
		QRect clientRect = frameRect();
		clientRect.adjust(frameWidth(), frameWidth(), -frameWidth(), -frameWidth());
		int thumbWidth = this->thumbWidth();
		int clientWidth = clientRect.width() - thumbWidth;
		int firstFrame = _animSettings->timeToFrame(_animSettings->animationInterval().start());
		int lastFrame = _animSettings->timeToFrame(_animSettings->animationInterval().end());
		int numFrames = lastFrame - firstFrame + 1;
		int nticks = std::min(clientWidth / tickWidth, numFrames);
		int ticksevery = numFrames / std::max(nticks, 1);
		if(ticksevery <= 1) ticksevery = ticksevery;
		else if(ticksevery <= 5) ticksevery = 5;
		else if(ticksevery <= 10) ticksevery = 10;
		else if(ticksevery <= 20) ticksevery = 20;
		else if(ticksevery <= 50) ticksevery = 50;
		else if(ticksevery <= 100) ticksevery = 100;
		else if(ticksevery <= 500) ticksevery = 500;
		else if(ticksevery <= 1000) ticksevery = 1000;
		else if(ticksevery <= 2000) ticksevery = 2000;
		else if(ticksevery <= 5000) ticksevery = 5000;
		else if(ticksevery <= 10000) ticksevery = 10000;
		if(ticksevery > 0) {
			return std::make_tuple(
					_animSettings->frameToTime(firstFrame),
					_animSettings->ticksPerFrame() * ticksevery,
					_animSettings->frameToTime(lastFrame));
		}
	}
	return std::tuple<TimePoint,TimePoint,TimePoint>(0, 1, 0);
}

/******************************************************************************
* Computes the x position within the widget corresponding to the given animation time.
******************************************************************************/
int AnimationTimeSlider::timeToPos(TimePoint time)
{
	if(!_animSettings) return 0;
	FloatType percentage = (FloatType)(time - _animSettings->animationInterval().start()) / (FloatType)(_animSettings->animationInterval().duration() + 1);
	QRect clientRect = frameRect();
	int tw = thumbWidth();
	int space = clientRect.width() - 2*frameWidth() - tw;
	return clientRect.x() + frameWidth() + (int)(percentage * space) + tw / 2;
}

/******************************************************************************
* Converts a distance in pixels to a time difference.
******************************************************************************/
TimePoint AnimationTimeSlider::distanceToTimeDifference(int distance)
{
	if(!_animSettings) return 0;
	QRect clientRect = frameRect();
	int tw = thumbWidth();
	int space = clientRect.width() - 2*frameWidth() - tw;
	return (int)((qint64)(_animSettings->animationInterval().duration() + 1) * distance / space);
}

/******************************************************************************
* Returns the recommended size for the widget.
******************************************************************************/
QSize AnimationTimeSlider::sizeHint() const
{
	return QSize(QFrame::sizeHint().width(), fontMetrics().height() + frameWidth() * 2 + 6);
}

/******************************************************************************
* Handles mouse down events.
******************************************************************************/
void AnimationTimeSlider::mousePressEvent(QMouseEvent* event)
{
	QRect thumbRect = thumbRectangle();
	if(thumbRect.contains(event->pos())) {
		_dragPos = ViewportInputMode::getMousePosition(event).x() - thumbRect.x();
	}
	else {
		_dragPos = thumbRect.width() / 2;
		mouseMoveEvent(event);
	}
	event->accept();
	update();
}

/******************************************************************************
* Is called when the widgets looses the input focus.
******************************************************************************/
void AnimationTimeSlider::focusOutEvent(QFocusEvent* event)
{
	_dragPos = -1;
	QFrame::focusOutEvent(event);
}

/******************************************************************************
* Handles mouse up events.
******************************************************************************/
void AnimationTimeSlider::mouseReleaseEvent(QMouseEvent* event)
{
	_dragPos = -1;
	event->accept();
	update();
}

/******************************************************************************
* Handles mouse move events.
******************************************************************************/
void AnimationTimeSlider::mouseMoveEvent(QMouseEvent* event)
{
	event->accept();
	if(!_animSettings) return;

	int newPos;
	int thumbSize = thumbWidth();

	if(_dragPos < 0)
		newPos = ViewportInputMode::getMousePosition(event).x() - thumbSize / 2;
	else
		newPos = ViewportInputMode::getMousePosition(event).x() - _dragPos;

	int rectWidth = frameRect().width() - 2*frameWidth();
	TimeInterval interval = _animSettings->animationInterval();
	TimePoint newTime = (TimePoint)((qint64)newPos * (qint64)(interval.duration() + 1) / (qint64)(rectWidth - thumbSize)) + interval.start();

	// Clamp new time to animation interval.
	newTime = qBound(interval.start(), newTime, interval.end());

	// Snap to frames
	int newFrame = _animSettings->timeToFrame(_animSettings->snapTime(newTime));
	if(_dragPos >= 0) {

		TimePoint newTime = _animSettings->frameToTime(newFrame);
		if(newTime == _animSettings->time()) return;

		// Set new time
		_animSettings->setTime(newTime);

		// Force immediate viewport update.
		_mainWindow->processViewportUpdates();
		repaint();
	}
	else if(interval.duration() > 0) {
		if(thumbRectangle().contains(event->pos()) == false) {
			FloatType percentage = (FloatType)(_animSettings->frameToTime(newFrame) - _animSettings->animationInterval().start())
					/ (FloatType)(_animSettings->animationInterval().duration() + 1);
			QRect clientRect = frameRect();
			clientRect.adjust(frameWidth(), frameWidth(), -frameWidth(), -frameWidth());
			int clientWidth = clientRect.width() - thumbWidth();
			QPoint pos(clientRect.x() + (int)(percentage * clientWidth) + thumbWidth() / 2, clientRect.height() / 2);
			QString frameName = _animSettings->namedFrames()[newFrame];
			QString tooltipText;
			if(!frameName.isEmpty())
				tooltipText = QString("%1 - %2").arg(newFrame).arg(frameName);
			else
				tooltipText = QString::number(newFrame);
			QToolTip::showText(mapToGlobal(pos), tooltipText, this);
		}
		else QToolTip::hideText();
	}
}

/******************************************************************************
* Computes the width of the thumb.
******************************************************************************/
int AnimationTimeSlider::thumbWidth() const
{
	int standardWidth = 70;
	// Expand the thumb width for animations with a large number of frames.
	if(_animSettings) {
		int nframes = _animSettings->animationInterval().duration() / _animSettings->ticksPerFrame();
		if(nframes > 1) {
			standardWidth += 10 * (int)std::log10(nframes);
		}
	}
	int clientWidth = frameRect().width() - 2*frameWidth();
	return std::min(clientWidth / 2, standardWidth);
}

/******************************************************************************
* Computes the coordinates of the slider thumb.
******************************************************************************/
QRect AnimationTimeSlider::thumbRectangle()
{
	if(!_animSettings)
		return QRect(0,0,0,0);

	TimeInterval interval = _animSettings->animationInterval();
	TimePoint value = std::min(std::max(_animSettings->time(), interval.start()), interval.end());
	FloatType percentage = (FloatType)(value - interval.start()) / (FloatType)(interval.duration() + 1);

	QRect clientRect = frameRect();
	clientRect.adjust(frameWidth(), frameWidth(), -frameWidth(), -frameWidth());
	int thumbSize = thumbWidth();
	int thumbPos = (int)((clientRect.width() - thumbSize) * percentage);
	return QRect(thumbPos + clientRect.x(), clientRect.y(), thumbSize, clientRect.height());
}

/******************************************************************************
* Is called whenever the Auto Key mode is activated or deactivated.
******************************************************************************/
void AnimationTimeSlider::onAutoKeyModeChanged(bool active)
{
	setPalette(active ? _autoKeyModePalette : _normalPalette);
	update();
}

}	// End of namespace
