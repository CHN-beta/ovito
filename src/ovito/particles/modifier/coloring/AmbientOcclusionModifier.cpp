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

#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/ParticlesVis.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/app/PluginManager.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include <ovito/core/rendering/FrameBuffer.h>
#include <ovito/core/rendering/SceneRenderer.h>
#include "AmbientOcclusionModifier.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(AmbientOcclusionModifier);
DEFINE_PROPERTY_FIELD(AmbientOcclusionModifier, intensity);
DEFINE_PROPERTY_FIELD(AmbientOcclusionModifier, samplingCount);
DEFINE_PROPERTY_FIELD(AmbientOcclusionModifier, bufferResolution);
SET_PROPERTY_FIELD_LABEL(AmbientOcclusionModifier, intensity, "Shading intensity");
SET_PROPERTY_FIELD_LABEL(AmbientOcclusionModifier, samplingCount, "Number of exposure samples");
SET_PROPERTY_FIELD_LABEL(AmbientOcclusionModifier, bufferResolution, "Render buffer resolution");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(AmbientOcclusionModifier, intensity, PercentParameterUnit, 0, 1);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(AmbientOcclusionModifier, samplingCount, IntegerParameterUnit, 3, 2000);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(AmbientOcclusionModifier, bufferResolution, IntegerParameterUnit, 1, AmbientOcclusionModifier::MAX_AO_RENDER_BUFFER_RESOLUTION);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
AmbientOcclusionModifier::AmbientOcclusionModifier(DataSet* dataset) : AsynchronousModifier(dataset),
	_intensity(0.7),
	_samplingCount(40),
	_bufferResolution(3)
{
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool AmbientOcclusionModifier::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	return input.containsObject<ParticlesObject>();
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the
* modifier's results.
******************************************************************************/
Future<AsynchronousModifier::EnginePtr> AmbientOcclusionModifier::createEngine(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input, ExecutionContext executionContext)
{
	if(Application::instance()->headlessMode())
		throwException(tr("The ambient occlusion modifier requires OpenGL support and cannot be used when program is running in headless mode. "
						  "Please run program on a machine where access to graphics hardware is available."));

	// Get modifier input.
	const ParticlesObject* particles = input.expectObject<ParticlesObject>();
	particles->verifyIntegrity();
	const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);
	const PropertyObject* typeProperty = particles->getProperty(ParticlesObject::TypeProperty);
	const PropertyObject* radiusProperty = particles->getProperty(ParticlesObject::RadiusProperty);
	const PropertyObject* shapeProperty = particles->getProperty(ParticlesObject::AsphericalShapeProperty);

	// Compute bounding box of input particles.
	Box3 boundingBox;
	if(ParticlesVis* particleVis = particles->visElement<ParticlesVis>()) {
		boundingBox.addBox(particleVis->particleBoundingBox(posProperty, typeProperty, radiusProperty, shapeProperty, true));
	}

	// The render buffer resolution.
	int res = qBound(0, bufferResolution(), (int)MAX_AO_RENDER_BUFFER_RESOLUTION);
	int resolution = (128 << res);

	TimeInterval validityInterval = input.stateValidity();
	ConstPropertyPtr radii = particles->inputParticleRadii();

	// Create the offscreen renderer implementation.
	OvitoClassPtr rendererClass = PluginManager::instance().findClass("OpenGLRenderer", "OffscreenOpenGLSceneRenderer");
	if(!rendererClass)
		throwException(tr("The OffscreenOpenGLSceneRenderer class is not available. Please make sure the OpenGLRenderer plugin is installed correctly."));
	OORef<SceneRenderer> renderer = static_object_cast<SceneRenderer>(rendererClass->createInstance(dataset(), ExecutionContext::Scripting));

	// Activate picking mode, because we want to render particles using false colors.
	renderer->setPicking(true);

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<AmbientOcclusionEngine>(modApp, executionContext, dataset(), validityInterval, particles, resolution, samplingCount(), posProperty, std::move(radii), boundingBox, std::move(renderer));
}

/******************************************************************************
* Compute engine constructor.
******************************************************************************/
AmbientOcclusionModifier::AmbientOcclusionEngine::AmbientOcclusionEngine(const PipelineObject* dataSource, ExecutionContext executionContext, DataSet* dataset, const TimeInterval& validityInterval, ParticleOrderingFingerprint fingerprint, int resolution, int samplingCount, ConstPropertyPtr positions,
		ConstPropertyPtr particleRadii, const Box3& boundingBox, OORef<SceneRenderer> renderer) :
	Engine(dataSource, executionContext, validityInterval),
	_resolution(resolution),
	_samplingCount(std::max(1,samplingCount)),
	_positions(std::move(positions)),
	_particleRadii(std::move(particleRadii)),
	_boundingBox(boundingBox),
	_renderer(std::move(renderer)),
	_brightness(ParticlesObject::OOClass().createUserProperty(dataset, fingerprint.particleCount(), PropertyObject::Float, 1, 0, QStringLiteral("Brightness"), true)),
	_inputFingerprint(std::move(fingerprint))
{
	OVITO_ASSERT(_particleRadii->size() == _positions->size());
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void AmbientOcclusionModifier::AmbientOcclusionEngine::perform()
{
	if(positions()->size() != 0) {
		if(_boundingBox.isEmpty())
			throw Exception(tr("Modifier input is degenerate or contains no particles."));

		setProgressText(tr("Ambient occlusion"));

		// Create the rendering frame buffer that receives the rendered image of the particles.
		FrameBuffer frameBuffer(_resolution, _resolution);

		// Initialize the renderer.
		_renderer->startRender(nullptr, nullptr, frameBuffer.size());
		try {
			// The buffered particle geometry used for rendering the particles.
			std::shared_ptr<ParticlePrimitive> particleBuffer;

			setProgressMaximum(_samplingCount);
			for(int sample = 0; sample < _samplingCount; sample++) {
				if(!setProgressValue(sample))
					break;

				// Generate lighting direction on unit sphere.
				FloatType y = (FloatType)sample * 2 / _samplingCount - FloatType(1) + FloatType(1) / _samplingCount;
				FloatType phi = (FloatType)sample * FLOATTYPE_PI * (FloatType(3) - sqrt(FloatType(5)));
				Vector3 dir(cos(phi), y, sin(phi));

				// Set up view projection.
				ViewProjectionParameters projParams;
				projParams.viewMatrix = AffineTransformation::lookAlong(_boundingBox.center(), dir, Vector3(0,0,1));

				// Transform bounding box to camera space.
				Box3 bb = _boundingBox.transformed(projParams.viewMatrix).centerScale(FloatType(1.01));

				// Complete projection parameters.
				projParams.aspectRatio = 1;
				projParams.isPerspective = false;
				projParams.inverseViewMatrix = projParams.viewMatrix.inverse();
				projParams.fieldOfView = FloatType(0.5) * _boundingBox.size().length();
				projParams.znear = -bb.maxc.z();
				projParams.zfar  = std::max(-bb.minc.z(), projParams.znear + FloatType(1));
				projParams.projectionMatrix = Matrix4::ortho(-projParams.fieldOfView, projParams.fieldOfView,
									-projParams.fieldOfView, projParams.fieldOfView,
									projParams.znear, projParams.zfar);
				projParams.inverseProjectionMatrix = projParams.projectionMatrix.inverse();
				projParams.validityInterval = TimeInterval::infinite();

				_renderer->beginFrame(0, projParams, nullptr);
				_renderer->setWorldTransform(AffineTransformation::Identity());
				try {
					// Create particle buffer.
					if(!particleBuffer) {
						particleBuffer = _renderer->createParticlePrimitive(ParticlePrimitive::SphericalShape, ParticlePrimitive::FlatShading, ParticlePrimitive::LowQuality);
						particleBuffer->setPositions(positions());
						particleBuffer->setRadii(particleRadii());
					}
					_renderer->renderParticles(particleBuffer);
				}
				catch(...) {
					_renderer->endFrame(false, nullptr);
					throw;
				}
				// Discard the existing image in the frame buffer so that
				// OffscreenOpenGLSceneRenderer::endFrame() can just return the unmodified
				// frame buffer contents.
				frameBuffer.image() = QImage();

				// Retrieve the frame buffer contents.
				_renderer->endFrame(true, &frameBuffer);

				// Extract brightness values from rendered image.
				const QImage& image = frameBuffer.image();
				PropertyAccess<FloatType> brightnessValues(brightness());
				for(int y = 0; y < _resolution; y++) {
					const QRgb* pixel = reinterpret_cast<const QRgb*>(image.scanLine(y));
					for(int x = 0; x < _resolution; x++, ++pixel) {
						quint32 red = qRed(*pixel);
						quint32 green = qGreen(*pixel);
						quint32 blue = qBlue(*pixel);
						quint32 alpha = qAlpha(*pixel);
						quint32 id = red + (green << 8) + (blue << 16) + (alpha << 24);
						if(id == 0)
							continue;
						// Subtracting base 1 from ID, because that's how OpenGLSceneRenderer::registerSubObjectIDs() is implemented.
						quint32 particleIndex = id - 1;
						OVITO_ASSERT(particleIndex < brightnessValues.size());
						brightnessValues[particleIndex] += 1;
					}
				}
			}
		}
		catch(...) {
			_renderer->endRender();
			throw;
		}
		_renderer->endRender();

		if(isCanceled()) return;
		setProgressValue(_samplingCount);

		// Normalize brightness values by particle area.
		ConstPropertyAccess<FloatType> radiusArray(particleRadii());
		PropertyAccess<FloatType> brightnessValues(brightness());
		auto r = radiusArray.cbegin();
		for(FloatType& b : brightnessValues) {
			if(*r != 0)
				b /= (*r) * (*r);
			++r;
		}

		if(isCanceled()) return;

		// Normalize brightness values by global maximum.
		FloatType maxBrightness = *boost::max_element(brightnessValues);
		if(maxBrightness != 0) {
			for(FloatType& b : brightnessValues) {
				b /= maxBrightness;
			}
		}
	}

	// Release data that is no longer needed to reduce memory footprint.
	_positions.reset();
	_particleRadii.reset();
	_renderer.reset();
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
void AmbientOcclusionModifier::AmbientOcclusionEngine::applyResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	AmbientOcclusionModifier* modifier = static_object_cast<AmbientOcclusionModifier>(modApp->modifier());
	OVITO_ASSERT(modifier);

	ParticlesObject* particles = state.expectMutableObject<ParticlesObject>();
	if(_inputFingerprint.hasChanged(particles))
		modApp->throwException(tr("Cached modifier results are obsolete, because the number or the storage order of input particles has changed."));
	OVITO_ASSERT(brightness() && particles->elementCount() == brightness()->size());

	// Get effective intensity.
	FloatType intensity = qBound(FloatType(0), modifier->intensity(), FloatType(1));
	if(intensity == 0 || particles->elementCount() == 0) return;

	// Get output property object.
	ConstPropertyAccess<FloatType> brightnessValues(brightness());
	PropertyAccess<Color> colorProperty = particles->createProperty(ParticlesObject::ColorProperty, true, Application::instance()->executionContext(), {particles});
	const FloatType* b = brightnessValues.cbegin();
	for(Color& c : colorProperty) {
		FloatType factor = FloatType(1) - intensity + (*b);
		if(factor < FloatType(1))
			c = c * factor;
		++b;
	}
}

}	// End of namespace
}	// End of namespace
