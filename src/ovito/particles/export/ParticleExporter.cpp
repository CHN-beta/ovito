////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2022 OVITO GmbH, Germany
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

#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include <ovito/core/dataset/scene/SelectionSet.h>
#include "ParticleExporter.h"

namespace Ovito::Particles {

IMPLEMENT_OVITO_CLASS(ParticleExporter);

/******************************************************************************
* Evaluates the pipeline of an PipelineSceneNode and makes sure that the data to be
* exported contains particles and throws an exception if not.
******************************************************************************/
PipelineFlowState ParticleExporter::getParticleData(TimePoint time, MainThreadOperation& operation) const
{
	PipelineFlowState state = getPipelineDataToBeExported(time, operation);
	if(operation.isCanceled())
		return {};

	const ParticlesObject* particles = state.getObject<ParticlesObject>();
	if(!particles)
		throwException(tr("The selected data collection does not contain any particles that can be exported."));
	if(!particles->getProperty(ParticlesObject::PositionProperty))
		throwException(tr("The particles to be exported do not have any coordinates ('Position' property is missing)."));

	// Verify data, make sure array length is consistent for all particle properties.
	particles->verifyIntegrity();

	// Verify data, make sure array length is consistent for all bond properties.
	if(particles->bonds()) {
		particles->bonds()->verifyIntegrity();
	}

	return state;
}

/******************************************************************************
 * This is called once for every output file to be written and before
 * exportFrame() is called.
 *****************************************************************************/
bool ParticleExporter::openOutputFile(const QString& filePath, int numberOfFrames, MainThreadOperation& operation)
{
	OVITO_ASSERT(!_outputFile.isOpen());
	OVITO_ASSERT(!_outputStream);

	_outputFile.setFileName(filePath);
	_outputStream.reset(new CompressedTextWriter(_outputFile, dataset()));
	_outputStream->setFloatPrecision(floatOutputPrecision());

	return true;
}

/******************************************************************************
 * This is called once for every output file written after exportFrame()
 * has been called.
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
bool ParticleExporter::exportFrame(int frameNumber, TimePoint time, const QString& filePath, MainThreadOperation& operation)
{
	// Retreive the particle data to be exported.
	const PipelineFlowState& state = getParticleData(time, operation);
	if(operation.isCanceled() || !state)
		return false;

	// Set progress display.
	operation.setProgressText(tr("Writing file %1").arg(filePath));

	// Let the subclass do the work.
	return exportData(state, frameNumber, time, filePath, operation);
}

}	// End of namespace
