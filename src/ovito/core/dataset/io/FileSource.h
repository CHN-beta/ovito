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


#include <ovito/core/Core.h>
#include <ovito/core/dataset/pipeline/CachingPipelineObject.h>
#include <ovito/core/dataset/data/DataCollection.h>
#include <ovito/core/utilities/concurrent/Future.h>
#include "FileSourceImporter.h"

namespace Ovito {

/**
 * \brief An object in the data pipeline that reads data from an external file.
 *
 * This class works in concert with the FileSourceImporter class.
 */
class OVITO_CORE_EXPORT FileSource : public CachingPipelineObject
{
	Q_OBJECT
	OVITO_CLASS(FileSource)

public:

	/// Constructor.
	Q_INVOKABLE FileSource(DataSet* dataset);

	/// \brief Determines the time interval over which a computed pipeline state will remain valid.
	virtual TimeInterval validityInterval(const PipelineEvaluationRequest& request) const override;

	/// \brief Sets the source location(s) for importing data.
	/// \param sourceUrls The new source location(s).
	/// \param importer The importer object that will parse the input file.
	/// \param autodetectFileSequences Enables the automatic detection of file sequences.
	/// \return false if the operation has been canceled by the user.
	bool setSource(std::vector<QUrl> sourceUrls, FileSourceImporter* importer, bool autodetectFileSequences);

	/// \brief This triggers a reload of input data from the external file for the given frame or all frames.
	/// \param refetchFiles Clears the remote file cache so that file data will be retreived again from the remote location. 
	/// \param frameIndex The index of the input frame to refresh; or -1 to refresh all frames.
	void reloadFrame(bool refetchFiles, int frameIndex = -1);

	/// \brief Scans the external file source and updates the internal frame list.
	SharedFuture<QVector<FileSourceImporter::Frame>> updateListOfFrames(bool refetchCurrentFile = false);

	/// \brief Returns the source frame that is currently used as a sub-object data collection.
	int dataCollectionFrame() const { return _dataCollectionFrame; }

	/// \brief Returns the list of animation frames in the input file(s).
	const QVector<FileSourceImporter::Frame>& frames() const { return _frames; }

	/// Returns the number of different source files in which the trajectory frames are stored. 
	int numberOfFiles() const { return _numberOfFiles; }

	/// \brief Returns the number of animation frames this pipeline object can provide.
	virtual int numberOfSourceFrames() const override;

	/// \brief Given an animation time, computes the source frame to show.
	virtual int animationTimeToSourceFrame(TimePoint time) const override;

	/// \brief Given a source frame index, returns the animation time at which it is shown.
	virtual TimePoint sourceFrameToAnimationTime(int frame) const override;

	/// \brief Returns the human-readable labels associated with the animation frames (e.g. the simulation timestep numbers).
	virtual QMap<int, QString> animationFrameLabels() const override;

	/// Returns the title of this object.
	virtual QString objectTitle() const override;

	/// Returns the list of data objects that are managed by this data source.
	/// The returned data objects will be displayed as sub-objects of the data source in the pipeline editor.
	virtual const DataCollection* getSourceDataCollection() const override { return dataCollection(); }

	/// \brief Scans the external data file(s) to find all contained frames.
	/// This method is an implementation detail. Please use the high-level method updateListOfFrames() instead.
	SharedFuture<QVector<FileSourceImporter::Frame>> requestFrameList(bool forceRescan);

Q_SIGNALS:

	/// This signal is emitted by the FileSource whenever its list of trajectory frames changes.
	void framesListChanged();

protected:

	/// Asks the object for the results of the data pipeline.
	virtual Future<PipelineFlowState> evaluateInternal(const PipelineEvaluationRequest& request) override;

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// Is called when a RefTarget referenced by this object has generated an event.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) const override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// Sets the source frame number that is currently used as a sub-object data collection.
	void setDataCollectionFrame(int frame) { _dataCollectionFrame = frame; }

private:

	/// Requests a source frame from the input sequence.
	Future<PipelineFlowState> requestFrameInternal(int frame);

	/// Updates the internal list of input frames.
	/// Invalidates cached frames in case they did change.
	void setListOfFrames(QVector<FileSourceImporter::Frame> frames);

	/// Computes the time interval covered on the time line by the given source source.
	TimeInterval frameTimeInterval(int frame) const;

	/// If the file source currently uses a wildcard search pattern, replaces it 
	/// with a single concrete filename. 
	void removeWildcardFilePattern();

	/// Generates a wildcard file seach pattern unless the file source already uses a pattern. 
	void generateWildcardFilePattern();

private:

	/// The associated importer object that is responsible for parsing the input file.
	DECLARE_REFERENCE_FIELD_FLAGS(OORef<FileSourceImporter>, importer, PROPERTY_FIELD_ALWAYS_DEEP_COPY | PROPERTY_FIELD_NO_UNDO);

	/// The list of source files (may include wild-card patterns).
	DECLARE_PROPERTY_FIELD_FLAGS(std::vector<QUrl>, sourceUrls, PROPERTY_FIELD_NO_UNDO);

	/// Controls the mapping of input file frames to animation frames (i.e. the numerator of the playback rate for the file sequence).
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, playbackSpeedNumerator, setPlaybackSpeedNumerator);

	/// Controls the mapping of input file frames to animation frames (i.e. the denominator of the playback rate for the file sequence).
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, playbackSpeedDenominator, setPlaybackSpeedDenominator);

	/// Specifies the starting animation frame to which the first frame of the file sequence is mapped.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, playbackStartTime, setPlaybackStartTime);

	/// Stores the master copy of the loaded data collection.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(DataOORef<const DataCollection>, dataCollection, setDataCollection, PROPERTY_FIELD_DONT_SAVE_RECOMPUTABLE_DATA | PROPERTY_FIELD_DONT_PROPAGATE_MESSAGES | PROPERTY_FIELD_NO_CHANGE_MESSAGE);

	/// Controls the automatic generation of a file name pattern in the GUI.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, autoGenerateFilePattern, setAutoGenerateFilePattern, PROPERTY_FIELD_MEMORIZE);

	/// Restricts the timeline to a single static frame of the loaded trajectory. 
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, restrictToFrame, setRestrictToFrame);

	/// The list of frames of the data source.
	QVector<FileSourceImporter::Frame> _frames;

	/// The human-readable labels associated with animation frames (e.g. the simulation timestep numbers).
	mutable QMap<int, QString> _frameLabels;

	/// The number of different source files from which the trajectory frames get loaded. 
	int _numberOfFiles = 0;

	/// The active future if loading the list of frames is in progress.
	SharedFuture<QVector<FileSourceImporter::Frame>> _framesListFuture;

	/// The source frame that is currently stored in the internal data collection.
	int _dataCollectionFrame = -1;

	/// The file that was originally selected by the user for import.
	/// The animation time slider will automatically be positioned to show the frame corresponding to this file.
	QString _originallySelectedFilename;

	/// Flag indicating that a call to DataObject::updateEditableProxies() is currently in progress
	/// and that change signals received from the data collection should be ignored.
	bool _updatingEditableProxies = false;
};

}	// End of namespace
