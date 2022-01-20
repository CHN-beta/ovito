////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 OVITO GmbH, Germany
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


#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/desktop/properties/PropertiesEditor.h>
#include <ovito/gui/desktop/properties/PropertiesPanel.h>
#include <ovito/gui/desktop/widgets/general/ElidedTextLabel.h>
#include <ovito/gui/desktop/widgets/display/StatusWidget.h>

namespace Ovito {

/**
 * A properties editor for the FileSource object.
 */
class FileSourceEditor : public PropertiesEditor
{
	OVITO_CLASS(FileSourceEditor)

public:

	/// Default constructor.
	Q_INVOKABLE FileSourceEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

	/// Loads a new file into the FileSource.
	bool importNewFile(FileSource* fileSource, const QUrl& url, OvitoClassPtr importerType, MainThreadOperation operation);

protected Q_SLOTS:

	/// Is called when the user presses the "Pick local input file" button.
	void onPickLocalInputFile();

	/// Is called when the user presses the "Pick remote input file" button.
	void onPickRemoteInputFile();

	/// Is called when the user presses the Reload frame button.
	void onReloadFrame();

	/// Is called when the user presses the Reload animation button.
	void onReloadAnimation();

	/// Updates the displayed status information.
	void updateInformationLabel();

	/// Updates the list of trajectory frames displayed in the UI.
	void updateFramesList();

	/// This is called when the user has changed the source URL.
	void onWildcardPatternEntered();

	/// Is called when the user has selected a certain frame in the frame list box.
	void onFrameSelected(int index);

private:

	QLineEdit* _filenameLabel;
	QLineEdit* _sourcePathLabel;
	QLineEdit* _wildcardPatternTextbox;
	QLabel* _fileSeriesLabel;
	QLabel* _timeSeriesLabel = nullptr;
	StatusWidget* _statusLabel;
	QComboBox* _framesListBox = nullptr;
	QStringListModel* _framesListModel = nullptr;
	QLabel* _playbackRatioDisplay = nullptr;
};

}	// End of namespace
