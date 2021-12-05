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

#include <ovito/core/Core.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include <ovito/core/dataset/scene/SelectionSet.h>
#include <ovito/core/dataset/data/AttributeDataObject.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include "AttributeFileExporter.h"

namespace Ovito {

IMPLEMENT_OVITO_CLASS(AttributeFileExporter);

/******************************************************************************
 * This is called once for every output file to be written and before
 * exportData() is called.
*****************************************************************************/
bool AttributeFileExporter::openOutputFile(const QString& filePath, int numberOfFrames, SynchronousOperation operation)
{
	OVITO_ASSERT(!_outputFile.isOpen());
	OVITO_ASSERT(!_outputStream);

	_outputFile.setFileName(filePath);
	_outputStream = std::make_unique<CompressedTextWriter>(_outputFile, dataset());

	textStream() << "#";
	for(const QString& attrName : attributesToExport()) {
		textStream() << " \"" << attrName << "\"";
	}
	textStream() << "\n";

	return true;
}

/******************************************************************************
 * This is called once for every output file written after exportData()
 * has been called.
*****************************************************************************/
void AttributeFileExporter::closeOutputFile(bool exportCompleted)
{
	_outputStream.reset();
	if(_outputFile.isOpen())
		_outputFile.close();

	if(!exportCompleted)
		_outputFile.remove();
}

/******************************************************************************
* Initializes the object's parameter fields with default values and loads 
* user-defined default values from the application's settings store (GUI only).
*****************************************************************************/
void AttributeFileExporter::initializeObject(ObjectInitializationHints hints)
{
	if(hints.testFlag(LoadUserDefaults)) {
		// This exporter is typically used to export attributes as functions of time.
		if(dataset()->animationSettings()->animationInterval().duration() != 0)
			setExportAnimation(true);

#ifndef OVITO_DISABLE_QSETTINGS
		// Restore last output column mapping.
		QSettings settings;
		settings.beginGroup("exporter/attributes/");
		setAttributesToExport(settings.value("attrlist", QVariant::fromValue(QStringList())).toStringList());
		settings.endGroup();
#endif
	}

	FileExporter::initializeObject(hints);
}

/******************************************************************************
* Evaluates the pipeline of the PipelineSceneNode to be exported and returns
* the attributes list.
******************************************************************************/
bool AttributeFileExporter::getAttributesMap(TimePoint time, QVariantMap& attributes, SynchronousOperation operation)
{
	const PipelineFlowState& state = getPipelineDataToBeExported(time, operation.subOperation());
	if(operation.isCanceled())
		return false;

	// Build list of attributes.
	attributes = state.data()->buildAttributesMap();

	// Add the implicit animation frame attribute.
	attributes.insert(QStringLiteral("Frame"), dataset()->animationSettings()->timeToFrame(time));

	return true;
}

/******************************************************************************
 * Exports a single animation frame to the current output file.
 *****************************************************************************/
bool AttributeFileExporter::exportFrame(int frameNumber, TimePoint time, const QString& filePath, SynchronousOperation operation)
{
	QVariantMap attrMap;
	if(!getAttributesMap(time, attrMap, operation.subOperation()))
		return false;

	// Write the values of all attributes marked for export to the output file.
	for(const QString& attrName : attributesToExport()) {
		if(!attrMap.contains(attrName))
			throwException(tr("The global attribute '%1' to be exported is not available at animation frame %2.").arg(attrName).arg(frameNumber));
		QString str = attrMap.value(attrName).toString();

		// Put string in quotes if it contains whitespace.
		if(!str.contains(QChar(' ')))
			textStream() << str << " ";
		else
			textStream() << "\"" << str << "\" ";
	}
	textStream() << "\n";

	return !operation.isCanceled();
}

}	// End of namespace
