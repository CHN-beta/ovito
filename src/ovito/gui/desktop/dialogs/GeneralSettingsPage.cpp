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
#include <ovito/gui/desktop/mainwin/cmdpanel/ModifierListModel.h>
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

	QGroupBox* openglGroupBox = new QGroupBox(tr("Viewport rendering / OpenGL"));
	layout1->addWidget(openglGroupBox);
	layout2 = new QGridLayout(openglGroupBox);

	// OpenGL context sharing:
	_overrideGLContextSharing = new QCheckBox(tr("Override context sharing"), openglGroupBox);
	_overrideGLContextSharing->setToolTip(tr("<p>Activate this option to explicitly control the sharing of OpenGL contexts between viewport windows.</p>"));
	layout2->addWidget(_overrideGLContextSharing, 0, 0);
	_contextSharingMode = new QComboBox(openglGroupBox);
	_contextSharingMode->setEnabled(false);
	if(OpenGLSceneRenderer::contextSharingEnabled(true)) {
		_contextSharingMode->addItem(tr("Enable sharing (default)"));
		_contextSharingMode->addItem(tr("Disable sharing"));
	}
	else {
		_contextSharingMode->addItem(tr("Enable sharing"));
		_contextSharingMode->addItem(tr("Disable sharing (default)"));
	}
	layout2->addWidget(_contextSharingMode, 0, 1);
	connect(_overrideGLContextSharing, &QCheckBox::toggled, _contextSharingMode, &QComboBox::setEnabled);
	_overrideGLContextSharing->setChecked(settings.contains("display/share_opengl_context"));
	_contextSharingMode->setCurrentIndex(OpenGLSceneRenderer::contextSharingEnabled() ? 0 : 1);

	// OpenGL point sprites:
	_overrideUseOfPointSprites = new QCheckBox(tr("Override usage of point sprites"), openglGroupBox);
	_overrideUseOfPointSprites->setToolTip(tr("<p>Activate this option to explicitly control the usage of OpenGL point sprites for rendering of particles.</p>"));
	layout2->addWidget(_overrideUseOfPointSprites, 1, 0);
	_pointSpriteMode = new QComboBox(openglGroupBox);
	_pointSpriteMode->setEnabled(false);
	if(OpenGLSceneRenderer::pointSpritesEnabled(true)) {
		_pointSpriteMode->addItem(tr("Use point sprites (default)"));
		_pointSpriteMode->addItem(tr("Don't use point sprites"));
	}
	else {
		_pointSpriteMode->addItem(tr("Use point sprites"));
		_pointSpriteMode->addItem(tr("Don't use point sprites (default)"));
	}
	layout2->addWidget(_pointSpriteMode, 1, 1);
	connect(_overrideUseOfPointSprites, &QCheckBox::toggled, _pointSpriteMode, &QComboBox::setEnabled);
	_overrideUseOfPointSprites->setChecked(settings.contains("display/use_point_sprites"));
	_pointSpriteMode->setCurrentIndex(OpenGLSceneRenderer::pointSpritesEnabled() ? 0 : 1);

	// OpenGL geometry shaders:
	_overrideUseOfGeometryShaders = new QCheckBox(tr("Override usage of geometry shaders"), openglGroupBox);
	_overrideUseOfGeometryShaders->setToolTip(tr("<p>Activate this option to explicitly control the usage of OpenGL geometry shaders.</p>"));
	layout2->addWidget(_overrideUseOfGeometryShaders, 2, 0);
	_geometryShaderMode = new QComboBox(openglGroupBox);
	_geometryShaderMode->setEnabled(false);
	if(OpenGLSceneRenderer::geometryShadersEnabled(true)) {
		_geometryShaderMode->addItem(tr("Use geometry shaders (default)"));
		_geometryShaderMode->addItem(tr("Don't use geometry shaders"));
	}
	else {
		_geometryShaderMode->addItem(tr("Use geometry shaders"));
		_geometryShaderMode->addItem(tr("Don't use geometry shaders (default)"));
	}
	layout2->addWidget(_geometryShaderMode, 2, 1);
	connect(_overrideUseOfGeometryShaders, &QCheckBox::toggled, _geometryShaderMode, &QComboBox::setEnabled);
	_overrideUseOfGeometryShaders->setChecked(settings.contains("display/use_geometry_shaders"));
	_geometryShaderMode->setCurrentIndex(OpenGLSceneRenderer::geometryShadersEnabled() ? 0 : 1);

	_restartRequiredLabel = new QLabel(tr("<p style=\"font-size: small; color: #DD2222;\">Restart required for changes to take effect.</p>"));
	_restartRequiredLabel->hide();
	layout2->addWidget(_restartRequiredLabel, 3, 0, 1, 2);
	connect(_overrideGLContextSharing, &QCheckBox::toggled, _restartRequiredLabel, &QLabel::show);
	connect(_contextSharingMode, &QComboBox::currentTextChanged, _restartRequiredLabel, &QLabel::show);
	connect(_overrideUseOfPointSprites, &QCheckBox::toggled, _restartRequiredLabel, &QLabel::show);
	connect(_pointSpriteMode, &QComboBox::currentTextChanged, _restartRequiredLabel, &QLabel::show);
	connect(_overrideUseOfGeometryShaders, &QCheckBox::toggled, _restartRequiredLabel, &QLabel::show);
	connect(_geometryShaderMode, &QComboBox::currentTextChanged, _restartRequiredLabel, &QLabel::show);

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
	if(_overrideGLContextSharing->isChecked())
		settings.setValue("display/share_opengl_context", _contextSharingMode->currentIndex() == 0);
	else
		settings.remove("display/share_opengl_context");
	if(_overrideUseOfPointSprites->isChecked())
		settings.setValue("display/use_point_sprites", _pointSpriteMode->currentIndex() == 0);
	else
		settings.remove("display/use_point_sprites");
	if(_overrideUseOfGeometryShaders->isChecked())
		settings.setValue("display/use_geometry_shaders", _geometryShaderMode->currentIndex() == 0);
	else
		settings.remove("display/use_geometry_shaders");
	return true;
}

}	// End of namespace
