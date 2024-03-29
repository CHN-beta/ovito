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

#include <ovito/mesh/Mesh.h>
#include <ovito/mesh/util/CapPolygonTessellator.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/rendering/SceneRenderer.h>
#include <ovito/core/rendering/MeshPrimitive.h>
#include <ovito/core/dataset/data/mesh/TriMeshObject.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include <ovito/core/dataset/animation/controller/Controller.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/app/Application.h>
#include "SurfaceMeshVis.h"
#include "SurfaceMesh.h"
#include "RenderableSurfaceMesh.h"

namespace Ovito::Mesh {

IMPLEMENT_OVITO_CLASS(SurfaceMeshVis);
DEFINE_PROPERTY_FIELD(SurfaceMeshVis, surfaceColor);
DEFINE_PROPERTY_FIELD(SurfaceMeshVis, capColor);
DEFINE_PROPERTY_FIELD(SurfaceMeshVis, showCap);
DEFINE_PROPERTY_FIELD(SurfaceMeshVis, smoothShading);
DEFINE_PROPERTY_FIELD(SurfaceMeshVis, reverseOrientation);
DEFINE_PROPERTY_FIELD(SurfaceMeshVis, highlightEdges);
DEFINE_PROPERTY_FIELD(SurfaceMeshVis, surfaceIsClosed);
DEFINE_PROPERTY_FIELD(SurfaceMeshVis, colorMappingMode);
DEFINE_REFERENCE_FIELD(SurfaceMeshVis, surfaceTransparencyController);
DEFINE_REFERENCE_FIELD(SurfaceMeshVis, capTransparencyController);
DEFINE_REFERENCE_FIELD(SurfaceMeshVis, surfaceColorMapping);
DEFINE_SHADOW_PROPERTY_FIELD(SurfaceMeshVis, surfaceColor);
DEFINE_SHADOW_PROPERTY_FIELD(SurfaceMeshVis, capColor);
DEFINE_SHADOW_PROPERTY_FIELD(SurfaceMeshVis, showCap);
DEFINE_SHADOW_PROPERTY_FIELD(SurfaceMeshVis, smoothShading);
DEFINE_SHADOW_PROPERTY_FIELD(SurfaceMeshVis, reverseOrientation);
DEFINE_SHADOW_PROPERTY_FIELD(SurfaceMeshVis, highlightEdges);
SET_PROPERTY_FIELD_LABEL(SurfaceMeshVis, surfaceColor, "Surface color");
SET_PROPERTY_FIELD_LABEL(SurfaceMeshVis, capColor, "Cap color");
SET_PROPERTY_FIELD_LABEL(SurfaceMeshVis, showCap, "Show cap polygons");
SET_PROPERTY_FIELD_LABEL(SurfaceMeshVis, smoothShading, "Smooth shading");
SET_PROPERTY_FIELD_LABEL(SurfaceMeshVis, surfaceTransparencyController, "Surface transparency");
SET_PROPERTY_FIELD_LABEL(SurfaceMeshVis, capTransparencyController, "Cap transparency");
SET_PROPERTY_FIELD_LABEL(SurfaceMeshVis, reverseOrientation, "Flip surface orientation");
SET_PROPERTY_FIELD_LABEL(SurfaceMeshVis, highlightEdges, "Highlight edges");
SET_PROPERTY_FIELD_LABEL(SurfaceMeshVis, surfaceIsClosed, "Closed surface");
SET_PROPERTY_FIELD_LABEL(SurfaceMeshVis, surfaceColorMapping, "Color mapping");
SET_PROPERTY_FIELD_LABEL(SurfaceMeshVis, colorMappingMode, "Color mapping mode");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(SurfaceMeshVis, surfaceTransparencyController, PercentParameterUnit, 0, 1);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(SurfaceMeshVis, capTransparencyController, PercentParameterUnit, 0, 1);

IMPLEMENT_OVITO_CLASS(SurfaceMeshPickInfo);

/******************************************************************************
* Constructor.
******************************************************************************/
SurfaceMeshVis::SurfaceMeshVis(ObjectCreationParams params) : TransformingDataVis(params),
	_surfaceColor(1, 1, 1),
	_capColor(0.8, 0.8, 1.0),
	_showCap(true),
	_smoothShading(true),
	_reverseOrientation(false),
	_highlightEdges(false),
	_surfaceIsClosed(true),
	_colorMappingMode(NoPseudoColoring)
{
	if(params.createSubObjects()) {
		// Create animation controllers for the transparency parameters.
		setSurfaceTransparencyController(ControllerManager::createFloatController(dataset()));
		setCapTransparencyController(ControllerManager::createFloatController(dataset()));

		// Create a color mapping object for pseudo-color visualization of a surface property.
		setSurfaceColorMapping(OORef<PropertyColorMapping>::create(params));
	}
}

/******************************************************************************
* This method is called once for this object after it has been completely
* loaded from a stream.
******************************************************************************/
void SurfaceMeshVis::loadFromStreamComplete(ObjectLoadStream& stream)
{
	TransformingDataVis::loadFromStreamComplete(stream);

	// For backward compatibility with OVITO 3.5.4.
	// Create a color mapping sub-object if it wasn't loaded from the state file.
	if(!surfaceColorMapping())
		setSurfaceColorMapping(OORef<PropertyColorMapping>::create(dataset()));
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void SurfaceMeshVis::propertyChanged(const PropertyFieldDescriptor* field)
{
	if(field == PROPERTY_FIELD(smoothShading) || field == PROPERTY_FIELD(reverseOrientation) || field == PROPERTY_FIELD(colorMappingMode)) {
		// This kind of parameter change triggers a regeneration of the cached RenderableSurfaceMesh.
		invalidateTransformedObjects();
	}

	// Whenever the pseudo-coloring mode is changed, update the source property reference.
	if(field == PROPERTY_FIELD(colorMappingMode) && !isBeingLoaded() && !isAboutToBeDeleted() && !dataset()->undoStack().isUndoingOrRedoing() && surfaceColorMapping()) {
		const PropertyContainerClass* newContainerClass = nullptr;
		if(colorMappingMode() == VertexPseudoColoring) newContainerClass = &SurfaceMeshVertices::OOClass();
		else if(colorMappingMode() == FacePseudoColoring) newContainerClass = &SurfaceMeshFaces::OOClass();
		else if(colorMappingMode() == RegionPseudoColoring) newContainerClass = &SurfaceMeshRegions::OOClass();
		if(newContainerClass)
			surfaceColorMapping()->setSourceProperty(surfaceColorMapping()->sourceProperty().convertToContainerClass(newContainerClass));
	}

	TransformingDataVis::propertyChanged(field);
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool SurfaceMeshVis::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(source == surfaceColorMapping() && event.type() == ReferenceEvent::TargetChanged) {
		if(static_cast<const TargetChangedEvent&>(event).field() == PROPERTY_FIELD(PropertyColorMapping::sourceProperty)) {
			// This kind of parameter change triggers a regeneration of the cached RenderableSurfaceMesh.
			invalidateTransformedObjects();
		}
	}
	return TransformingDataVis::referenceEvent(source, event);
}

/******************************************************************************
* Is called when the value of a reference field of this RefMaker changes.
******************************************************************************/
void SurfaceMeshVis::referenceReplaced(const PropertyFieldDescriptor* field, RefTarget* oldTarget, RefTarget* newTarget, int listIndex)
{
	if(field == PROPERTY_FIELD(surfaceColorMapping)) {
		// This kind of parameter change triggers a regeneration of the cached RenderableSurfaceMesh.
		invalidateTransformedObjects();
	}
	TransformingDataVis::referenceReplaced(field, oldTarget, newTarget, listIndex);
}

/******************************************************************************
* Lets the vis element transform a data object in preparation for rendering.
******************************************************************************/
Future<PipelineFlowState> SurfaceMeshVis::transformDataImpl(const PipelineEvaluationRequest& request, const DataObject* dataObject, PipelineFlowState&& flowState)
{
	// Get the input surface mesh.
	const SurfaceMesh* surfaceMesh = dynamic_object_cast<SurfaceMesh>(dataObject);
	if(!surfaceMesh)
		return std::move(flowState);

	// Make sure the surface mesh is ok.
	surfaceMesh->verifyMeshIntegrity();

	// Create compute engine.
	auto engine = createSurfaceEngine(surfaceMesh);

	// Submit engine for execution and post-process results.
	return engine->runAsync(taskManager())
		.then(executor(), [this, flowState = std::move(flowState), dataObject = OORef<DataObject>(dataObject)](DataOORef<const TriMeshObject>&& surfaceMesh, DataOORef<const TriMeshObject>&& capPolygonsMesh, std::vector<ColorA>&& materialColors, std::vector<size_t>&& originalFaceMap, bool renderFacesTwoSided, PipelineStatus&& status) mutable {
			// Output the computed mesh as a RenderableSurfaceMesh.
			DataOORef<RenderableSurfaceMesh> renderableMesh = DataOORef<RenderableSurfaceMesh>::create(dataset(), 
				ObjectCreationParams::WithoutVisElement, this, dataObject, std::move(surfaceMesh), std::move(capPolygonsMesh), !renderFacesTwoSided);
            renderableMesh->setVisElement(this);
			renderableMesh->setMaterialColors(std::move(materialColors));
            renderableMesh->setOriginalFaceMap(std::move(originalFaceMap));
			flowState.addObject(std::move(renderableMesh));
			if(flowState.status().type() != PipelineStatus::Error && status.type() != PipelineStatus::Success)
				flowState.setStatus(std::move(status));
			return std::move(flowState);
		});
}

/******************************************************************************
* Computes the bounding box of the displayed data.
******************************************************************************/
Box3 SurfaceMeshVis::boundingBox(TimePoint time, const ConstDataObjectPath& path, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval)
{
	Box3 bb;

	// Compute mesh bounding box.
	// Requires that the periodic SurfaceMesh has already been transformed into a non-periodic RenderableSurfaceMesh.
	if(const RenderableSurfaceMesh* meshObj = dynamic_object_cast<RenderableSurfaceMesh>(path.back())) {
		if(meshObj->surfaceMesh()) bb.addBox(meshObj->surfaceMesh()->boundingBox());
		if(meshObj->capPolygonsMesh()) bb.addBox(meshObj->capPolygonsMesh()->boundingBox());
	}
	return bb;
}

/******************************************************************************
* Lets the visualization element render the data object.
******************************************************************************/
PipelineStatus SurfaceMeshVis::render(TimePoint time, const ConstDataObjectPath& path, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode)
{
	// Ignore render calls for the original SurfaceMesh.
	// We are only interested in the RenderableSurfaceMesh.
	if(dynamic_object_cast<SurfaceMesh>(path.back()) != nullptr)
		return {};

	if(renderer->isBoundingBoxPass()) {
		TimeInterval validityInterval;
		renderer->addToLocalBoundingBox(boundingBox(time, path, contextNode, flowState, validityInterval));
		return {};
	}

	// Get the rendering colors for the surface and cap meshes.
	FloatType surface_alpha = 1;
	FloatType cap_alpha = 1;
	TimeInterval iv;
	if(surfaceTransparencyController())
		surface_alpha = qBound(0.0, FloatType(1) - surfaceTransparencyController()->getFloatValue(time, iv), 1.0);
	if(capTransparencyController())
		cap_alpha = qBound(0.0, FloatType(1) - capTransparencyController()->getFloatValue(time, iv), 1.0);
	ColorA color_surface(colorMappingMode() == NoPseudoColoring ? surfaceColor() : Color(1,1,1), surface_alpha);
	ColorA color_cap(capColor(), cap_alpha);

	// The key type used for caching the surface primitive:
	using SurfaceCacheKey = RendererResourceKey<struct SurfaceMeshCache,
		ConstDataObjectRef,			// Mesh object
		ColorA,						// Surface color
		ColorA,						// Cap color
		bool						// Edge highlighting
	>;

    // The values stored in the vis cache.
    struct CacheValue {
        MeshPrimitive surfacePrimitive;
        MeshPrimitive capPrimitive;
        OORef<ObjectPickInfo> pickInfo;
    };

    // Get the renderable mesh.
    const RenderableSurfaceMesh* renderableMesh = dynamic_object_cast<RenderableSurfaceMesh>(path.back());
    if(!renderableMesh) return {};

	// Lookup the rendering primitive in the vis cache.
    auto& visCache = dataset()->visCache().get<CacheValue>(SurfaceCacheKey(path.back(), color_surface, color_cap, highlightEdges()));

	// Check if we already have a valid rendering primitive that is up to date.
	if(!visCache.surfacePrimitive.mesh()) {
		auto materialColors = renderableMesh->materialColors();
		for(ColorA& c : materialColors) {
			c.a() = surface_alpha;
		}
		visCache.surfacePrimitive.setMaterialColors(std::move(materialColors));
		visCache.surfacePrimitive.setUniformColor(color_surface);
		visCache.surfacePrimitive.setEmphasizeEdges(highlightEdges());
		visCache.surfacePrimitive.setCullFaces(renderableMesh->backfaceCulling());
		visCache.surfacePrimitive.setMesh(renderableMesh->surfaceMesh());

        // Get the original surface mesh.
        if(const SurfaceMesh* surfaceMesh = dynamic_object_cast<SurfaceMesh>(renderableMesh->sourceDataObject().get())) {
			// Create the pick record that keeps a reference to the original data.
			visCache.pickInfo = createPickInfo(surfaceMesh, renderableMesh);
		}
	}

	// Check if we already have a valid rendering primitive that is up to date.
	if(!visCache.capPrimitive.mesh() && showCap()) {
		visCache.capPrimitive.setUniformColor(color_cap);
		visCache.capPrimitive.setMesh(renderableMesh->capPolygonsMesh(), MeshPrimitive::ConvexShapeMode);
	}
	else if(visCache.capPrimitive.mesh() && !showCap()) {
		visCache.capPrimitive.setMesh(nullptr);
	}

	// Handle picking of triangles.
	renderer->beginPickObject(contextNode, visCache.pickInfo);
	if(visCache.surfacePrimitive.mesh()) {

		// Update the color mapping.
		visCache.surfacePrimitive.setPseudoColorMapping(surfaceColorMapping()->pseudoColorMapping());

		renderer->renderMesh(visCache.surfacePrimitive);
	}
	if(showCap() && visCache.capPrimitive.mesh()) {
		if(!renderer->isPicking() || cap_alpha >= 1)
			renderer->renderMesh(visCache.capPrimitive);
	}
	renderer->endPickObject();

	return {};
}

/******************************************************************************
* Create the viewport picking record for the surface mesh object.
******************************************************************************/
OORef<ObjectPickInfo> SurfaceMeshVis::createPickInfo(const SurfaceMesh* mesh, const RenderableSurfaceMesh* renderableMesh) const
{
	OVITO_ASSERT(mesh);
	OVITO_ASSERT(renderableMesh);
    return new SurfaceMeshPickInfo(this, mesh, renderableMesh);
}

/******************************************************************************
* Returns a human-readable string describing the picked object,
* which will be displayed in the status bar by OVITO.
******************************************************************************/
QString SurfaceMeshPickInfo::infoString(PipelineSceneNode* objectNode, quint32 subobjectId)
{
    QString str = surfaceMesh()->objectTitle();

	// Display all the properties of the face and also the properties of the mesh region to which the face belongs.
    auto facetIndex = faceIndexFromSubObjectID(subobjectId);
	if(surfaceMesh()->faces() && facetIndex >= 0 && facetIndex < surfaceMesh()->faces()->elementCount()) {
		for(const PropertyObject* property : surfaceMesh()->faces()->properties()) {
	        if(facetIndex >= property->size()) continue;
			if(property->type() == SurfaceMeshFaces::SelectionProperty) continue;
			if(property->type() == SurfaceMeshFaces::ColorProperty) continue;
			if(property->type() == SurfaceMeshFaces::RegionProperty) continue;
			if(!str.isEmpty()) str += QStringLiteral("<sep>");
			str += QStringLiteral("<key>");
			str += property->name();
			str += QStringLiteral(":</key> ");
			if(property->dataType() == PropertyObject::Int) {
				ConstPropertyAccess<int, true> data(property);
				for(size_t component = 0; component < data.componentCount(); component++) {
					if(component != 0) str += QStringLiteral(", ");
					str += QString::number(data.get(facetIndex, component));
					if(property->elementTypes().empty() == false) {
						if(const ElementType* ptype = property->elementType(data.get(facetIndex, component))) {
							if(!ptype->name().isEmpty())
								str += QStringLiteral(" (%1)").arg(ptype->name());
						}
					}
				}
			}
			else if(property->dataType() == PropertyObject::Int64) {
				ConstPropertyAccess<qlonglong, true> data(property);
				for(size_t component = 0; component < property->componentCount(); component++) {
					if(component != 0) str += QStringLiteral(", ");
					str += QString::number(data.get(facetIndex, component));
				}
			}
			else if(property->dataType() == PropertyObject::Float) {
				ConstPropertyAccess<FloatType, true> data(property);
				for(size_t component = 0; component < property->componentCount(); component++) {
					if(component != 0) str += QStringLiteral(", ");
					str += QString::number(data.get(facetIndex, component));
				}
			}
			else {
				str += QStringLiteral("<%1>").arg(getQtTypeNameFromId(property->dataType()) ? getQtTypeNameFromId(property->dataType()) : QStringLiteral("unknown"));
			}
		}

		// Additionally, list all properties of the region to which the face belongs.
		if(ConstPropertyAccess<int> regionProperty = surfaceMesh()->faces()->getProperty(SurfaceMeshFaces::RegionProperty)) {
			if(facetIndex < regionProperty.size() && surfaceMesh()->regions()) {
				int regionIndex = regionProperty[facetIndex];
				if(!str.isEmpty()) str += QStringLiteral("<sep>");
				str += QStringLiteral("<key>Region:</key> %1").arg(regionIndex);
				for(const PropertyObject* property : surfaceMesh()->regions()->properties()) {
					if(regionIndex < 0 || regionIndex >= property->size()) continue;
					if(property->type() == SurfaceMeshRegions::SelectionProperty) continue;
					if(property->type() == SurfaceMeshRegions::ColorProperty) continue;
					str += QStringLiteral("<sep><key>");
					str += property->name();
					str += QStringLiteral(":</key> ");
					if(property->dataType() == PropertyObject::Int) {
						ConstPropertyAccess<int, true> data(property);
						for(size_t component = 0; component < property->componentCount(); component++) {
							if(component != 0) str += QStringLiteral(", ");
							str += QString::number(data.get(regionIndex, component));
							if(property->elementTypes().empty() == false) {
								if(const ElementType* ptype = property->elementType(data.get(regionIndex, component))) {
									if(!ptype->name().isEmpty())
										str += QStringLiteral(" (%1)").arg(ptype->name());
								}
							}
						}
					}
					else if(property->dataType() == PropertyObject::Int64) {
						ConstPropertyAccess<qlonglong, true> data(property);
						for(size_t component = 0; component < property->componentCount(); component++) {
							if(component != 0) str += QStringLiteral(", ");
							str += QString::number(data.get(regionIndex, component));
						}
					}
					else if(property->dataType() == PropertyObject::Float) {
						ConstPropertyAccess<FloatType, true> data(property);
						for(size_t component = 0; component < property->componentCount(); component++) {
							if(component != 0) str += QStringLiteral(", ");
							str += QString::number(data.get(regionIndex, component));
						}
					}
					else {
						str += QStringLiteral("<%1>").arg(getQtTypeNameFromId(property->dataType()) ? getQtTypeNameFromId(property->dataType()) : QStringLiteral("unknown"));
					}
				}
			}
		}
    }

    return str;
}

/******************************************************************************
* Creates the asynchronous task that builds the non-peridic representation of the input surface mesh.
******************************************************************************/
std::shared_ptr<SurfaceMeshVis::PrepareSurfaceEngine> SurfaceMeshVis::createSurfaceEngine(const SurfaceMesh* mesh) const
{
	return std::make_shared<PrepareSurfaceEngine>(
		mesh,
		reverseOrientation(),
		smoothShading(),
		colorMappingMode(),
		surfaceColorMapping()->sourceProperty(),
		colorMappingMode() == NoPseudoColoring ? surfaceColor() : Color(1,1,1),
		surfaceIsClosed());
}

/******************************************************************************
* Computes the results and stores them in this object for later retrieval.
******************************************************************************/
void SurfaceMeshVis::PrepareSurfaceEngine::perform()
{
	setProgressText(tr("Preparing mesh for display"));
	if(_generateCapPolygons)
		beginProgressSubStepsWithWeights({1,1,12,1,8});
	else
		beginProgressSubStepsWithWeights({1,1,12,1});

	determineVisibleFaces();

	if(isCanceled()) return;
	nextProgressSubStep();

	// Create accessor for the input mesh data.
	const SurfaceMeshAccess inputMeshData(inputMesh());

	// Determine whether we can simply use two-sided rendering to display faces.
	// Thisis the case if there is no visible mesh face that has a 
	// corresponding opposite face.
	if(_faceSubset.empty()) {
		_renderFacesTwoSided = std::none_of(inputMeshData.topology()->begin_faces(), inputMeshData.topology()->end_faces(),
			std::bind(&SurfaceMeshTopology::hasOppositeFace, inputMeshData.topology(), std::placeholders::_1));
	}
	else {
		_renderFacesTwoSided = std::none_of(inputMeshData.topology()->begin_faces(), inputMeshData.topology()->end_faces(),
			[&](SurfaceMeshAccess::face_index face) { return _faceSubset[face] && inputMeshData.hasOppositeFace(face) && _faceSubset[inputMeshData.oppositeFace(face)]; });
	}

	if(isCanceled()) return;
	nextProgressSubStep();

	if(!buildSurfaceTriangleMesh() && !isCanceled())
		throw Exception(tr("Failed to build non-periodic representation of periodic surface mesh. Periodic domain might be too small."));

	if(isCanceled()) return;
	nextProgressSubStep();

	determineFaceColors();
	if(isCanceled()) return;

	if(_generateCapPolygons) {
		nextProgressSubStep();
		if(cell() && cell()->volume3D() > FLOATTYPE_EPSILON)
			buildCapTriangleMesh();
	}

	setResult(
		std::move(_outputMesh), 
		std::move(_capPolygonsMesh), 
		std::move(_materialColors), 
		std::move(_originalFaceMap), 
		_renderFacesTwoSided,
		std::move(_status));

	endProgressSubSteps();
}

/******************************************************************************
* Transfers face colors from the input to the output mesh.
******************************************************************************/
void SurfaceMeshVis::PrepareSurfaceEngine::determineFaceColors()
{
	ColorA defaultFaceColor(_surfaceColor);

	if(ConstPropertyAccess<Color> colorProperty = inputMesh()->faces()->getProperty(SurfaceMeshFaces::ColorProperty)) {
		// The "Color" property of mesh faces has the highest priority.
		// If it is present, use its information to color the triangle faces.
		outputMesh()->setHasFaceColors(true);
		auto meshFaceColor = outputMesh()->faceColors().begin();
		for(size_t originalFace : _originalFaceMap) {
			*meshFaceColor++ = colorProperty[originalFace];
		}
	}
	else if(ConstPropertyAccess<Color> colorProperty = inputMesh()->regions()->getProperty(SurfaceMeshRegions::ColorProperty)) {
		// If the "Color" property of mesh regions is present, use it information to color the 
		// mesh faces according to the region they belong to.
		if(ConstPropertyAccess<int> regionProperty = inputMesh()->faces()->getProperty(SurfaceMeshFaces::RegionProperty)) {
			outputMesh()->setHasFaceColors(true);
			size_t regionCount = colorProperty.size();
			auto meshFaceColor = outputMesh()->faceColors().begin();
			for(size_t originalFace : _originalFaceMap) {
				SurfaceMeshAccess::region_index regionIndex = regionProperty[originalFace];
				if(regionIndex >= 0 && regionIndex < regionCount)
					*meshFaceColor++ = colorProperty[regionIndex];
				else
					*meshFaceColor++ = defaultFaceColor;
			}
		}
	}
	else if(_colorMappingMode == FacePseudoColoring && _pseudoColorPropertyRef && inputMesh()->faces()) {
		if(const PropertyObject* pseudoColorProperty = _pseudoColorPropertyRef.findInContainer(inputMesh()->faces())) {
			if(_pseudoColorPropertyRef.vectorComponent() < (int)pseudoColorProperty->componentCount()) {
				outputMesh()->setHasFacePseudoColors(true);
				ConstPropertyAccess<void,true> pseudoColorArray(pseudoColorProperty);
				size_t vecComponent = std::max(0, _pseudoColorPropertyRef.vectorComponent());
				auto meshFacePseudoColor = outputMesh()->facePseudoColors().begin();
				for(size_t originalFace : _originalFaceMap) {
					*meshFacePseudoColor++ = pseudoColorArray.get<FloatType>(originalFace, vecComponent);
				}
			}
			else {
				_status = PipelineStatus(PipelineStatus::Error, tr("The vector component is out of range. The property '%1' has only %2 values per data element.").arg(_pseudoColorPropertyRef.name()).arg(pseudoColorProperty->componentCount()));
			}
		}
		else {
			_status = PipelineStatus(PipelineStatus::Error, tr("The face property with the name '%1' does not exist.").arg(_pseudoColorPropertyRef.name()));
		}
	}
	else if(_colorMappingMode == RegionPseudoColoring && _pseudoColorPropertyRef && inputMesh()->regions()) {
		if(const PropertyObject* pseudoColorProperty = _pseudoColorPropertyRef.findInContainer(inputMesh()->regions())) {
			if(_pseudoColorPropertyRef.vectorComponent() < (int)pseudoColorProperty->componentCount()) {
				if(ConstPropertyAccess<int> regionProperty = inputMesh()->faces()->getProperty(SurfaceMeshFaces::RegionProperty)) {
					outputMesh()->setHasFacePseudoColors(true);
					ConstPropertyAccess<void,true> pseudoColorArray(pseudoColorProperty);
					size_t vecComponent = std::max(0, _pseudoColorPropertyRef.vectorComponent());
					size_t regionCount = pseudoColorProperty->size();
					auto meshFacePseudoColor = outputMesh()->facePseudoColors().begin();
					for(size_t originalFace : _originalFaceMap) {
						SurfaceMeshAccess::region_index regionIndex = regionProperty[originalFace];
						if(regionIndex >= 0 && regionIndex < regionCount)
							*meshFacePseudoColor++ = pseudoColorArray.get<FloatType>(regionIndex, vecComponent);
						else
							*meshFacePseudoColor++ = 0;
					}
				}
			}
			else {
				_status = PipelineStatus(PipelineStatus::Error, tr("The vector component is out of range. The property '%1' has only %2 values per data element.").arg(_pseudoColorPropertyRef.name()).arg(pseudoColorProperty->componentCount()));
			}
		}
		else {
			_status = PipelineStatus(PipelineStatus::Error, tr("The region property with the name '%1' does not exist.").arg(_pseudoColorPropertyRef.name()));
		}
	}

	if(ConstPropertyAccess<int> selectionProperty = inputMesh()->faces()->getProperty(SurfaceMeshFaces::SelectionProperty)) {
		auto meshFace = outputMesh()->faces().begin();
		for(size_t originalFace : _originalFaceMap) {
			if(selectionProperty[originalFace])
				meshFace->setSelected();
			++meshFace;
		}
	}
	else if(ConstPropertyAccess<int> selectionProperty = inputMesh()->regions()->getProperty(SurfaceMeshRegions::SelectionProperty)) {
		// If the "Selection" property of mesh regions is present, use it information to highlight the 
		// mesh faces that belong to selected regions.
		if(ConstPropertyAccess<int> regionProperty = inputMesh()->faces()->getProperty(SurfaceMeshFaces::RegionProperty)) {
			size_t regionCount = selectionProperty.size();
			auto meshFace = outputMesh()->faces().begin();
			for(size_t originalFace : _originalFaceMap) {
				SurfaceMeshAccess::region_index regionIndex = regionProperty[originalFace];
				if(regionIndex >= 0 && regionIndex < regionCount && selectionProperty[regionIndex])
					meshFace->setSelected();
				++meshFace;
			}
		}
	}
}

/******************************************************************************
* Transfers vertex colors from the input to the output mesh.
******************************************************************************/
void SurfaceMeshVis::PrepareSurfaceEngine::determineVertexColors()
{
	if(ConstPropertyAccess<Color> colorProperty = inputMesh()->vertices()->getProperty(SurfaceMeshVertices::ColorProperty)) {
		OVITO_ASSERT(colorProperty.size() == outputMesh()->vertexCount());
		if(colorProperty.size() == outputMesh()->vertexCount()) {
			outputMesh()->setHasVertexColors(true);
			boost::copy(colorProperty, outputMesh()->vertexColors().begin());
		}
	}
	else if(_colorMappingMode == VertexPseudoColoring && _pseudoColorPropertyRef) {
		if(const PropertyObject* pseudoColorProperty = _pseudoColorPropertyRef.findInContainer(inputMesh()->vertices())) {
			OVITO_ASSERT(pseudoColorProperty->size() == outputMesh()->vertexCount());
			if(_pseudoColorPropertyRef.vectorComponent() < (int)pseudoColorProperty->componentCount()) {
				outputMesh()->setHasVertexPseudoColors(true);
				pseudoColorProperty->copyTo(outputMesh()->vertexPseudoColors().begin(), std::max(0, _pseudoColorPropertyRef.vectorComponent()));
			}
			else {
				_status = PipelineStatus(PipelineStatus::Error, tr("The vector component is out of range. The property '%1' has only %2 values per data element.").arg(_pseudoColorPropertyRef.name()).arg(pseudoColorProperty->componentCount()));
			}
		}
		else {
			_status = PipelineStatus(PipelineStatus::Error, tr("The vertex property with the name '%1' does not exist.").arg(_pseudoColorPropertyRef.name()));
		}
	}
}

/******************************************************************************
* Generates the triangle mesh from the periodic surface mesh, which will be rendered.
******************************************************************************/
bool SurfaceMeshVis::PrepareSurfaceEngine::buildSurfaceTriangleMesh()
{
	if(cell() && cell()->is2D())
		throw Exception(tr("Cannot generate surface triangle mesh when domain is two-dimensional."));

	beginProgressSubStepsWithWeights({1,1,1,1,1,1});

	// Create accessor for the input mesh data.
	const SurfaceMeshAccess inputMeshData(inputMesh());

	// Transfer vertices and faces from half-edge mesh structure to triangle mesh structure.
	_outputMesh = DataOORef<TriMeshObject>::create(inputMesh()->dataset(), ObjectCreationParams::WithoutVisElement);
	inputMeshData.convertToTriMesh(*outputMesh(), _smoothShading, _faceSubset, &_originalFaceMap, !_renderFacesTwoSided);

	// Check for early abortion.
	if(isCanceled())
		return false;
	nextProgressSubStep();

	// Assign mesh vertex colors if available.
	determineVertexColors();

	// Flip orientation of mesh faces if requested.
	if(_reverseOrientation)
		outputMesh()->flipFaces();

	// Check for early abortion.
	if(isCanceled())
		return false;
	nextProgressSubStep();

	// Convert vertex positions to reduced coordinates and transfer them to the output mesh.
	OVITO_ASSERT(outputMesh()->vertices().size() == inputMeshData.vertexCount());
	if(cell()) {
		SurfaceMeshAccess::vertex_index vidx = 0;
		for(Point3& p : outputMesh()->vertices()) {
			p = cell()->absoluteToReduced(inputMeshData.vertexPosition(vidx++));
			OVITO_ASSERT(std::isfinite(p.x()) && std::isfinite(p.y()) && std::isfinite(p.z()));
		}
	}

#if 0
	// Subdivide quadrilaterals, formed by pairs of adjacent triangles, into four smaller triangles 
	// in order to obtain a more symmetric interpolation of vertex colors. This is needed for 
	// correct visualization of voxel grid cross sections.
	if(_surfaceMesh.hasVertexColors() && cell()) {
		auto originalTriangleCount = _surfaceMesh.faceCount();
		for(int triangleIndex = 1; triangleIndex < originalTriangleCount; triangleIndex++) {

			// Check if the current pair of triangles are associated with the same source mesh face.
			auto originalFace = _originalFaceMap[triangleIndex];
			if(_originalFaceMap[triangleIndex - 1] != originalFace)
				continue;

			// Check the triangles' edge visibility: Do they form a quadrilateral?
			TriMeshFace& face1 = _surfaceMesh.face(triangleIndex - 1);
			TriMeshFace& face2 = _surfaceMesh.face(triangleIndex);
			if(!face1.edgeVisible(0) || !face1.edgeVisible(1) || face1.edgeVisible(2)) continue;
			if(face2.edgeVisible(0) || !face2.edgeVisible(1) || !face2.edgeVisible(2)) continue;

			// We have found a quadrilateral made of two triangles.

			// Compute center of the quadrilateral.
			SurfaceMesh::edge_index edge1 = _inputMesh.firstFaceEdge(originalFace);
			SurfaceMesh::edge_index edge2 = _inputMesh.nextFaceEdge(edge1);
			Point3 p = _inputMesh.vertexPosition(_inputMesh.vertex1(edge1));
			Vector3 edgeVector = _inputMesh.edgeVector(edge1) + _inputMesh.edgeVector(edge2);
			p += FloatType(0.5) * edgeVector;

			// Create additional vertex in the output mesh.
			int newVertex = _surfaceMesh.addVertex(cell()->absoluteToReduced(p));

			// Compute color of new vertex via interpolation.
			int v1 = face1.vertex(0);
			int v2 = face1.vertex(1);
			int v3 = face1.vertex(2);
			int v4 = face2.vertex(2);
			const ColorA& c1 = _surfaceMesh.vertexColor(v1);
			const ColorA& c2 = _surfaceMesh.vertexColor(v2);
			const ColorA& c3 = _surfaceMesh.vertexColor(v3);
			const ColorA& c4 = _surfaceMesh.vertexColor(v4);
			_surfaceMesh.setVertexColor(newVertex, FloatType(0.25) * (c1+c2+c3+c4));

			// Compute normal of new vertex via interpolation.
			Vector3 n2, n3, n4;
			Vector3 centerNormal;
			if(_surfaceMesh.hasNormals()) {
				const Vector3& n1 = _surfaceMesh.faceVertexNormal(triangleIndex - 1, 0);
				n2 = _surfaceMesh.faceVertexNormal(triangleIndex - 1, 1);
				n3 = _surfaceMesh.faceVertexNormal(triangleIndex - 1, 2);
				n4 = _surfaceMesh.faceVertexNormal(triangleIndex, 2);
				centerNormal = (n1+n2+n3+n4).safelyNormalized();
				_surfaceMesh.setFaceVertexNormal(triangleIndex - 1, 2, centerNormal);
				_surfaceMesh.setFaceVertexNormal(triangleIndex, 1, centerNormal);
			}

			face1.setVertex(2, newVertex);
			face1.setEdgeVisibility(true, false, false);
			face2.setVertex(1, newVertex);
			face2.setEdgeVisibility(false, false, true);

			// Create two additional triangles.
			TriMeshFace& face3 = _surfaceMesh.addFace();
			face3.setVertices(v2, v3, newVertex);
			face3.setEdgeVisibility(true, false, false);
			TriMeshFace& face4 = _surfaceMesh.addFace();
			face4.setVertices(v3, v4, newVertex);
			face4.setEdgeVisibility(true, false, false);
			if(_surfaceMesh.hasNormals()) {
				auto n = _surfaceMesh.normals().end() - 6;
				*n++ = n2;
				*n++ = n3;
				*n++ = centerNormal;
				*n++ = n3;
				*n++ = n4;
				*n++ = centerNormal;
			}

			_originalFaceMap.push_back(originalFace);
			_originalFaceMap.push_back(originalFace);

			if(isCanceled())
				return false;
		}
	}
#endif

	nextProgressSubStep();

	// Wrap mesh at periodic boundaries.
	for(size_t dim = 0; dim < 3; dim++) {
		if(!cell() || cell()->hasPbc(dim) == false) continue;

		if(isCanceled())
			return false;

		// Make sure all vertices are located inside the periodic box.
		for(Point3& p : outputMesh()->vertices()) {
			OVITO_ASSERT(std::isfinite(p[dim]));
			p[dim] -= std::floor(p[dim]);
			OVITO_ASSERT(p[dim] >= FloatType(0) && p[dim] <= FloatType(1));
		}

		// Split triangle faces at periodic boundaries.
		int oldFaceCount = outputMesh()->faceCount();
		int oldVertexCount = outputMesh()->vertexCount();
		std::vector<Point3> newVertices;
		std::vector<ColorA> newVertexColors;
		std::vector<FloatType> newVertexPseudoColors;
		std::map<std::pair<int,int>,std::tuple<int,int,FloatType>> newVertexLookupMap;
		for(int findex = 0; findex < oldFaceCount; findex++) {
			if(!splitFace(findex, oldVertexCount, newVertices, newVertexColors, newVertexPseudoColors, newVertexLookupMap, dim)) {
				return false;
			}
		}

		// Insert newly created vertices into mesh.
		outputMesh()->setVertexCount(oldVertexCount + newVertices.size());
		std::copy(newVertices.cbegin(), newVertices.cend(), outputMesh()->vertices().begin() + oldVertexCount);
		if(outputMesh()->hasVertexColors()) {
			OVITO_ASSERT(newVertexColors.size() == newVertices.size());
			std::copy(newVertexColors.cbegin(), newVertexColors.cend(), outputMesh()->vertexColors().begin() + oldVertexCount);
		}
		if(outputMesh()->hasVertexPseudoColors()) {
			OVITO_ASSERT(newVertexPseudoColors.size() == newVertices.size());
			std::copy(newVertexPseudoColors.cbegin(), newVertexPseudoColors.cend(), outputMesh()->vertexPseudoColors().begin() + oldVertexCount);
		}
	}
	if(isCanceled())
		return false;

	nextProgressSubStep();

	// Convert vertex positions back from reduced coordinates to absolute coordinates.
	if(cell()) {
		const AffineTransformation cellMatrix = cell()->matrix();
		for(Point3& p : outputMesh()->vertices())
			p = cellMatrix * p;
	}

	nextProgressSubStep();

	// Clip mesh at cutting planes.
	if(!inputMesh()->cuttingPlanes().empty()) {
		auto of = _originalFaceMap.begin();
		for(TriMeshFace& face : outputMesh()->faces())
			face.setMaterialIndex(*of++);

		for(const Plane3& plane : inputMesh()->cuttingPlanes()) {
			if(isCanceled())
				return false;

			outputMesh()->clipAtPlane(plane);
		}

		_originalFaceMap.resize(outputMesh()->faces().size());
		of = _originalFaceMap.begin();
		for(TriMeshFace& face : outputMesh()->faces())
			*of++ = face.materialIndex();
	}

	outputMesh()->invalidateVertices();
	OVITO_ASSERT(_originalFaceMap.size() == outputMesh()->faces().size());

	endProgressSubSteps();
	return true;
}

/******************************************************************************
* Splits a triangle face at a periodic boundary.
******************************************************************************/
bool SurfaceMeshVis::PrepareSurfaceEngine::splitFace(int faceIndex, int oldVertexCount, std::vector<Point3>& newVertices, std::vector<ColorA>& newVertexColors,
		std::vector<FloatType>& newVertexPseudoColors, std::map<std::pair<int,int>,std::tuple<int,int,FloatType>>& newVertexLookupMap, size_t dim)
{
    TriMeshFace& face = outputMesh()->face(faceIndex);
	OVITO_ASSERT(face.vertex(0) != face.vertex(1));
	OVITO_ASSERT(face.vertex(1) != face.vertex(2));
	OVITO_ASSERT(face.vertex(2) != face.vertex(0));

	FloatType z[3];
	for(int v = 0; v < 3; v++)
		z[v] = outputMesh()->vertex(face.vertex(v))[dim];
	FloatType zd[3] = { z[1] - z[0], z[2] - z[1], z[0] - z[2] };

	OVITO_ASSERT(z[1] - z[0] == -(z[0] - z[1]));
	OVITO_ASSERT(z[2] - z[1] == -(z[1] - z[2]));
	OVITO_ASSERT(z[0] - z[2] == -(z[2] - z[0]));

	if(std::abs(zd[0]) < FloatType(0.5) && std::abs(zd[1]) < FloatType(0.5) && std::abs(zd[2]) < FloatType(0.5))
		return true;	// Face does not cross the periodic boundary.

	// Create four new vertices (or use existing ones created during splitting of adjacent faces).
	int properEdge = -1;
	int newVertexIndices[3][2];
	Vector3 interpolatedNormals[3];
	for(int i = 0; i < 3; i++) {
		if(std::abs(zd[i]) < FloatType(0.5)) {
			if(properEdge != -1)
				return false;		// The simulation box may be too small or invalid.
			properEdge = i;
			continue;
		}
		int vi1 = face.vertex(i);
		int vi2 = face.vertex((i+1)%3);
		int oi1, oi2;
		if(zd[i] <= FloatType(-0.5)) {
			std::swap(vi1, vi2);
			oi1 = 1; oi2 = 0;
		}
		else {
			oi1 = 0; oi2 = 1;
		}
		auto entry = newVertexLookupMap.find(std::make_pair(vi1, vi2));
		if(entry != newVertexLookupMap.end()) {
			newVertexIndices[i][oi1] = std::get<0>(entry->second);
			newVertexIndices[i][oi2] = std::get<1>(entry->second);
		}
		else {
			Vector3 delta = outputMesh()->vertex(vi2) - outputMesh()->vertex(vi1);
			delta[dim] -= FloatType(1);
			if(cell()) {
				for(size_t d = dim + 1; d < 3; d++) {
					if(cell()->hasPbc(d))
						delta[d] -= std::floor(delta[d] + FloatType(0.5));
				}
			}
			FloatType t;
			if(delta[dim] != 0)
				t = outputMesh()->vertex(vi1)[dim] / (-delta[dim]);
			else
				t = FloatType(0.5);
			OVITO_ASSERT(std::isfinite(t));
			Point3 p = delta * t + outputMesh()->vertex(vi1);
			newVertexIndices[i][oi1] = oldVertexCount + (int)newVertices.size();
			newVertexIndices[i][oi2] = oldVertexCount + (int)newVertices.size() + 1;
			entry = newVertexLookupMap.emplace(std::make_pair(vi1, vi2), std::make_tuple(newVertexIndices[i][oi1], newVertexIndices[i][oi2], t)).first;
			newVertices.push_back(p);
			p[dim] += FloatType(1);
			newVertices.push_back(p);
			// Compute the color at the intersection point by interpolating the colors of the two existing vertices.
			if(outputMesh()->hasVertexColors()) {
				const ColorA& color1 = outputMesh()->vertexColor(vi1);
				const ColorA& color2 = outputMesh()->vertexColor(vi2);
				ColorA interp_color(color1.r() + (color2.r() - color1.r()) * t,
									color1.g() + (color2.g() - color1.g()) * t,
									color1.b() + (color2.b() - color1.b()) * t,
									color1.a() + (color2.a() - color1.a()) * t);
				newVertexColors.push_back(interp_color);
				newVertexColors.push_back(interp_color);
			}
			if(outputMesh()->hasVertexPseudoColors()) {
				FloatType pseudocolor1 = outputMesh()->vertexPseudoColor(vi1);
				FloatType pseudocolor2 = outputMesh()->vertexPseudoColor(vi2);
				FloatType interp_pseudocolor = pseudocolor1 + (pseudocolor2 - pseudocolor1) * t;
				newVertexPseudoColors.push_back(interp_pseudocolor);
				newVertexPseudoColors.push_back(interp_pseudocolor);
			}
		}
		// Compute interpolated normal vector at intersection point.
		if(_smoothShading) {
			const Vector3& n1 = outputMesh()->faceVertexNormal(faceIndex, (i+oi1)%3);
			const Vector3& n2 = outputMesh()->faceVertexNormal(faceIndex, (i+oi2)%3);
			FloatType t = std::get<2>(entry->second);
			interpolatedNormals[i] = n1*t + n2*(FloatType(1)-t);
			interpolatedNormals[i].normalizeSafely();
		}
	}
	OVITO_ASSERT(properEdge != -1);

	// Build output triangles.
	int originalVertices[3] = { face.vertex(0), face.vertex(1), face.vertex(2) };
	bool originalEdgeVisibility[3] = { face.edgeVisible(0), face.edgeVisible(1), face.edgeVisible(2) };
	int pe1 = (properEdge+1)%3;
	int pe2 = (properEdge+2)%3;
	face.setVertices(originalVertices[properEdge], originalVertices[pe1], newVertexIndices[pe2][1]);
	face.setEdgeVisibility(originalEdgeVisibility[properEdge], false, originalEdgeVisibility[pe2]);

    int materialIndex = face.materialIndex();
	OVITO_ASSERT(_originalFaceMap.size() == outputMesh()->faceCount());
	outputMesh()->setFaceCount(outputMesh()->faceCount() + 2);
    _originalFaceMap.resize(_originalFaceMap.size() + 2, _originalFaceMap[faceIndex]);
	TriMeshFace& newFace1 = outputMesh()->face(outputMesh()->faceCount() - 2);
	TriMeshFace& newFace2 = outputMesh()->face(outputMesh()->faceCount() - 1);
	newFace1.setVertices(originalVertices[pe1], newVertexIndices[pe1][0], newVertexIndices[pe2][1]);
	newFace2.setVertices(newVertexIndices[pe1][1], originalVertices[pe2], newVertexIndices[pe2][0]);
    newFace1.setMaterialIndex(materialIndex);
    newFace2.setMaterialIndex(materialIndex);
	newFace1.setEdgeVisibility(originalEdgeVisibility[pe1], false, false);
	newFace2.setEdgeVisibility(originalEdgeVisibility[pe1], originalEdgeVisibility[pe2], false);
	if(_smoothShading) {
		auto n = outputMesh()->normals().end() - 6;
		*n++ = outputMesh()->faceVertexNormal(faceIndex, pe1);
		*n++ = interpolatedNormals[pe1];
		*n++ = interpolatedNormals[pe2];
		*n++ = interpolatedNormals[pe1];
		*n++ = outputMesh()->faceVertexNormal(faceIndex, pe2);
		*n   = interpolatedNormals[pe2];
		n = outputMesh()->normals().begin() + faceIndex*3;
		std::rotate(n, n + properEdge, n + 3);
		outputMesh()->setFaceVertexNormal(faceIndex, 2, interpolatedNormals[pe2]);
	}

	return true;
}

/******************************************************************************
* Generates the cap polygons where the surface mesh intersects the
* periodic domain boundaries.
******************************************************************************/
void SurfaceMeshVis::PrepareSurfaceEngine::buildCapTriangleMesh()
{
	OVITO_ASSERT(cell());

	// Create the output mesh object.
	_capPolygonsMesh = DataOORef<TriMeshObject>::create(inputMesh()->dataset(), ObjectCreationParams::WithoutVisElement);

	// Create accessor for the input mesh data.
	const SurfaceMeshAccess inputMeshData(inputMesh());

	// Access the 'Filled' property of volumetric regions if it is defined for the input surface mesh.
    ConstPropertyAccess<int> isFilledProperty(inputMeshData.regionProperty(SurfaceMeshRegions::IsFilledProperty));
	bool hasRegions = isFilledProperty && inputMeshData.hasFaceRegions();
	bool flipCapNormal = (cell()->matrix().determinant() < 0);

	// Convert vertex positions to reduced coordinates.
	AffineTransformation invCellMatrix = cell()->inverseMatrix();
	if(flipCapNormal)
		invCellMatrix.column(0) = -invCellMatrix.column(0);

	std::vector<Point3> reducedPos(inputMeshData.vertexCount());
	SurfaceMeshAccess::vertex_index vidx = 0;
	for(Point3& p : reducedPos)
		p = invCellMatrix * inputMeshData.vertexPosition(vidx++);

	int isBoxCornerInside3DRegion = -1;

	// Create caps on each side of the simulation with periodic boundary conditions.
	for(size_t dim = 0; dim < 3; dim++) {
		if(!cell()->hasPbc(dim)) continue;

		if(isCanceled())
			return;

		// Make sure all vertices are located inside the periodic box.
		for(Point3& p : reducedPos) {
			FloatType& c = p[dim];
			OVITO_ASSERT(std::isfinite(c));
			if(FloatType s = std::floor(c))
				c -= s;
		}

		// Used to keep track of already visited faces during the current pass.
		std::vector<bool> visitedFaces(inputMeshData.faceCount(), false);

		// The lists of 2d contours generated by clipping the 3d surface mesh.
		std::vector<std::vector<Point2>> openContours;
		std::vector<std::vector<Point2>> closedContours;

		// Find a first edge that crosses a periodic cell boundary.
		for(SurfaceMeshAccess::face_index face : _originalFaceMap) {
			// Skip faces that have already been visited.
			if(visitedFaces[face]) continue;
			if(isCanceled()) return;
			visitedFaces[face] = true;

			// Determine whether the mesh face is bordering a filled or an empty region.
			if(hasRegions) {
				SurfaceMeshAccess::region_index region = inputMeshData.faceRegion(face);
				if(region >= 0 && region < isFilledProperty.size()) {
					if((bool)isFilledProperty[region] == _reverseOrientation) {
						// Skip faces that are adjacent to an empty volumetric region.
						continue;
					}

					// Also skip any two-sided faces that are part of an interior interface.
					SurfaceMeshAccess::face_index oppositeFace = inputMeshData.oppositeFace(face);
					if(oppositeFace != SurfaceMeshAccess::InvalidIndex) {						
						SurfaceMeshAccess::region_index oppositeRegion = inputMeshData.faceRegion(oppositeFace);
						if(oppositeRegion >= 0 && oppositeRegion < isFilledProperty.size()) {
							if((bool)isFilledProperty[oppositeRegion] != _reverseOrientation) {
								continue;
							}
						}
					}
				}
			}

			SurfaceMeshAccess::edge_index startEdge = inputMeshData.firstFaceEdge(face);
			SurfaceMeshAccess::edge_index edge = startEdge;
			do {
				const Point3& v1 = reducedPos[inputMeshData.vertex1(edge)];
				const Point3& v2 = reducedPos[inputMeshData.vertex2(edge)];
				if(v2[dim] - v1[dim] >= FloatType(0.5)) {
					std::vector<Point2> contour = traceContour(inputMeshData, edge, reducedPos, visitedFaces, dim);
					if(contour.empty())
						throw Exception(tr("Surface mesh is not a proper manifold."));
					clipContour(contour, std::array<bool,2>{{ cell()->hasPbc((dim+1)%3), cell()->hasPbc((dim+2)%3) }}, openContours, closedContours);
					break;
				}
				edge = inputMeshData.nextFaceEdge(edge);
			}
			while(edge != startEdge);
		}

		// Invert surface orientation if requested. (Not needed if regions are defined. Then we can just swap roles of filled and empty regions).
		if(!hasRegions && _reverseOrientation) {
			for(auto& contour : openContours)
				std::reverse(std::begin(contour), std::end(contour));
		}

		// Feed contours into tessellator to create triangles.
		CapPolygonTessellator tessellator(*_capPolygonsMesh, dim);
		tessellator.beginPolygon();
		for(const auto& contour : closedContours) {
			if(isCanceled())
				return;
			tessellator.beginContour();
			for(const Point2& p : contour) {
				tessellator.vertex(p);
			}
			tessellator.endContour();
		}

		auto yxCoord2ArcLength = [](const Point2& p) {
			if(p.x() == 0) return p.y();
			else if(p.y() == 1) return p.x() + FloatType(1);
			else if(p.x() == 1) return FloatType(3) - p.y();
			else return std::fmod(FloatType(4) - p.x(), FloatType(4));
		};

		// Build the outer contour.
		if(!openContours.empty()) {
			boost::dynamic_bitset<> visitedContours(openContours.size());
			for(auto c1 = openContours.begin(); c1 != openContours.end(); ++c1) {
				if(isCanceled())
					return;
				if(!visitedContours.test(c1 - openContours.begin())) {
					tessellator.beginContour();
					auto currentContour = c1;
					do {
						for(const Point2& p : *currentContour) {
							tessellator.vertex(p);
						}
						visitedContours.set(currentContour - openContours.begin());

						FloatType t_exit = yxCoord2ArcLength(currentContour->back());

						// Find the next contour.
						FloatType t_entry;
						FloatType closestDist = FLOATTYPE_MAX;
						for(auto c = openContours.begin(); c != openContours.end(); ++c) {
							FloatType t = yxCoord2ArcLength(c->front());
							FloatType dist = t_exit - t;
							if(dist < 0) dist += FloatType(4);
							if(dist < closestDist) {
								closestDist = dist;
								currentContour = c;
								t_entry = t;
							}
						}
						int exitCorner = (int)std::floor(t_exit);
						int entryCorner = (int)std::floor(t_entry);
						if(exitCorner < 0 || exitCorner >= 4) break;
						if(entryCorner < 0 || entryCorner >= 4) break;
						if(exitCorner != entryCorner || t_exit < t_entry) {
							for(int corner = exitCorner;;) {
								switch(corner) {
								case 0: tessellator.vertex(Point2(0,0)); break;
								case 1: tessellator.vertex(Point2(0,1)); break;
								case 2: tessellator.vertex(Point2(1,1)); break;
								case 3: tessellator.vertex(Point2(1,0)); break;
								}
								corner = (corner + 3) % 4;
								if(corner == entryCorner) break;
							}
						}
					}
					while(!visitedContours.test(currentContour - openContours.begin()));
					tessellator.endContour();
				}
			}
		}
		else {
			if(isBoxCornerInside3DRegion == -1) {
				if(closedContours.empty()) {
					if(std::optional<std::pair<SurfaceMeshAccess::region_index, FloatType>> region = inputMeshData.locatePoint(cell()->cellOrigin(), 0, _faceSubset)) {
						if(hasRegions) {
							if(region->first >= 0 && region->first < isFilledProperty.size()) {
								isBoxCornerInside3DRegion = (bool)isFilledProperty[region->first];
							}
							else
								isBoxCornerInside3DRegion = false;
						}
						else {
							isBoxCornerInside3DRegion = region->first != SurfaceMeshAccess::InvalidIndex;
						}
					}
					else {
						isBoxCornerInside3DRegion = false;
					}
				}
				else {
					isBoxCornerInside3DRegion = isCornerInside2DRegion(closedContours);
					if(hasRegions && _reverseOrientation)
						isBoxCornerInside3DRegion = !isBoxCornerInside3DRegion;
				}
				if(_reverseOrientation)
					isBoxCornerInside3DRegion = !isBoxCornerInside3DRegion;
			}
			if(isBoxCornerInside3DRegion) {
				tessellator.beginContour();
				tessellator.vertex(Point2(0,0));
				tessellator.vertex(Point2(1,0));
				tessellator.vertex(Point2(1,1));
				tessellator.vertex(Point2(0,1));
				tessellator.endContour();
			}
		}

		tessellator.endPolygon();
	}

	// Check for early abortion.
	if(isCanceled())
		return;

	// Convert vertex positions back from reduced coordinates to absolute coordinates.
	const AffineTransformation cellMatrix = invCellMatrix.inverse();
	for(Point3& p : _capPolygonsMesh->vertices())
		p = cellMatrix * p;

	// Clip mesh at cutting planes.
	for(const Plane3& plane : inputMesh()->cuttingPlanes()) {
		if(isCanceled())
			return;
		_capPolygonsMesh->clipAtPlane(plane);
	}
}

/******************************************************************************
* Traces the closed contour of the surface-boundary intersection.
******************************************************************************/
std::vector<Point2> SurfaceMeshVis::PrepareSurfaceEngine::traceContour(const SurfaceMeshAccess& inputMeshData, SurfaceMesh::edge_index firstEdge, const std::vector<Point3>& reducedPos, std::vector<bool>& visitedFaces, size_t dim) const
{
	OVITO_ASSERT(cell());
	size_t dim1 = (dim + 1) % 3;
	size_t dim2 = (dim + 2) % 3;
	std::vector<Point2> contour;
	SurfaceMeshAccess::edge_index edge = firstEdge;
	do {
		OVITO_ASSERT(inputMeshData.adjacentFace(edge) != SurfaceMeshAccess::InvalidIndex);

		// Mark face as visited.
		visitedFaces[inputMeshData.adjacentFace(edge)] = true;

		// Compute intersection point.
		Point3 v1 = reducedPos[inputMeshData.vertex1(edge)];
		Point3 v2 = reducedPos[inputMeshData.vertex2(edge)];
		Vector3 delta = v2 - v1;
		OVITO_ASSERT(delta[dim] >= FloatType(0.5));

		delta[dim] -= FloatType(1);
		if(cell()->hasPbc(dim1)) {
			FloatType& c = delta[dim1];
			if(FloatType s = std::floor(c + FloatType(0.5)))
				c -= s;
		}
		if(cell()->hasPbc(dim2)) {
			FloatType& c = delta[dim2];
			if(FloatType s = std::floor(c + FloatType(0.5)))
				c -= s;
		}
		if(std::abs(delta[dim]) > FloatType(1e-9f)) {
			FloatType t = v1[dim] / delta[dim];
			FloatType x = v1[dim1] - delta[dim1] * t;
			FloatType y = v1[dim2] - delta[dim2] * t;
			OVITO_ASSERT(std::isfinite(x) && std::isfinite(y));
			if(contour.empty() || std::abs(x - contour.back().x()) > FLOATTYPE_EPSILON || std::abs(y - contour.back().y()) > FLOATTYPE_EPSILON) {
				contour.push_back({x,y});
			}
		}
		else {
			FloatType x1 = v1[dim1];
			FloatType y1 = v1[dim2];
			FloatType x2 = v1[dim1] + delta[dim1];
			FloatType y2 = v1[dim2] + delta[dim2];
			if(contour.empty() || std::abs(x1 - contour.back().x()) > FLOATTYPE_EPSILON || std::abs(y1 - contour.back().y()) > FLOATTYPE_EPSILON) {
				contour.push_back({x1,y1});
			}
			else if(contour.empty() || std::abs(x2 - contour.back().x()) > FLOATTYPE_EPSILON || std::abs(y2 - contour.back().y()) > FLOATTYPE_EPSILON) {
				contour.push_back({x2,y2});
			}
		}

		// Find the face edge that crosses the boundary in the reverse direction.
		FloatType v1d = v2[dim];
		for(;;) {
			edge = inputMeshData.nextFaceEdge(edge);
			FloatType v2d = reducedPos[inputMeshData.vertex2(edge)][dim];
			if(v2d - v1d <= FloatType(-0.5))
				break;
			v1d = v2d;
		}

		edge = inputMeshData.oppositeEdge(edge);
		if(edge == SurfaceMeshAccess::InvalidIndex) {
			// Mesh is not closed (not a proper manifold).
			contour.clear();
			break;
		}
	}
	while(edge != firstEdge);
	return contour;
}

/******************************************************************************
* Clips a 2d contour at a periodic boundary.
******************************************************************************/
void SurfaceMeshVis::PrepareSurfaceEngine::clipContour(std::vector<Point2>& input, std::array<bool,2> pbcFlags, std::vector<std::vector<Point2>>& openContours, std::vector<std::vector<Point2>>& closedContours)
{
	if(!pbcFlags[0] && !pbcFlags[1]) {
		closedContours.push_back(std::move(input));
		return;
	}

	// Ensure all coordinates are within the primary image.
	if(pbcFlags[0]) {
		for(auto& v : input) {
			OVITO_ASSERT(std::isfinite(v.x()));
			if(FloatType s = floor(v.x())) v.x() -= s;
		}
	}
	if(pbcFlags[1]) {
		for(auto& v : input) {
			OVITO_ASSERT(std::isfinite(v.y()));
			if(FloatType s = floor(v.y())) v.y() -= s;
		}
	}

	std::vector<std::vector<Point2>> contours;
	contours.push_back({});

	auto v1 = input.cend() - 1;
	for(auto v2 = input.cbegin(); v2 != input.cend(); v1 = v2, ++v2) {
		contours.back().push_back(*v1);

		Vector2 delta = (*v2) - (*v1);
		if(std::abs(delta.x()) < FloatType(0.5) && std::abs(delta.y()) < FloatType(0.5))
			continue;

		FloatType t[2] = { 2, 2 };
		Vector2I crossDir(0, 0);
		for(size_t dim = 0; dim < 2; dim++) {
			if(pbcFlags[dim]) {
				if(delta[dim] >= FloatType(0.5)) {
					delta[dim] -= FloatType(1);
					if(std::abs(delta[dim]) > FLOATTYPE_EPSILON)
						t[dim] = std::min((*v1)[dim] / -delta[dim], FloatType(1));
					else
						t[dim] = FloatType(0.5);
					crossDir[dim] = -1;
					OVITO_ASSERT(t[dim] >= 0 && t[dim] <= 1);
				}
				else if(delta[dim] <= FloatType(-0.5)) {
					delta[dim] += FloatType(1);
					if(std::abs(delta[dim]) > FLOATTYPE_EPSILON)
						t[dim] = std::max((FloatType(1) - (*v1)[dim]) / delta[dim], FloatType(0));
					else
						t[dim] = FloatType(0.5);
					crossDir[dim] = +1;
					OVITO_ASSERT(t[dim] >= 0 && t[dim] <= 1);
				}
			}
		}

		Point2 base = *v1;
		if(t[0] < t[1]) {
			OVITO_ASSERT(t[0] <= 1);
			computeContourIntersection(0, t[0], base, delta, crossDir[0], contours);
			if(crossDir[1] != 0) {
				OVITO_ASSERT(t[1] <= 1);
				computeContourIntersection(1, t[1], base, delta, crossDir[1], contours);
			}
		}
		else if(t[1] < t[0]) {
			OVITO_ASSERT(t[1] <= 1);
			computeContourIntersection(1, t[1], base, delta, crossDir[1], contours);
			if(crossDir[0] != 0) {
				OVITO_ASSERT(t[0] <= 1);
				computeContourIntersection(0, t[0], base, delta, crossDir[0], contours);
			}
		}
	}

	if(contours.size() == 1) {
		closedContours.push_back(std::move(contours.back()));
	}
	else {
		auto& firstSegment = contours.front();
		auto& lastSegment = contours.back();
		firstSegment.insert(firstSegment.begin(), lastSegment.begin(), lastSegment.end());
		contours.pop_back();
		for(auto& contour : contours) {
			bool isDegenerate = std::all_of(contour.begin(), contour.end(), [&contour](const Point2& p) {
				return p.equals(contour.front());
			});
			if(!isDegenerate)
				openContours.push_back(std::move(contour));
		}
	}
}

/******************************************************************************
* Computes the intersection point of a 2d contour segment crossing a
* periodic boundary.
******************************************************************************/
void SurfaceMeshVis::PrepareSurfaceEngine::computeContourIntersection(size_t dim, FloatType t, Point2& base, Vector2& delta, int crossDir, std::vector<std::vector<Point2>>& contours)
{
	OVITO_ASSERT(std::isfinite(t));
	Point2 intersection = base + t * delta;
	intersection[dim] = (crossDir == -1) ? 0 : 1;
	contours.back().push_back(intersection);
	intersection[dim] = (crossDir == +1) ? 0 : 1;
	contours.push_back({intersection});
	base = intersection;
	delta *= (FloatType(1) - t);
}

/******************************************************************************
* Determines if the 2D box corner (0,0) is inside the closed region described
* by the 2d polygon.
*
* 2D version of the algorithm:
*
* J. Andreas Baerentzen and Henrik Aanaes
* Signed Distance Computation Using the Angle Weighted Pseudonormal
* IEEE Transactions on Visualization and Computer Graphics 11 (2005), Page 243
******************************************************************************/
bool SurfaceMeshVis::PrepareSurfaceEngine::isCornerInside2DRegion(const std::vector<std::vector<Point2>>& contours)
{
	OVITO_ASSERT(!contours.empty());
	bool isInside = true;

	// Determine which vertex is closest to the test point.
	std::vector<Point2>::const_iterator closestVertex = contours.front().end();
	FloatType closestDistanceSq = FLOATTYPE_MAX;
	for(const auto& contour : contours) {
		auto v1 = contour.end() - 1;
		for(auto v2 = contour.begin(); v2 != contour.end(); v1 = v2++) {
			Vector2 r = (*v1) - Point2::Origin();
			FloatType distanceSq = r.squaredLength();
			if(distanceSq < closestDistanceSq) {
				closestDistanceSq = distanceSq;
				closestVertex = v1;

				// Compute pseuso-normal at vertex.
				auto v0 = (v1 == contour.begin()) ? (contour.end() - 1) : (v1 - 1);
				Vector2 edgeDir = (*v2) - (*v0);
				Vector2 normal(edgeDir.y(), -edgeDir.x());
				isInside = (normal.dot(r) > 0);
			}

			// Check if any edge is closer to the test point.
			Vector2 edgeDir = (*v2) - (*v1);
			FloatType edgeLength = edgeDir.length();
			if(edgeLength <= FLOATTYPE_EPSILON) continue;
			edgeDir /= edgeLength;
			FloatType d = -edgeDir.dot(r);
			if(d <= 0 || d >= edgeLength) continue;
			Vector2 c = r + edgeDir * d;
			distanceSq = c.squaredLength();
			if(distanceSq < closestDistanceSq) {
				closestDistanceSq = distanceSq;

				// Compute normal at edge.
				Vector2 normal(edgeDir.y(), -edgeDir.x());
				isInside = (normal.dot(c) > 0);
			}
		}
	}

	return isInside;
}

}	// End of namespace
