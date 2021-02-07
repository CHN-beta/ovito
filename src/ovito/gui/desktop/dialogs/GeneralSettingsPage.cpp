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

#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/base/mainwin/ModifierListModel.h>
#include <ovito/opengl/OpenGLSceneRenderer.h>
#include "GeneralSettingsPage.h"

namespace Ovito {

IMPLEMENT_OVITO_CLASS(GeneralSettingsPage);

/******************************************************************************
* Creates the widget that contains the plugin specific setting controls.
******************************************************************************/
void GeneralSettingsPage::insertSettingsDialogPage(ApplicationSettingsDialog* settingsDialog, QTabWidget* tabWidget)
{
	QWidget* page = new QWidget();
	tabWidget->addTab(page, tr("General"));
	QVBoxLayout* layout1 = new QVBoxLayout(page);

	QSettings settings;

	QGroupBox* uiGroupBox = new QGroupBox(tr("User interface"), page);
	layout1->addWidget(uiGroupBox);
	QGridLayout* layout2 = new QGridLayout(uiGroupBox);

	_useQtFileDialog = new QCheckBox(tr("Load file: Use alternative file selection dialog"));
	_useQtFileDialog->setToolTip(tr(
			"<p>Use an alternative file selection dialog instead of the native dialog box provided by the operating system.</p>"));
	layout2->addWidget(_useQtFileDialog, 0, 0);
	_useQtFileDialog->setChecked(settings.value("file/use_qt_dialog", false).toBool());

	_sortModifiersByCategory = new QCheckBox(tr("Modifiers list: Sort by category"));
	_sortModifiersByCategory->setToolTip(tr("<p>Show categorized list of available modifiers in command panel.</p>"));
	layout2->addWidget(_sortModifiersByCategory, 1, 0);
	_sortModifiersByCategory->setChecked(ModifierListModel::useCategoriesGlobal());

#if !defined(OVITO_BUILD_APPSTORE_VERSION)
	QGroupBox* updateGroupBox = new QGroupBox(tr("Program updates"), page);
	layout1->addWidget(updateGroupBox);
	layout2 = new QGridLayout(updateGroupBox);

	_enableUpdateChecks = new QCheckBox(tr("Auto-refresh news page from web server"), updateGroupBox);
	_enableUpdateChecks->setToolTip(tr(
			"<p>The news page is fetched from <i>www.ovito.org</i> and displayed on each program startup. "
			"It contains information about new program updates when they become available.</p>"));
	layout2->addWidget(_enableUpdateChecks, 0, 0);
	_enableUsageStatistics = new QCheckBox(tr("Send unique installation ID to web server"), updateGroupBox);
	_enableUsageStatistics->setToolTip(tr(
			"<p>Every installation of OVITO has a unique identifier, which is generated on first program start. "
			"This option enables the transmission of the anonymous identifier to the web server to help the developers collect "
			"program usage statistics.</p>"));
	layout2->addWidget(_enableUsageStatistics, 1, 0);

	_enableUpdateChecks->setChecked(settings.value("updates/check_for_updates", true).toBool());
	_enableUsageStatistics->setChecked(settings.value("updates/transmit_id", true).toBool());

	connect(_enableUpdateChecks, &QCheckBox::toggled, _enableUsageStatistics, &QCheckBox::setEnabled);
	_enableUsageStatistics->setEnabled(_enableUpdateChecks->isChecked());
#endif

	layout1->addStretch();
}

/******************************************************************************
* Lets the page save all changed settings.
******************************************************************************/
bool GeneralSettingsPage::saveValues(ApplicationSettingsDialog* settingsDialog, QTabWidget* tabWidget)
{
	QSettings settings;
	settings.setValue("file/use_qt_dialog", _useQtFileDialog->isChecked());
	ModifierListModel::setUseCategoriesGlobal(_sortModifiersByCategory->isChecked());
#if !defined(OVITO_BUILD_APPSTORE_VERSION)
	settings.setValue("updates/check_for_updates", _enableUpdateChecks->isChecked());
	settings.setValue("updates/transmit_id", _enableUsageStatistics->isChecked());
#endif
	return true;
}

}	// End of namespace
