////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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
#include "AutocompleteLineEdit.h"

namespace Ovito {

/******************************************************************************
* Constructor.
******************************************************************************/
AutocompleteLineEdit::AutocompleteLineEdit(QWidget* parent) : QLineEdit(parent),
		_wordSplitter("(?:(?<![\\w\\.@])(?=[\\w\\.@])|(?<=[\\w\\.@])(?![\\w\\.@]))")
{
	_wordListModel = new QStringListModel(this);
	_completer = new QCompleter(this);
	_completer->setCompletionMode(QCompleter::PopupCompletion);
	_completer->setCaseSensitivity(Qt::CaseInsensitive);
	_completer->setModel(_wordListModel);
	_completer->setWidget(this);
	connect(_completer, qOverload<const QString&>(&QCompleter::activated), this, &AutocompleteLineEdit::onComplete);
}

/******************************************************************************
* Inserts a complete word into the text field.
******************************************************************************/
void AutocompleteLineEdit::onComplete(const QString& completion)
{
	QStringList tokens = getTokenList();
	int pos = 0;
	for(QString& token : tokens) {
		pos += token.length();
		if(pos >= cursorPosition()) {
			int oldLen = token.length();
			token = completion;
			setText(tokens.join(QString()));
			setCursorPosition(pos - oldLen + completion.length());
			break;
		}
	}
}

/******************************************************************************
* Creates a list of tokens from the current text string.
******************************************************************************/
QStringList AutocompleteLineEdit::getTokenList() const
{
	// Split text at word boundaries. Consider '.' a word character.
	return text().split(_wordSplitter);
}

/******************************************************************************
* Handles key-press events.
******************************************************************************/
void AutocompleteLineEdit::keyPressEvent(QKeyEvent* event)
{
	if(_completer->popup()->isVisible()) {
		if(event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return ||
				event->key() == Qt::Key_Escape || event->key() == Qt::Key_Tab) {
	                event->ignore();
	                return;
		}
	}

	QLineEdit::keyPressEvent(event);

	QStringList tokens = getTokenList();
	if(tokens.empty())
		return;
	int pos = 0;
	QString completionPrefix;
	for(const QString& token : tokens) {
		pos += token.length();
		if(pos >= cursorPosition()) {
			completionPrefix = token.trimmed();
			break;
		}
	}

	if(completionPrefix != _completer->completionPrefix()) {
		_completer->setCompletionPrefix(completionPrefix);
		_completer->popup()->setCurrentIndex(_completer->completionModel()->index(0,0));
	}
	if(completionPrefix.isEmpty() == false && !_wordListModel->stringList().contains(completionPrefix))
		_completer->complete();
	else
		_completer->popup()->hide();
}

}	// End of namespace
