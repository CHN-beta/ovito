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

#pragma once


#include <ovito/gui/qml/GUI.h>
#include <ovito/gui/qml/dataset/WasmDataSetContainer.h>
#include <ovito/core/app/UserInterface.h>
#include <ovito/gui/base/mainwin/PipelineListModel.h>
#include <ovito/gui/base/mainwin/ModifierListModel.h>

namespace Ovito {

/**
 * \brief The main window of the application.
 */
class OVITO_GUI_EXPORT MainWindow : public QQuickItem, public MainWindowInterface
{
	Q_OBJECT
	QML_ELEMENT

	Q_PROPERTY(Ovito::WasmDataSetContainer* datasetContainer READ datasetContainer CONSTANT)
	Q_PROPERTY(QString statusBarText READ statusBarText WRITE showStatusBarMessage NOTIFY statusBarTextChanged)
	Q_PROPERTY(Ovito::ModifierListModel* modifierListModel MEMBER _modifierListModel CONSTANT)
	Q_PROPERTY(Ovito::PipelineListModel* pipelineListModel MEMBER _pipelineListModel CONSTANT)
	Q_PROPERTY(QString systemReport READ generateSystemReport CONSTANT)

public:

	/// Constructor of the main window class.
	MainWindow();

	/// Destructor.
	virtual ~MainWindow();

	/// Returns the container that keeps a reference to the current dataset.
	WasmDataSetContainer* datasetContainer() { return &_datasetContainer; }

	/// Displays a message string in the window's status bar.
	virtual void showStatusBarMessage(const QString& message, int timeout = 0) override;

	/// Returns the text currently displayed in the window's status bar.
	const QString& statusBarText() const { return _statusBarTextMarkup; }

	/// Executes the user-provided function and records all actions on the undo stack.
	Q_INVOKABLE void undoableOperation(const QString& actionDescription, const QJSValue& callbackFunction);

public Q_SLOTS:

	/// Lets the user select a file on the local computer to be imported into the scene.
	void importDataFile();

	/// This slot function displays an error popup in the main window.
	void showErrorMessage(const QString& message, const QString& detailedText) {
		Q_EMIT error(message, detailedText);
	}

	/// Hides any messages currently displayed in the window's status bar.
	virtual void clearStatusBarMessage() override { 
		showStatusBarMessage(QString()); 
	}

	/// Shows the online manual and opens the given help page.
	void openHelpTopic(const QString& page) {
		MainWindowInterface::openHelpTopic(page);
	}

Q_SIGNALS:

	/// This signal is emitted to display an error message to the user.
	void error(const QString& message, const QString& detailedText);

	/// This signal is emitted when the text to be displayed in the window's status bar changes.
	void statusBarTextChanged(const QString& message);

private:

	/// Container managing the current dataset.
	WasmDataSetContainer _datasetContainer;

	/// The text string displayed in the window's status bar.
	QString _statusBarText;

	/// The markup text displayed in the window's status bar.
	QString _statusBarTextMarkup;

	/// Used for timed display of status bar texts.
	QTimer _statusBarTimer;

	/// A Qt list model with all available modifier types.
	ModifierListModel* _modifierListModel;

	/// A Qt list model for the items in the selected data pipeline.
	PipelineListModel* _pipelineListModel;
};

}	// End of namespace
