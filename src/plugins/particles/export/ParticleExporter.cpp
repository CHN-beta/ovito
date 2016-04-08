///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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

#include <plugins/particles/Particles.h>
#include <core/scene/ObjectNode.h>
#include <core/scene/SceneRoot.h>
#include <core/scene/objects/DataObject.h>
#include <core/animation/AnimationSettings.h>
#include <core/app/Application.h>
#include <gui/mainwin/MainWindow.h>
#include <gui/utilities/concurrent/ProgressDialogAdapter.h>

#include <plugins/particles/objects/ParticlePropertyObject.h>
#include "ParticleExporter.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Export)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Particles, ParticleExporter, FileExporter);
DEFINE_PROPERTY_FIELD(ParticleExporter, _outputFilename, "OutputFile");
DEFINE_PROPERTY_FIELD(ParticleExporter, _exportAnimation, "ExportAnimation");
DEFINE_PROPERTY_FIELD(ParticleExporter, _useWildcardFilename, "UseWildcardFilename");
DEFINE_PROPERTY_FIELD(ParticleExporter, _wildcardFilename, "WildcardFilename");
DEFINE_PROPERTY_FIELD(ParticleExporter, _startFrame, "StartFrame");
DEFINE_PROPERTY_FIELD(ParticleExporter, _endFrame, "EndFrame");
DEFINE_PROPERTY_FIELD(ParticleExporter, _everyNthFrame, "EveryNthFrame");
SET_PROPERTY_FIELD_LABEL(ParticleExporter, _outputFilename, "Output filename");
SET_PROPERTY_FIELD_LABEL(ParticleExporter, _exportAnimation, "Export animation");
SET_PROPERTY_FIELD_LABEL(ParticleExporter, _useWildcardFilename, "Use wildcard filename");
SET_PROPERTY_FIELD_LABEL(ParticleExporter, _wildcardFilename, "Wildcard filename");
SET_PROPERTY_FIELD_LABEL(ParticleExporter, _startFrame, "Start frame");
SET_PROPERTY_FIELD_LABEL(ParticleExporter, _endFrame, "End frame");
SET_PROPERTY_FIELD_LABEL(ParticleExporter, _everyNthFrame, "Every Nth frame");

/******************************************************************************
* Constructs a new instance of the class.
******************************************************************************/
ParticleExporter::ParticleExporter(DataSet* dataset) : FileExporter(dataset),
	_exportAnimation(false),
	_useWildcardFilename(false), _startFrame(0), _endFrame(-1),
	_everyNthFrame(1)
{
	INIT_PROPERTY_FIELD(ParticleExporter::_outputFilename);
	INIT_PROPERTY_FIELD(ParticleExporter::_exportAnimation);
	INIT_PROPERTY_FIELD(ParticleExporter::_useWildcardFilename);
	INIT_PROPERTY_FIELD(ParticleExporter::_wildcardFilename);
	INIT_PROPERTY_FIELD(ParticleExporter::_startFrame);
	INIT_PROPERTY_FIELD(ParticleExporter::_endFrame);
	INIT_PROPERTY_FIELD(ParticleExporter::_everyNthFrame);
}

/******************************************************************************
* Sets the name of the output file that should be written by this exporter.
******************************************************************************/
void ParticleExporter::setOutputFilename(const QString& filename)
{
	_outputFilename = filename;

	// Generate a default wildcard pattern from the filename.
	if(wildcardFilename().isEmpty()) {
		QString fn = QFileInfo(filename).fileName();
		if(!fn.contains('*')) {
			int dotIndex = fn.lastIndexOf('.');
			if(dotIndex > 0)
				setWildcardFilename(fn.left(dotIndex) + QStringLiteral(".*") + fn.mid(dotIndex));
			else
				setWildcardFilename(fn + QStringLiteral(".*"));
		}
		else
			setWildcardFilename(fn);
	}
}

/******************************************************************************
* Exports the scene to the given file.
******************************************************************************/
bool ParticleExporter::exportToFile(const QVector<SceneNode*>& nodes, const QString& filePath, bool noninteractive)
{
	// Save the output path.
	setOutputFilename(filePath);

	// Use the entire animation as default export interval if no interval has been set before.
	if(startFrame() > endFrame()) {
		setStartFrame(0);
		int lastFrame = dataset()->animationSettings()->timeToFrame(dataset()->animationSettings()->animationInterval().end());
		setEndFrame(lastFrame);
	}

	if(Application::instance().guiMode() && !noninteractive) {

		// Get the data to be exported.
		PipelineFlowState flowState = getParticles(nodes, dataset()->animationSettings()->time());
		if(flowState.isEmpty())
			throwException(tr("The selected object does not contain any particles that could be exported."));

		// Show optional export settings dialog.
		if(!showSettingsDialog(flowState, MainWindow::fromDataset(dataset())))
			return false;
	}

	// Perform the actual export operation.
	return writeOutputFiles(nodes);
}

/******************************************************************************
* Retrieves the particles to be exported by evaluating the modification pipeline.
******************************************************************************/
PipelineFlowState ParticleExporter::getParticles(const QVector<SceneNode*>& nodes, TimePoint time)
{
	// Iterate over all scene nodes.
	for(SceneNode* sceneNode : nodes) {

		ObjectNode* node = dynamic_object_cast<ObjectNode>(sceneNode);
		if(!node) continue;

		// Check if the node's pipeline evaluates to something that contains particles.
		const PipelineFlowState& state = node->evalPipeline(time);
		if(ParticlePropertyObject* posProperty = ParticlePropertyObject::findInState(state, ParticleProperty::PositionProperty)) {

			// Verify data, make sure array length is consistent for all particle properties.
			for(DataObject* obj : state.objects()) {
				if(ParticlePropertyObject* p = dynamic_object_cast<ParticlePropertyObject>(obj)) {
					if(p->size() != posProperty->size())
						throwException(tr("Data produced by modification pipeline is invalid. Array size is not the same for all particle properties."));
				}
			}

			return state;
		}
	}

	// Nothing to export.
	return PipelineFlowState();
}

/******************************************************************************
 * Exports the particles contained in the given scene to the output file(s).
 *****************************************************************************/
bool ParticleExporter::writeOutputFiles(const QVector<SceneNode*>& nodes)
{
	OVITO_ASSERT_MSG(!outputFilename().isEmpty(), "ParticleExporter::writeOutputFiles()", "Output filename has not been set. ParticleExporter::setOutputFilename() must be called first.");
	OVITO_ASSERT_MSG(startFrame() <= endFrame(), "ParticleExporter::writeOutputFiles()", "Export interval has not been set. ParticleExporter::setStartFrame() and ParticleExporter::setEndFrame() must be called first.");

	if(startFrame() > endFrame())
		throwException(tr("The animation interval to be exported is empty or has not been set."));

	// Show progress dialog.
	std::unique_ptr<QProgressDialog> progressDialog;
	std::unique_ptr<ProgressDialogAdapter> progressDisplay;
	if(Application::instance().guiMode()) {
		progressDialog.reset(new QProgressDialog(MainWindow::fromDataset(dataset())));
		progressDialog->setWindowModality(Qt::WindowModal);
		progressDialog->setAutoClose(false);
		progressDialog->setAutoReset(false);
		progressDialog->setMinimumDuration(0);
		progressDisplay.reset(new ProgressDialogAdapter(progressDialog.get()));
	}

	// Compute the number of frames that need to be exported.
	TimePoint exportTime;
	int firstFrameNumber, numberOfFrames;
	if(_exportAnimation) {
		firstFrameNumber = startFrame();
		exportTime = dataset()->animationSettings()->frameToTime(firstFrameNumber);
		numberOfFrames = (endFrame() - startFrame() + everyNthFrame()) / everyNthFrame();
		if(numberOfFrames < 1 || everyNthFrame() < 1)
			throwException(tr("Invalid export animation range: Frame %1 to %2").arg(startFrame()).arg(endFrame()));
	}
	else {
		exportTime = dataset()->animationSettings()->time();
		firstFrameNumber = dataset()->animationSettings()->timeToFrame(exportTime);
		numberOfFrames = 1;
	}

	// Validate export settings.
	if(_exportAnimation && useWildcardFilename()) {
		if(wildcardFilename().isEmpty())
			throwException(tr("Cannot write animation frame to separate files. Wildcard pattern has not been specified."));
		if(wildcardFilename().contains(QChar('*')) == false)
			throwException(tr("Cannot write animation frames to separate files. The filename must contain the '*' wildcard character, which gets replaced by the frame number."));
	}

	if(progressDisplay) progressDisplay->setMaximum(numberOfFrames * 100);
	QDir dir = QFileInfo(outputFilename()).dir();
	QString filename = outputFilename();

	// Open output file for writing.
	if(!_exportAnimation || !useWildcardFilename()) {
		if(!openOutputFile(filename, numberOfFrames))
			return false;
	}

	try {

		// Export animation frames.
		for(int frameIndex = 0; frameIndex < numberOfFrames; frameIndex++) {
			if(progressDisplay)
				progressDisplay->setValue(frameIndex * 100);

			int frameNumber = firstFrameNumber + frameIndex * everyNthFrame();

			if(_exportAnimation && useWildcardFilename()) {
				// Generate an output filename based on the wildcard pattern.
				filename = dir.absoluteFilePath(wildcardFilename());
				filename.replace(QChar('*'), QString::number(frameNumber));

				if(!openOutputFile(filename, 1))
					return false;
			}

			if(!exportFrame(nodes, frameNumber, exportTime, filename, progressDisplay.get()) && progressDisplay)
				progressDisplay->cancel();

			if(_exportAnimation && useWildcardFilename())
				closeOutputFile(!progressDisplay || !progressDisplay->wasCanceled());

			if(progressDisplay && progressDisplay->wasCanceled())
				break;

			// Go to next animation frame.
			exportTime += dataset()->animationSettings()->ticksPerFrame() * everyNthFrame();
		}
	}
	catch(...) {
		closeOutputFile(false);
		throw;
	}

	// Close output file.
	if(!_exportAnimation || !useWildcardFilename()) {
		closeOutputFile(!progressDisplay || !progressDisplay->wasCanceled());
	}

	return !progressDisplay || !progressDisplay->wasCanceled();
}

/******************************************************************************
 * This is called once for every output file to be written and before exportParticles() is called.
 *****************************************************************************/
bool ParticleExporter::openOutputFile(const QString& filePath, int numberOfFrames)
{
	OVITO_ASSERT(!_outputFile.isOpen());
	OVITO_ASSERT(!_outputStream);

	_outputFile.setFileName(filePath);
	_outputStream.reset(new CompressedTextWriter(_outputFile));

	return true;
}

/******************************************************************************
 * This is called once for every output file written after exportParticles() has been called.
 *****************************************************************************/
void ParticleExporter::closeOutputFile(bool exportCompleted)
{
	_outputStream.reset();
	if(_outputFile.isOpen())
		_outputFile.close();

	if(!exportCompleted)
		_outputFile.remove();
}

/******************************************************************************
 * Exports a single animation frame to the current output file.
 *****************************************************************************/
bool ParticleExporter::exportFrame(const QVector<SceneNode*>& nodes, int frameNumber, TimePoint time, const QString& filePath, AbstractProgressDisplay* progressDisplay)
{
	// Jump to animation time.
	dataset()->animationSettings()->setTime(time);

	// Wait until the scene is ready.
	if(!dataset()->waitUntilSceneIsReady(tr("Preparing frame %1 for export...").arg(frameNumber), progressDisplay))
		return false;

	if(progressDisplay)
		progressDisplay->setStatusText(tr("Exporting frame %1 to file '%2'.").arg(frameNumber).arg(filePath));

	// Evaluate modification pipeline to get the particles to be exported.
	PipelineFlowState state = getParticles(nodes, time);
	if(state.isEmpty())
		throwException(tr("The object to be exported does not contain any particles."));

	try {
		return exportParticles(state, frameNumber, time, filePath, progressDisplay);
	}
	catch(Exception& ex) {
		// Provide a local context for errors that occurred during export.
		if(ex.context() == nullptr) ex.setContext(dataset());
		throw;
	}
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
