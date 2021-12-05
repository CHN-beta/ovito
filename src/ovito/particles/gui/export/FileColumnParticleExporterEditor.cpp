////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2016 OVITO GmbH, Germany
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
#include <ovito/particles/export/FileColumnParticleExporter.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/gui/desktop/utilities/concurrent/ProgressDialog.h>
#include "FileColumnParticleExporterEditor.h"

namespace Ovito::Particles {

SET_OVITO_OBJECT_EDITOR(FileColumnParticleExporter, FileColumnParticleExporterEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void FileColumnParticleExporterEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Particle properties to export"), rolloutParams);
	QGridLayout* columnsGroupBoxLayout = new QGridLayout(rollout);

	_columnMappingWidget = new QListWidget();
	columnsGroupBoxLayout->addWidget(_columnMappingWidget, 0, 0, 5, 1);
	columnsGroupBoxLayout->setRowStretch(2, 1);

	QPushButton* moveUpButton = new QPushButton(tr("Move up"), rollout);
	QPushButton* moveDownButton = new QPushButton(tr("Move down"), rollout);
	QPushButton* selectAllButton = new QPushButton(tr("Select all"), rollout);
	QPushButton* selectNoneButton = new QPushButton(tr("Unselect all"), rollout);
	columnsGroupBoxLayout->addWidget(moveUpButton, 0, 1, 1, 1);
	columnsGroupBoxLayout->addWidget(moveDownButton, 1, 1, 1, 1);
	columnsGroupBoxLayout->addWidget(selectAllButton, 3, 1, 1, 1);
	columnsGroupBoxLayout->addWidget(selectNoneButton, 4, 1, 1, 1);
	moveUpButton->setEnabled(_columnMappingWidget->currentRow() >= 1);
	moveDownButton->setEnabled(_columnMappingWidget->currentRow() >= 0 && _columnMappingWidget->currentRow() < _columnMappingWidget->count() - 1);

	connect(_columnMappingWidget, &QListWidget::itemSelectionChanged, [moveUpButton, moveDownButton, this]() {
		moveUpButton->setEnabled(_columnMappingWidget->currentRow() >= 1);
		moveDownButton->setEnabled(_columnMappingWidget->currentRow() >= 0 && _columnMappingWidget->currentRow() < _columnMappingWidget->count() - 1);
	});

	connect(moveUpButton, &QPushButton::clicked, [this]() {
		int currentIndex = _columnMappingWidget->currentRow();
		QListWidgetItem* currentItem = _columnMappingWidget->takeItem(currentIndex);
		_columnMappingWidget->insertItem(currentIndex - 1, currentItem);
		_columnMappingWidget->setCurrentRow(currentIndex - 1);
		onParticlePropertyItemChanged();
	});

	connect(moveDownButton, &QPushButton::clicked, [this]() {
		int currentIndex = _columnMappingWidget->currentRow();
		QListWidgetItem* currentItem = _columnMappingWidget->takeItem(currentIndex);
		_columnMappingWidget->insertItem(currentIndex + 1, currentItem);
		_columnMappingWidget->setCurrentRow(currentIndex + 1);
		onParticlePropertyItemChanged();
	});

	connect(selectAllButton, &QPushButton::clicked, [this]() {
		for(int index = 0; index < _columnMappingWidget->count(); index++)
			_columnMappingWidget->item(index)->setCheckState(Qt::Checked);
	});

	connect(selectNoneButton, &QPushButton::clicked, [this]() {
		for(int index = 0; index < _columnMappingWidget->count(); index++)
			_columnMappingWidget->item(index)->setCheckState(Qt::Unchecked);
	});

	connect(this, &PropertiesEditor::contentsReplaced, this, &FileColumnParticleExporterEditor::updateParticlePropertiesList);
	connect(_columnMappingWidget, &QListWidget::itemChanged, this, &FileColumnParticleExporterEditor::onParticlePropertyItemChanged);
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool FileColumnParticleExporterEditor::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(source == editObject() && event.type() == ReferenceEvent::ReferenceChanged) {
		if(static_cast<const ReferenceFieldEvent&>(event).field() == PROPERTY_FIELD(FileExporter::nodeToExport)) {
			updateParticlePropertiesList();
		}
	}
	return PropertiesEditor::referenceEvent(source, event);
}

/******************************************************************************
* Updates the displayed list of particle properties that are available for export.
******************************************************************************/
void FileColumnParticleExporterEditor::updateParticlePropertiesList()
{
	_columnMappingWidget->clear();

	FileColumnParticleExporter* exporter = dynamic_object_cast<FileColumnParticleExporter>(editObject());
	if(!exporter) return;

	try {
		// Determine the data that is available for export.
		ProgressDialog progressDialog(container(), exporter->dataset()->taskManager());
		PipelineFlowState state = exporter->getParticleData(exporter->dataset()->animationSettings()->time(), progressDialog.createOperation());
		if(!state)
			throw Exception(tr("Operation has been canceled by the user."));

		bool hasParticleIdentifiers = false;
		const ParticlesObject* particles = state.expectObject<ParticlesObject>();
		for(const PropertyObject* property : particles->properties()) {
			if(property->componentCount() == 1) {
				insertPropertyItem(ParticlePropertyReference(property), property->name(), exporter->columnMapping());
				if(property->type() == ParticlesObject::IdentifierProperty)
					hasParticleIdentifiers = true;
			}
			else {
				for(int vectorComponent = 0; vectorComponent < (int)property->componentCount(); vectorComponent++) {
					QString propertyName = property->nameWithComponent(vectorComponent);
					ParticlePropertyReference propRef(property, vectorComponent);
					insertPropertyItem(propRef, propertyName, exporter->columnMapping());
				}
			}
		}
		if(!hasParticleIdentifiers)
			insertPropertyItem(ParticlesObject::IdentifierProperty, tr("Particle index"), exporter->columnMapping());
	}
	catch(const Exception& ex) {
		// Ignore errors, but display a message in the UI widget to inform user.
		_columnMappingWidget->addItems(ex.messages());
	}

	// Update the settings stored in the exporter to match the current settings in the UI.
	saveChanges(exporter);
}

/******************************************************************************
* Populates the column mapping list box with an entry.
******************************************************************************/
void FileColumnParticleExporterEditor::insertPropertyItem(ParticlePropertyReference propRef, const QString& displayName, const ParticlesOutputColumnMapping& columnMapping)
{
	QListWidgetItem* item = new QListWidgetItem(displayName);
	item->setFlags(Qt::ItemFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren));
	item->setCheckState(Qt::Unchecked);
	item->setData(Qt::UserRole, QVariant::fromValue(static_cast<PropertyReference&>(propRef)));
	int sortKey = columnMapping.size();

	for(int c = 0; c < (int)columnMapping.size(); c++) {
		if(columnMapping[c] == propRef) {
			item->setCheckState(Qt::Checked);
			sortKey = c;
			break;
		}
	}

	item->setData(Qt::InitialSortOrderRole, sortKey);
	if(sortKey < (int)columnMapping.size()) {
		int insertIndex = 0;
		for(; insertIndex < _columnMappingWidget->count(); insertIndex++) {
			int k = _columnMappingWidget->item(insertIndex)->data(Qt::InitialSortOrderRole).value<int>();
			if(sortKey < k)
				break;
		}
		_columnMappingWidget->insertItem(insertIndex, item);
	}
	else {
		_columnMappingWidget->addItem(item);
	}
}

/******************************************************************************
* This writes the settings made in the UI back to the exporter.
******************************************************************************/
void FileColumnParticleExporterEditor::saveChanges(FileColumnParticleExporter* exporter)
{
	ParticlesOutputColumnMapping newMapping;
	for(int index = 0; index < _columnMappingWidget->count(); index++) {
		if(_columnMappingWidget->item(index)->checkState() == Qt::Checked) {
			newMapping.push_back(_columnMappingWidget->item(index)->data(Qt::UserRole).value<PropertyReference>());
		}
	}
	exporter->setColumnMapping(newMapping);
}

/******************************************************************************
* Is called when the user checked/unchecked an item in the particle property list.
******************************************************************************/
void FileColumnParticleExporterEditor::onParticlePropertyItemChanged()
{
	FileColumnParticleExporter* particleExporter = dynamic_object_cast<FileColumnParticleExporter>(editObject());
	if(!particleExporter) return;

	// Stores current settings in exporter object.
	saveChanges(particleExporter);

	// Remember the output column mapping for the next time.
	QSettings settings;
	settings.beginGroup("exporter/particles/");
	settings.setValue("columnmapping", particleExporter->columnMapping().toByteArray(particleExporter->dataset()->taskManager()));
	settings.endGroup();
}

}	// End of namespace
