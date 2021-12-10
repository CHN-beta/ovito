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
#include <ovito/core/dataset/pipeline/PipelineFlowState.h>
#include <ovito/core/dataset/data/AttributeDataObject.h>
#include <ovito/core/dataset/io/AttributeFileExporter.h>
#include <ovito/gui/desktop/dialogs/FileExporterSettingsDialog.h>
#include <ovito/gui/desktop/dialogs/HistoryFileDialog.h>
#include <ovito/gui/desktop/utilities/concurrent/ProgressDialog.h>
#include <ovito/gui/desktop/mainwin/MainWindow.h>
#include "GlobalAttributesInspectionApplet.h"

namespace Ovito {

IMPLEMENT_OVITO_CLASS(GlobalAttributesInspectionApplet);

/******************************************************************************
* Determines whether the given pipeline dataset contains data that can be
* displayed by this applet.
******************************************************************************/
bool GlobalAttributesInspectionApplet::appliesTo(const DataCollection& data)
{
	return data.containsObject<AttributeDataObject>();
}

/******************************************************************************
* Lets the applet create the UI widget that is to be placed into the data
* inspector panel.
******************************************************************************/
QWidget* GlobalAttributesInspectionApplet::createWidget(MainWindow* mainWindow)
{
	_mainWindow = mainWindow;

	QWidget* panel = new QWidget();
	QHBoxLayout* layout = new QHBoxLayout(panel);
	layout->setContentsMargins(0,0,0,0);
	layout->setSpacing(0);

	QToolBar* toolbar = new QToolBar();
	toolbar->setOrientation(Qt::Vertical);
	toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
	toolbar->setIconSize(QSize(22,22));
	toolbar->setStyleSheet("QToolBar { padding: 0px; margin: 0px; border: 0px none black; spacing: 0px; }");

	QAction* exportToFileAction = new QAction(QIcon(":/guibase/actions/file/file_save_as.bw.svg"), tr("Export attributes to text file"), this);
	connect(exportToFileAction, &QAction::triggered, this, &GlobalAttributesInspectionApplet::exportToFile);
	toolbar->addAction(exportToFileAction);

	_tableView = new TableView();
	_tableModel = new AttributeTableModel(_tableView);
	_tableView->setModel(_tableModel);
	_tableView->verticalHeader()->hide();
	_tableView->horizontalHeader()->resizeSection(0, 180);
	_tableView->horizontalHeader()->setStretchLastSection(true);

	layout->addWidget(_tableView, 1);
	layout->addWidget(toolbar, 0);

	return panel;
}

/******************************************************************************
* Updates the contents displayed in the inspector.
******************************************************************************/
void GlobalAttributesInspectionApplet::updateDisplay(const PipelineFlowState& state, PipelineSceneNode* pipeline)
{
	DataInspectionApplet::updateDisplay(state, pipeline);
	_tableModel->setContents(state.data());
}

/******************************************************************************
* Selects a specific data object in this applet.
******************************************************************************/
bool GlobalAttributesInspectionApplet::selectDataObject(PipelineObject* dataSource, const QString& objectIdentifierHint, const QVariant& modeHint)
{
	for(size_t i = 0; i < _tableModel->attributes().size(); i++) {
		const auto& attr = _tableModel->attributes()[i];
		if(attr->dataSource() == dataSource) {
			if(objectIdentifierHint.isEmpty() || attr->identifier().startsWith(objectIdentifierHint)) {
				// Note: Defer selecting the table row to a somewhat later time, because QTableView only accepts
				// selection calls when it's visible and after the parent widget has been enabled.
				QTimer::singleShot(0, this, [this,i]() {
					_tableView->selectRow(i);
				});
				return true;
			}
		}
	}
	return false;
}

/******************************************************************************
* Exports the global attributes to a text file.
******************************************************************************/
void GlobalAttributesInspectionApplet::exportToFile()
{
	if(!currentPipeline())
		return;

	// Let the user select a destination file.
	HistoryFileDialog dialog("export", _mainWindow, tr("Export Attributes"));
	QString filterString = QStringLiteral("%1 (%2)").arg(AttributeFileExporter::OOClass().fileFilterDescription(), AttributeFileExporter::OOClass().fileFilter());
	dialog.setNameFilter(filterString);
	dialog.setOption(QFileDialog::DontUseNativeDialog);
	dialog.setAcceptMode(QFileDialog::AcceptSave);
	dialog.setFileMode(QFileDialog::AnyFile);

	// Go to the last directory used.
	QSettings settings;
	settings.beginGroup("file/export");
	QString lastExportDirectory = settings.value("last_export_dir").toString();
	if(!lastExportDirectory.isEmpty())
		dialog.setDirectory(lastExportDirectory);

	if(!dialog.exec() || dialog.selectedFiles().empty())
		return;
	QString exportFile = dialog.selectedFiles().front();

	// Remember directory for the next time...
	settings.setValue("last_export_dir", dialog.directory().absolutePath());

	// Export to selected file.
	try {
		// Create exporter service.
		OORef<AttributeFileExporter> exporter = OORef<AttributeFileExporter>::create(currentPipeline()->dataset(), ObjectInitializationHint::LoadUserDefaults);

		// Pass output filename to exporter.
		exporter->setOutputFilename(exportFile);

		// Set scene node to be exported.
		exporter->setNodeToExport(currentPipeline());

		// Let the user adjust the export settings.
		FileExporterSettingsDialog settingsDialog(_mainWindow, exporter);
		if(settingsDialog.exec() != QDialog::Accepted)
			return;

		// Show progress dialog.
		ProgressDialog progressDialog(_mainWindow, exporter->dataset()->taskManager(), tr("File export"));

		// Let the exporter do its job.
		exporter->doExport(progressDialog.createOperation());
	}
	catch(const Exception& ex) {
		ex.reportError();
	}
}

}	// End of namespace
