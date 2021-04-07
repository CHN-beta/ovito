////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 OVITO GmbH, Germany
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
#include <ovito/gui/desktop/dialogs/ApplicationSettingsDialog.h>
#include <ovito/core/dataset/pipeline/ModifierTemplates.h>

namespace Ovito {

/**
 * Page of the application settings dialog, which allows the user to manage the defined modifier templates.
 */
class OVITO_GUI_EXPORT ModifierTemplatesPage : public ApplicationSettingsDialogPage
{
	Q_OBJECT
	OVITO_CLASS(ModifierTemplatesPage)

public:

	/// Default constructor.
	Q_INVOKABLE ModifierTemplatesPage() = default;

	/// \brief Creates the widget.
	virtual void insertSettingsDialogPage(ApplicationSettingsDialog* settingsDialog, QTabWidget* tabWidget) override;

	/// \brief Lets the settings page to save all values entered by the user.
	virtual bool saveValues(ApplicationSettingsDialog* settingsDialog, QTabWidget* tabWidget) override;

	/// \brief Lets the settings page restore the original values of changed settings when the user presses the Cancel button.
	virtual void restoreValues(ApplicationSettingsDialog* settingsDialog, QTabWidget* tabWidget) override;

	/// \brief Returns an integer value that is used to sort the dialog pages in ascending order.
	virtual int pageSortingKey() const override { return 3; }

private Q_SLOTS:

	/// Is invoked when the user presses the "Create template" button.
	void onCreateTemplate();

	/// Is invoked when the user presses the "Delete template" button.
	void onDeleteTemplate();

	/// Is invoked when the user presses the "Rename template" button.
	void onRenameTemplate();

	/// Is invoked when the user presses the "Export templates" button.
	void onExportTemplates();

	/// Is invoked when the user presses the "Import templates" button.
	void onImportTemplates();

private:

	QListView* _listWidget;
	ApplicationSettingsDialog* _settingsDialog;
	bool _dirtyFlag = false;
};

}	// End of namespace
