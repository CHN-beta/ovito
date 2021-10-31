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
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/objects/ParticleType.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/mesh/tri/TriMeshObject.h>
#include <ovito/mesh/surface/SurfaceMesh.h>
#include <ovito/mesh/surface/SurfaceMeshAccess.h>
#include <ovito/mesh/io/ParaViewVTPMeshImporter.h>
#include <ovito/core/dataset/data/DataObjectAccess.h>
#include <ovito/core/dataset/io/FileSource.h>
#include "ParaViewVTPParticleImporter.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(ParaViewVTPParticleImporter);
IMPLEMENT_OVITO_CLASS(ParticlesParaViewVTMFileFilter);

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool ParaViewVTPParticleImporter::OOMetaClass::checkFileFormat(const FileHandle& file) const
{
	// Initialize XML reader and open input file.
	std::unique_ptr<QIODevice> device = file.createIODevice();
	if(!device->open(QIODevice::ReadOnly | QIODevice::Text))
		return false;
	QXmlStreamReader xml(device.get());

	// Parse XML. First element must be <VTKFile type="PolyData">.
	if(xml.readNext() != QXmlStreamReader::StartDocument)
		return false;
	if(xml.readNext() != QXmlStreamReader::StartElement)
		return false;
	if(xml.name().compare(QLatin1String("VTKFile")) != 0)
		return false;
	if(xml.attributes().value("type").compare(QLatin1String("PolyData")) != 0)
		return false;

	// Continue until we reach the <Piece> element. 
	while(xml.readNextStartElement()) {
		if(xml.name().compare(QLatin1String("Piece")) == 0) {
			// Number of lines, triangle strips, and polygons must be zero.
			if(xml.attributes().value("NumberOfLines").toULongLong() == 0 && xml.attributes().value("NumberOfStrips").toULongLong() == 0 && xml.attributes().value("NumberOfPolys").toULongLong() == 0) {
				// Number of vertices must match number of points.
				if(xml.attributes().value("NumberOfPoints") == xml.attributes().value("NumberOfVerts")) {
					return !xml.hasError();
				}
			}
			break;
		}
	}

	return false;
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
void ParaViewVTPParticleImporter::FrameLoader::loadFile()
{
	setProgressText(tr("Reading ParaView VTP particles file %1").arg(fileHandle().toString()));

	// Initialize XML reader and open input file.
	std::unique_ptr<QIODevice> device = fileHandle().createIODevice();
	if(!device->open(QIODevice::ReadOnly | QIODevice::Text))
		throw Exception(tr("Failed to open VTP file: %1").arg(device->errorString()));
	QXmlStreamReader xml(device.get());

	// Append particles to existing particles object when requested by the caller.
	// This may be the case when loading a multi-block dataset specified in a VTM file.
	size_t baseParticleIndex = 0;
	bool preserveExistingData = false;
	if(loadRequest().appendData) {
		baseParticleIndex = particles()->elementCount();
		preserveExistingData = (baseParticleIndex != 0);
	}

	// Parse the elements of the XML file.
	while(xml.readNextStartElement()) {
		if(isCanceled())
			return;

		if(xml.name().compare(QLatin1String("VTKFile")) == 0) {
			if(xml.attributes().value("type").compare(QLatin1String("PolyData")) != 0)
				xml.raiseError(tr("VTK file is not of type PolyData."));
			else if(xml.attributes().value("byte_order").compare(QLatin1String("LittleEndian")) != 0)
				xml.raiseError(tr("Byte order must be 'LittleEndian'. Please contact the OVITO developers to request an extension of the file parser."));
			else if(xml.attributes().value("compressor").compare(QLatin1String("")) != 0)
				xml.raiseError(tr("The parser does not support compressed data arrays. Please contact the OVITO developers to request an extension of the file parser."));
		}
		else if(xml.name().compare(QLatin1String("PolyData")) == 0) {
			// Do nothing. Parse child elements.
		}
		else if(xml.name().compare(QLatin1String("Piece")) == 0) {

			// Parse number of lines, triangle strips and polygons.
			if(xml.attributes().value("NumberOfLines").toULongLong() != 0
					|| xml.attributes().value("NumberOfStrips").toULongLong() != 0
					|| xml.attributes().value("NumberOfPolys").toULongLong() != 0) {
				xml.raiseError(tr("Number of lines, strips and polys are nonzero. This particle file parser can only read PolyData datasets containing vertices only."));
				break;
			}

			// Parse number of points.
			size_t numParticles = xml.attributes().value("NumberOfPoints").toULongLong();
			// Parse number of vertices.
			size_t numVertices = xml.attributes().value("NumberOfVerts").toULongLong();
			if(numVertices != numParticles) {
				xml.raiseError(tr("Number of vertices does not match number of points. This file parser can only read datasets consisting of vertices only."));
				break;
			}
			setParticleCount(baseParticleIndex + numParticles);
		}
		else if(xml.name().compare(QLatin1String("PointData")) == 0 || xml.name().compare(QLatin1String("Points")) == 0 || xml.name().compare(QLatin1String("Verts")) == 0) {
			// Parse child elements.
			while(xml.readNextStartElement() && !isCanceled()) {
				if(xml.name().compare(QLatin1String("DataArray")) == 0) {
					int vectorComponent = -1;
					if(PropertyObject* property = createParticlePropertyForDataArray(xml, vectorComponent, preserveExistingData)) {
						ParaViewVTPMeshImporter::parseVTKDataArray(property, baseParticleIndex, property->size(), vectorComponent, xml);
						if(xml.hasError() || isCanceled())
							break;

						// Create particle types if this is a typed property.
						OvitoClassPtr elementTypeClass = ParticlesObject::OOClass().typedPropertyElementClass(property->type());
						if(!elementTypeClass && property->name() == QStringLiteral("Material Type")) elementTypeClass = &ElementType::OOClass();
						if(elementTypeClass) {
							for(int t : ConstPropertyAccess<int>(property).csubrange(baseParticleIndex, property->size())) {
								if(!property->elementType(t)) {
									DataOORef<ElementType> elementType = static_object_cast<ElementType>(elementTypeClass->createInstance(dataset(), executionContext()));
									elementType->setNumericId(t);
									elementType->initializeType(PropertyReference(&ParticlesObject::OOClass(), property), executionContext());
									if(elementTypeClass == &ParticleType::OOClass()) {
										// Load mesh-based shape of the particle type as specified in the VTM container file.
										loadParticleShape(static_object_cast<ParticleType>(elementType.get()));
									}
									property->addElementType(std::move(elementType));
								}
							}
						}
					}
					if(xml.tokenType() != QXmlStreamReader::EndElement)
						xml.skipCurrentElement();
				}
				else {
					xml.raiseError(tr("Unexpected XML element <%1>.").arg(xml.name().toString()));
				}
			}
		}
		else if(xml.name().compare(QLatin1String("CellData")) == 0 || xml.name().compare(QLatin1String("Lines")) == 0 || xml.name().compare(QLatin1String("Strips")) == 0 || xml.name().compare(QLatin1String("Polys")) == 0) {
			// Do nothing. Ignore element contents.
			xml.skipCurrentElement();
		}
		else {
			xml.raiseError(tr("Unexpected XML element <%1>.").arg(xml.name().toString()));
		}
	}

	// Handle XML parsing errors.
	if(xml.hasError()) {
		throw Exception(tr("VTP file parsing error on line %1, column %2: %3")
			.arg(xml.lineNumber()).arg(xml.columnNumber()).arg(xml.errorString()));
	}
	if(isCanceled())
		return;

	// Convert superquadric 'Blockiness' values from the Aspherix simulation to 'Roundness' values used by OVITO particle visualization.
	bool transposeOrientations = false;
	if(PropertyObject* roundnessProperty = particles()->getMutableProperty(ParticlesObject::SuperquadricRoundnessProperty)) {
		for(Vector2& v : PropertyAccess<Vector2>(roundnessProperty).subrange(baseParticleIndex, roundnessProperty->size())) {
			// Blockiness1: "north-south" blockiness
			// Blockiness2: "east-west" blockiness
			// Roundness.x: "east-west" roundness
			// Roundness.y: "north-south" roundness
			std::swap(v.x(), v.y());
    		// Roundness = 2.0 / Blockiness:
			if(v.x() != 0) v.x() = FloatType(2) / v.x();
			if(v.y() != 0) v.y() = FloatType(2) / v.y();
		}
		transposeOrientations = true;
		if(isCanceled())
			return;
	}

	// Convert 3x3 'Tensor' property into particle orientation.
	if(const PropertyObject* tensorProperty = particles()->getProperty(QStringLiteral("Tensor"))) {
		if(tensorProperty->dataType() == PropertyObject::Float && tensorProperty->componentCount() == 9) {
			PropertyAccess<Quaternion> orientations = particles()->createProperty(ParticlesObject::OrientationProperty, preserveExistingData, executionContext());
			Quaternion* q = orientations.begin() + baseParticleIndex;
			for(const Matrix3& tensor : ConstPropertyAccess<Matrix3>(tensorProperty).csubrange(baseParticleIndex, tensorProperty->size())) {
				*q++ = Quaternion(transposeOrientations ? tensor.transposed() : tensor);
			}
			if(isCanceled())
				return;
		}
	}

	// Reset "Radius" property of particles with a mesh-based shape to zero to get correct scaling. 
	if(const PropertyObject* typeProperty = particles()->getProperty(ParticlesObject::TypeProperty)) {
		std::vector<int> typesWithMeshShape;
		for(const ElementType* type : typeProperty->elementTypes()) {
			if(const ParticleType* particleType = dynamic_object_cast<ParticleType>(type))
				if(particleType->shape() == ParticlesVis::ParticleShape::Mesh)
					typesWithMeshShape.push_back(particleType->numericId());
		}
		if(typesWithMeshShape.size() == typeProperty->elementTypes().size()) {
			// If all particle shapes are mesh-based, simply remove the "Radius" property, which is not used in this case anyway.
			if(const PropertyObject* radiusProperty = particles()->getProperty(ParticlesObject::RadiusProperty))
				particles()->removeProperty(radiusProperty);
		}
		else if(!typesWithMeshShape.empty()) {
			if(PropertyAccess<FloatType> radiusArray = particles()->getMutableProperty(ParticlesObject::RadiusProperty)) {
				FloatType* radius = radiusArray.begin() + baseParticleIndex;
				for(int t : ConstPropertyAccess<int>(typeProperty).csubrange(baseParticleIndex, typeProperty->size())) {
					if(std::find(typesWithMeshShape.cbegin(), typesWithMeshShape.cend(), t) != typesWithMeshShape.cend())
						*radius = 0.0;
					++radius;
				}
			}
		}
	}

	// Report number of particles to the user.
	QString statusString = tr("Number of particles: %1").arg(particles()->elementCount());
	state().setStatus(std::move(statusString));

	// Call base implementation to finalize the loaded particle data.
	ParticleImporter::FrameLoader::loadFile();
}

/******************************************************************************
* Creates the right kind of OVITO property object that will receive the data 
* read from a <DataArray> element.
******************************************************************************/
PropertyObject* ParaViewVTPParticleImporter::FrameLoader::createParticlePropertyForDataArray(QXmlStreamReader& xml, int& vectorComponent, bool preserveExistingData)
{
	int numComponents = std::max(1, xml.attributes().value("NumberOfComponents").toInt());
	auto name = xml.attributes().value("Name");

	if(name.compare(QLatin1String("connectivity"), Qt::CaseInsensitive) == 0 || name.compare(QLatin1String("offsets"), Qt::CaseInsensitive) == 0) {
		return nullptr;
	}
	else if(name.compare(QLatin1String("points"), Qt::CaseInsensitive) == 0 && numComponents == 3) {
		return particles()->createProperty(ParticlesObject::PositionProperty, preserveExistingData, executionContext());
	}
	else if(name.compare(QLatin1String("id"), Qt::CaseInsensitive) == 0 && numComponents == 1) {
		return particles()->createProperty(ParticlesObject::IdentifierProperty, preserveExistingData, executionContext());
	}
	else if(name.compare(QLatin1String("type"), Qt::CaseInsensitive) == 0 && numComponents == 1) {
		PropertyObject* property = particles()->createProperty(QStringLiteral("Material Type"), PropertyObject::Int, 1, 0, preserveExistingData);
		property->setTitle(tr("Material types"));
		return property;
	}
	else if(name.compare(QLatin1String("shapetype"), Qt::CaseInsensitive) == 0 && numComponents == 1) {
		return particles()->createProperty(ParticlesObject::TypeProperty, preserveExistingData, executionContext());
	}
	else if(name.compare(QLatin1String("mass"), Qt::CaseInsensitive) == 0 && numComponents == 1) {
		return particles()->createProperty(ParticlesObject::MassProperty, preserveExistingData, executionContext());
	}
	else if(name.compare(QLatin1String("radius"), Qt::CaseInsensitive) == 0 && numComponents == 1) {
		return particles()->createProperty(ParticlesObject::RadiusProperty, preserveExistingData, executionContext());
	}
	else if(name.compare(QLatin1String("v"), Qt::CaseInsensitive) == 0 && numComponents == 3) {
		return particles()->createProperty(ParticlesObject::VelocityProperty, preserveExistingData, executionContext());
	}
	else if(name.compare(QLatin1String("omega"), Qt::CaseInsensitive) == 0 && numComponents == 3) {
		return particles()->createProperty(ParticlesObject::AngularVelocityProperty, preserveExistingData, executionContext());
	}
	else if(name.compare(QLatin1String("tq"), Qt::CaseInsensitive) == 0 && numComponents == 3) {
		return particles()->createProperty(ParticlesObject::TorqueProperty, preserveExistingData, executionContext());
	}
	else if(name.compare(QLatin1String("f"), Qt::CaseInsensitive) == 0 && numComponents == 3) {
		return particles()->createProperty(ParticlesObject::ForceProperty, preserveExistingData, executionContext());
	}
	else if(name.compare(QLatin1String("density"), Qt::CaseInsensitive) == 0 && numComponents == 1) {
		return particles()->createProperty(QStringLiteral("Density"), PropertyObject::Float, 1, 0, preserveExistingData);
	}
	else if(name.compare(QLatin1String("tensor"), Qt::CaseInsensitive) == 0 && numComponents == 9) {
		return particles()->createProperty(QStringLiteral("Tensor"), PropertyObject::Float, 9, 0, preserveExistingData);
	}
	else if(name.compare(QLatin1String("shapex"), Qt::CaseInsensitive) == 0 && numComponents == 1) {
		vectorComponent = 0;
		return particles()->createProperty(ParticlesObject::AsphericalShapeProperty, true, executionContext());
	}
	else if(name.compare(QLatin1String("shapey"), Qt::CaseInsensitive) == 0 && numComponents == 1) {
		vectorComponent = 1;
		return particles()->createProperty(ParticlesObject::AsphericalShapeProperty, true, executionContext());
	}
	else if(name.compare(QLatin1String("shapez"), Qt::CaseInsensitive) == 0 && numComponents == 1) {
		vectorComponent = 2;
		return particles()->createProperty(ParticlesObject::AsphericalShapeProperty, true, executionContext());
	}
	else if(name.compare(QLatin1String("blockiness1"), Qt::CaseInsensitive) == 0 && numComponents == 1) {
		vectorComponent = 0;
		return particles()->createProperty(ParticlesObject::SuperquadricRoundnessProperty, true, executionContext());
	}
	else if(name.compare(QLatin1String("blockiness2"), Qt::CaseInsensitive) == 0 && numComponents == 1) {
		vectorComponent = 1;
		return particles()->createProperty(ParticlesObject::SuperquadricRoundnessProperty, true, executionContext());
	}
	else {
		return particles()->createProperty(name.toString(), PropertyObject::Float, numComponents, 0, preserveExistingData);
	}
	return nullptr;
}

/******************************************************************************
* Helper method that loads the shape of a particle type from an external geometry file.
******************************************************************************/
void ParaViewVTPParticleImporter::FrameLoader::loadParticleShape(ParticleType* particleType)
{
	OVITO_ASSERT(dataset()->undoStack().isRecordingThread() == false);

	// According to Aspherix convention, particle type -1 has no shape.
	if(particleType->numericId() < 0 || particleType->numericId() >= _particleShapeFiles.size())
		return;

	// Adopt the particle type name from the VTM file.
	particleType->setName(_particleShapeFiles[particleType->numericId()].first);

	// Set radius of particle type to 1.0 to always get correct scaling of shape geometry.
	particleType->setRadius(1.0);

	// Fetch the shape geometry file, then continue in main thread.
	// Note: Invoking a file importer is currently only allowed from the main thread. This may change in the future.
	const QUrl& geometryFileUrl = _particleShapeFiles[particleType->numericId()].second;
	Future<PipelineFlowState> stateFuture = Application::instance()->fileManager()->fetchUrl(*taskManager(), geometryFileUrl).then(particleType->executor(executionContext()), [particleType,dataSource=dataSource()](const FileHandle& fileHandle) {

		// Detect geometry file format and create an importer for it.
		// Note: For loading particle shape geometries we only accept FileSourceImporters.
		ExecutionContext executionContext = Application::instance()->executionContext();
		OORef<FileSourceImporter> importer = dynamic_object_cast<FileSourceImporter>(FileImporter::autodetectFileFormat(particleType->dataset(), executionContext, fileHandle));
		if(!importer)
			return Future<PipelineFlowState>::createImmediateEmpty();

		// Set up a file load request to be passed to the importer.
		LoadOperationRequest request;
		request.dataset = particleType->dataset();
		request.dataSource = dataSource;
		request.fileHandle = fileHandle;
		request.frame = Frame(fileHandle);
		request.state = PipelineFlowState(DataOORef<const DataCollection>::create(particleType->dataset(), executionContext), PipelineStatus::Success);

		// Let the importer parse the geometry file.
		return importer->loadFrame(request);
	});
	if(!waitForFuture(stateFuture))
		return;

	// Check if the importer has loaded any data.
	PipelineFlowState state = stateFuture.result();
	if(!state || state.status().type() == PipelineStatus::Error)
		return;

	// Look for a triangle mesh or a surface mesh.
	DataObjectAccess<DataOORef, TriMeshObject> meshObj = state.getObject<TriMeshObject>();
	if(!meshObj || !meshObj->mesh()) {
		if(const SurfaceMesh* surfaceMesh = state.getObject<SurfaceMesh>()) {
			// Convert surface mesh to triangle mesh.
			std::shared_ptr<TriMesh> triMesh = std::make_shared<TriMesh>();
			SurfaceMeshAccess(surfaceMesh).convertToTriMesh(*triMesh, false);
			meshObj.reset(DataOORef<TriMeshObject>::create(surfaceMesh->dataset(), ExecutionContext::Scripting));
			meshObj.makeMutable()->setMesh(std::move(triMesh));
			meshObj.makeMutable()->setVisElement(nullptr);
		}
		else return;
	}
	state.reset();

	// Show sharp edges of the mesh.
	meshObj.makeMutable()->modifiableMesh()->determineEdgeVisibility();

	particleType->setShapeMesh(meshObj.take());
	particleType->setShape(ParticlesVis::Mesh);

	// Aspherix particle geometries seem no to have a consistent face winding order. 
	// Need to turn edge highlighting and backface culling off by default.
	particleType->setShapeBackfaceCullingEnabled(false);
	particleType->setHighlightShapeEdges(false);
}

/******************************************************************************
* Is called once before the datasets referenced in a multi-block VTM file will be loaded.
******************************************************************************/
void ParticlesParaViewVTMFileFilter::preprocessDatasets(std::vector<ParaViewVTMBlockInfo>& blockDatasets, FileSourceImporter::LoadOperationRequest& request, const ParaViewVTMImporter& vtmImporter)
{
	// Resize particles object to zero elements in the existing pipeline state.
	// This is mainly done to remove the existing particles in those animation frames in which the VTM file has empty data blocks.
	for(const DataObject* obj : request.state.data()->objects()) {
		if(const ParticlesObject* particles = dynamic_object_cast<ParticlesObject>(obj)) {
			ParticlesObject* mutableParticles = request.state.mutableData()->makeMutable(particles);
			mutableParticles->setElementCount(0);
		}
	}

	// Remove those datasets from the multi-block structure that represent particle shapes.
	// Keep a list of these removed datasets for later to load them together with the particles dataset.
	blockDatasets.erase(std::remove_if(blockDatasets.begin(), blockDatasets.end(), [this](const auto& blockInfo) {
		if(blockInfo.blockPath.size() == 2 && blockInfo.blockPath[0] == QStringLiteral("Convex shapes")) {
			// Store the particle type name and the URL of the type's shape file in the internal list.
			_particleShapeFiles.emplace_back(blockInfo.blockPath[1], std::move(blockInfo.location));
			return true;
		}
		return false;
	}), blockDatasets.end());
}

/******************************************************************************
* Is called before parsing of a dataset reference in a multi-block VTM file begins.
******************************************************************************/
void ParticlesParaViewVTMFileFilter::configureImporter(const ParaViewVTMBlockInfo& blockInfo, FileSourceImporter::LoadOperationRequest& loadRequest, FileSourceImporter* importer)
{
	// Pass the list of particle shape files to be loaded to the VTP particle importer, which will take care
	// of loading the files.
	if(ParaViewVTPParticleImporter* particleImporter = dynamic_object_cast<ParaViewVTPParticleImporter>(importer)) {
		particleImporter->setParticleShapeFileList(std::move(_particleShapeFiles));
	}
}

}	// End of namespace
}	// End of namespace
