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

#include <ovito/particles/gui/ParticlesGui.h>
#include <ovito/particles/import/lammps/LAMMPSTextDumpImporter.h>
#include <ovito/stdobj/gui/properties/InputColumnMappingDialog.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include <ovito/gui/desktop/properties/BooleanRadioButtonParameterUI.h>
#include <ovito/gui/desktop/utilities/concurrent/ProgressDialog.h>
#include <ovito/gui/desktop/mainwin/MainWindow.h>
#include <ovito/core/dataset/io/FileSource.h>
#include "LAMMPSTextDumpImporterEditor.h"

namespace Ovito::Particles {

IMPLEMENT_OVITO_CLASS(LAMMPSTextDumpImporterEditor);
SET_OVITO_OBJECT_EDITOR(LAMMPSTextDumpImporter, LAMMPSTextDumpImporterEditor);

/******************************************************************************
 * Displays a dialog box that allows the user to edit the custom file column to particle
 * property mapping.
 *****************************************************************************/
bool LAMMPSTextDumpImporterEditor::showEditColumnMappingDialog(LAMMPSTextDumpImporter* importer, const FileSourceImporter::Frame& frame)
{
	// Read the list of columns from the file's header.
	Future<ParticleInputColumnMapping> inspectFuture = importer->inspectFileHeader(frame);

	// Block UI until reading is done.
	{
		ProgressDialog progressDialog(parentWindow(), mainWindow(), tr("Inspecting file header"));
		if(!progressDialog.waitForFuture(inspectFuture))
			return false;
	}

	ParticleInputColumnMapping mapping = inspectFuture.result();

	if(!importer->customColumnMapping().empty()) {
		ParticleInputColumnMapping customMapping = importer->customColumnMapping();
		customMapping.resize(mapping.size());
		for(size_t i = 0; i < customMapping.size(); i++)
			customMapping[i].columnName = mapping[i].columnName;
		mapping = std::move(customMapping);
	}

	InputColumnMappingDialog dialog(mapping, parentWindow());
	if(dialog.exec() == QDialog::Accepted) {
		importer->setCustomColumnMapping(dialog.mapping());
		importer->setUseCustomColumnMapping(true);
		return true;
	}
	else {
		return false;
	}
}

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void LAMMPSTextDumpImporterEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("LAMMPS dump reader"), rolloutParams);

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGroupBox* optionsBox = new QGroupBox(tr("Options"), rollout);
	QVBoxLayout* sublayout = new QVBoxLayout(optionsBox);
	sublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(optionsBox);

	// Multi-timestep file
	BooleanParameterUI* multitimestepUI = new BooleanParameterUI(this, PROPERTY_FIELD(FileSourceImporter::isMultiTimestepFile));
	// The following signal handler updates the parameter UI whenever the isMultiTimestepFile parameter of the current file source importer changes.
	// It is needed, because target-changed messages are surpressed for this property field and the normal update mechanism for the parameter UI doesn't work.
	connect(this, &PropertiesEditor::contentsReplaced, this, [con = QMetaObject::Connection(), multitimestepUI = multitimestepUI](RefTarget* editObject) mutable {
#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable : 4573)
#endif
		disconnect(con);
		con = editObject ? connect(static_object_cast<FileSourceImporter>(editObject), &FileSourceImporter::isMultiTimestepFileChanged, multitimestepUI, &ParameterUI::updateUI) : QMetaObject::Connection();
#ifdef _MSC_VER
	#pragma warning(pop)
#endif
	});
	sublayout->addWidget(multitimestepUI->checkBox());

	// Sort particles
	BooleanParameterUI* sortParticlesUI = new BooleanParameterUI(this, PROPERTY_FIELD(ParticleImporter::sortParticles));
	sublayout->addWidget(sortParticlesUI->checkBox());

	QGroupBox* columnMappingBox = new QGroupBox(tr("File columns"), rollout);
	sublayout = new QVBoxLayout(columnMappingBox);
	sublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(columnMappingBox);

	BooleanRadioButtonParameterUI* useCustomMappingUI = new BooleanRadioButtonParameterUI(this, PROPERTY_FIELD(LAMMPSTextDumpImporter::useCustomColumnMapping));
	useCustomMappingUI->buttonFalse()->setText(tr("Automatic mapping"));
	sublayout->addWidget(useCustomMappingUI->buttonFalse());
	useCustomMappingUI->buttonTrue()->setText(tr("User-defined mapping to particle properties"));
	sublayout->addWidget(useCustomMappingUI->buttonTrue());
	connect(useCustomMappingUI->buttonFalse(), &QRadioButton::clicked, this, [this]() {
		if(LAMMPSTextDumpImporter* importer = static_object_cast<LAMMPSTextDumpImporter>(editObject()))
			importer->requestReload();
	}, Qt::QueuedConnection);

	QPushButton* editMappingButton = new QPushButton(tr("Edit column mapping..."));
	sublayout->addWidget(editMappingButton);
	connect(editMappingButton, &QPushButton::clicked, this, &LAMMPSTextDumpImporterEditor::onEditColumnMapping);
}

/******************************************************************************
* Is called when the user pressed the "Edit column mapping" button.
******************************************************************************/
void LAMMPSTextDumpImporterEditor::onEditColumnMapping()
{
	if(LAMMPSTextDumpImporter* importer = static_object_cast<LAMMPSTextDumpImporter>(editObject())) {
		UndoableTransaction::handleExceptions(importer->dataset()->undoStack(), tr("Change file column mapping"), [this, importer]() {

			// Determine the currently loaded data file of the FileSource.
			FileSource* fileSource = importer->fileSource();
			if(!fileSource || fileSource->frames().empty()) return;
			int frameIndex = qBound(0, fileSource->dataCollectionFrame(), fileSource->frames().size() - 1);

			// Show the dialog box, which lets the user modify the file column mapping.
			if(showEditColumnMappingDialog(importer, fileSource->frames()[frameIndex])) {
				importer->requestReload();
			}
		});
	}
}

}	// End of namespace
