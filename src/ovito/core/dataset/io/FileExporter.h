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


#include <ovito/core/Core.h>
#include <ovito/core/oo/RefTarget.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/scene/SceneNode.h>
#include <ovito/core/dataset/data/DataObjectReference.h>

namespace Ovito {

/**
 * \brief A meta-class for file exporters (i.e. classes derived from FileExporter).
 */
class OVITO_CORE_EXPORT FileExporterClass : public RefTarget::OOMetaClass
{
public:

	/// Inherit standard constructor from base meta class.
	using RefTarget::OOMetaClass::OOMetaClass;

	/// \brief Returns the filename filter that specifies the file extension that can be exported by this service.
	/// \return A wild-card pattern for the file types that can be produced by the FileExporter class (e.g. \c "*.xyz" or \c "*").
	virtual QString fileFilter() const {
		OVITO_ASSERT_MSG(false, "FileExporterClass::fileFilter()", "This method should be overridden by a meta-subclass of FileExporterClass.");
		return {};
	}

	/// \brief Returns the file type description that is displayed in the drop-down box of the export file dialog.
	/// \return A human-readable string describing the file format written by the FileExporter class.
	virtual QString fileFilterDescription() const {
		OVITO_ASSERT_MSG(false, "FileExporterClass::fileFilterDescription()", "This method should be overridden by a meta-subclass of FileExporterClass.");
		return {};
	}
};

/**
 * \brief Abstract base class for file writers that export data from OVITO to an external file in a specific format.
 */
class OVITO_CORE_EXPORT FileExporter : public RefTarget
{
	OVITO_CLASS_META(FileExporter, FileExporterClass)

public:

	/// Initializes the object's parameter fields with default values and loads 
	/// user-defined default values from the application's settings store (GUI only).
	virtual void initializeObject(ObjectInitializationHints hints) override;

	/// \brief Selects the default scene node to be exported by this exporter.
	virtual void selectDefaultExportableData();

	/// \brief Determines whether the given scene node is suitable for exporting with this exporter service.
	/// By default, all pipeline scene nodes are considered suitable that produce suitable data objects
	/// of the type(s) specified by the FileExporter::exportableDataObjectClass() method.
	/// Subclasses can refine this behavior as needed.
	virtual bool isSuitableNode(SceneNode* node) const;

	/// \brief Determines whether the given pipeline output is suitable for exporting with this exporter service.
	/// By default, all data collections are considered suitable that contain suitable data objects
	/// of the type(s) specified by the FileExporter::exportableDataObjectClass() method.
	/// Subclasses can refine this behavior as needed.
	virtual bool isSuitablePipelineOutput(const PipelineFlowState& state) const;

	/// \brief Returns the specific type(s) of data objects that this exporter service can export.
	/// The default implementation returns an empty list to indicate that the exporter is not restricted to
	/// a specfic class of data objects. Subclasses should override this behavior.
	virtual std::vector<DataObjectClassPtr> exportableDataObjectClass() const { return {}; }

	/// \brief Sets the name of the output file that should be written by this exporter.
	virtual void setOutputFilename(const QString& filename);

	/// \brief Exports the scene data to the output file(s).
	/// \return \c true if the output file has been successfully written;
	///         \c false if the export operation has been canceled by the user.
	/// \throws Util::Exception if the export operation has failed due to an error.
	virtual bool doExport(SynchronousOperation operation);

	/// Helper function that is called by sub-classes prior to file output in order to
	/// activate the default "C" locale.
	static void activateCLocale();

	/// \brief Indicates whether this file exporter can write more than one animation frame into a single output file.
	virtual bool supportsMultiFrameFiles() const { return false; }

	/// \brief Evaluates the pipeline whose data is to be exported.
	PipelineFlowState getPipelineDataToBeExported(TimePoint time, SynchronousOperation operation, bool requestRenderState = false) const;

	/// \brief Returns a string with the list of available data objects of the given type.
	QString getAvailableDataObjectList(const PipelineFlowState& state, const DataObject::OOMetaClass& objectType) const;

protected:

	/// Initializes the object.
	FileExporter(DataSet* dataset);

	/// \brief This is called once for every output file to be written and before exportFrame() is called.
	virtual bool openOutputFile(const QString& filePath, int numberOfFrames, SynchronousOperation operation) = 0;

	/// \brief This is called once for every output file written after exportFrame() has been called.
	virtual void closeOutputFile(bool exportCompleted) = 0;

	/// \brief Exports a single animation frame to the current output file.
	virtual bool exportFrame(int frameNumber, TimePoint time, const QString& filePath, SynchronousOperation operation);

private:

	/// The output file path.
	DECLARE_PROPERTY_FIELD(QString, outputFilename);

	/// Controls whether only the current animation frame or an entire animation interval should be exported.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, exportAnimation, setExportAnimation);

	/// Indicates that the exporter should produce a separate file for each timestep.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, useWildcardFilename, setUseWildcardFilename);

	/// The wildcard name that is used to generate the output filenames.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, wildcardFilename, setWildcardFilename);

	/// The first animation frame that should be exported.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, startFrame, setStartFrame);

	/// The last animation frame that should be exported.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, endFrame, setEndFrame);

	/// Controls the interval between exported frames.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, everyNthFrame, setEveryNthFrame);

	/// Controls the desired precision with which floating-point numbers are written if the format is text-based.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, floatOutputPrecision, setFloatOutputPrecision);

	/// The scene node to be exported.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(OORef<SceneNode>, nodeToExport, setNodeToExport, PROPERTY_FIELD_NO_SUB_ANIM);

	/// The specific data object from the pipeline output to be exported.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(DataObjectReference, dataObjectToExport, setDataObjectToExport);

	/// Whether to ignore pipeline errors or not during export.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, ignorePipelineErrors, setIgnorePipelineErrors);
};

}	// End of namespace
