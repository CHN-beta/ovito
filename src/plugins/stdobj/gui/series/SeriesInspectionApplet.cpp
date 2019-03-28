///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#include <plugins/stdobj/gui/StdObjGui.h>
#include <plugins/stdobj/io/DataSeriesExporter.h>
#include <gui/mainwin/MainWindow.h>
#include <gui/dialogs/FileExporterSettingsDialog.h>
#include <gui/dialogs/HistoryFileDialog.h>
#include <gui/utilities/concurrent/ProgressDialog.h>
#include "SeriesInspectionApplet.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(SeriesInspectionApplet);

/******************************************************************************
* Lets the applet create the UI widget that is to be placed into the data
* inspector panel.
******************************************************************************/
QWidget* SeriesInspectionApplet::createWidget(MainWindow* mainWindow)
{
	createBaseWidgets();
	_mainWindow = mainWindow;

	QSplitter* splitter = new QSplitter();
	splitter->addWidget(containerSelectionWidget());

	QWidget* rightContainer = new QWidget();
	splitter->addWidget(rightContainer);
	splitter->setStretchFactor(0, 1);
	splitter->setStretchFactor(1, 4);

	QHBoxLayout* rightLayout = new QHBoxLayout(rightContainer);
	rightLayout->setContentsMargins(0,0,0,0);
	rightLayout->setSpacing(0);
	QToolBar* toolbar = new QToolBar();
	toolbar->setOrientation(Qt::Vertical);
	toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
	toolbar->setIconSize(QSize(22,22));
	toolbar->setStyleSheet("QToolBar { padding: 0px; margin: 0px; border: 0px none black; spacing: 0px; }");

	_exportSeriesToFileAction = new QAction(QIcon(":/gui/actions/file/file_save_as.bw.svg"), tr("Export data to file"), this);
	connect(_exportSeriesToFileAction, &QAction::triggered, this, &SeriesInspectionApplet::exportDataToFile);
	toolbar->addAction(_exportSeriesToFileAction);

	QStackedWidget* stackedWidget = new QStackedWidget();
	rightLayout->addWidget(stackedWidget, 1);
	rightLayout->addWidget(toolbar, 0);

	_plotWidget = new DataSeriesPlotWidget();
	stackedWidget->addWidget(_plotWidget);
	stackedWidget->addWidget(tableView());

	return splitter;
}

/******************************************************************************
* Is called when the user selects a different container object from the list.
******************************************************************************/
void SeriesInspectionApplet::currentContainerChanged()
{
	PropertyInspectionApplet::currentContainerChanged();

	// Update the displayed plot.
	plotWidget()->setSeries(static_object_cast<DataSeriesObject>(selectedContainerObject()));

	// Update actions.
	_exportSeriesToFileAction->setEnabled(plotWidget()->series() != nullptr);
}

/******************************************************************************
* Exports the current data series to a text file.
******************************************************************************/
void SeriesInspectionApplet::exportDataToFile()
{
	const DataSeriesObject* series = plotWidget()->series();
	if(!series)
		return;

	// Let the user select a destination file.
	HistoryFileDialog dialog("export", _mainWindow, tr("Export Data Series"));
#ifndef Q_OS_WIN
	QString filterString = QStringLiteral("%1 (%2)").arg(DataSeriesExporter::OOClass().fileFilterDescription(), DataSeriesExporter::OOClass().fileFilter());
#else
		// Workaround for bug in Windows file selection dialog (https://bugreports.qt.io/browse/QTBUG-45759)
	QString filterString = QStringLiteral("%1 (*)").arg(DataSeriesExporter::OOClass().fileFilterDescription());
#endif
	dialog.setNameFilter(filterString);
	dialog.setAcceptMode(QFileDialog::AcceptSave);
	dialog.setFileMode(QFileDialog::AnyFile);
	dialog.setConfirmOverwrite(true);

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
		OORef<DataSeriesExporter> exporter = new DataSeriesExporter(series->dataset());

		// Load user-defined default settings.
		exporter->loadUserDefaults();

		// Pass output filename to exporter.
		exporter->setOutputFilename(exportFile);

		// Set scene node to be exported.
		exporter->setNodeToExport(currentSceneNode());

		// Set data series to be exported.
		exporter->setDataObjectToExport(DataObjectReference(&DataSeriesObject::OOClass(), series->identifier(), series->title()));

		// Let the user adjust the export settings.
		FileExporterSettingsDialog settingsDialog(_mainWindow, exporter);
		if(settingsDialog.exec() != QDialog::Accepted)
			return;

		// Show progress dialog.
		ProgressDialog progressDialog(_mainWindow, tr("File export"));

		// Let the exporter do its job.
		exporter->doExport(progressDialog.taskManager().createMainThreadOperation<>(true));
	}
	catch(const Exception& ex) {
		ex.reportError();
	}
}

}	// End of namespace
}	// End of namespace
