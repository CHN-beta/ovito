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
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/rendering/SceneRenderer.h>
#include <ovito/core/rendering/CylinderPrimitive.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include "BondsVis.h"
#include "ParticlesVis.h"

namespace Ovito::Particles {

IMPLEMENT_OVITO_CLASS(BondsVis);
IMPLEMENT_OVITO_CLASS(BondPickInfo);
SET_PROPERTY_FIELD_LABEL(BondsVis, bondWidth, "Bond width");
SET_PROPERTY_FIELD_LABEL(BondsVis, bondColor, "Uniform bond color");
SET_PROPERTY_FIELD_LABEL(BondsVis, shadingMode, "Shading mode");
SET_PROPERTY_FIELD_LABEL(BondsVis, renderingQuality, "Rendering quality");
SET_PROPERTY_FIELD_LABEL(BondsVis, coloringMode, "Coloring mode");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(BondsVis, bondWidth, WorldParameterUnit, 0);

/******************************************************************************
* Constructor.
******************************************************************************/
BondsVis::BondsVis(DataSet* dataset) : DataVis(dataset),
	_bondWidth(0.4),
	_bondColor(0.6, 0.6, 0.6),
	_shadingMode(NormalShading),
	_renderingQuality(CylinderPrimitive::HighQuality),
	_coloringMode(ParticleBasedColoring)
{
}

/******************************************************************************
* Computes the bounding box of the visual element.
******************************************************************************/
Box3 BondsVis::boundingBox(TimePoint time, const ConstDataObjectPath& path, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval)
{
	if(path.size() < 2) return {};
	const BondsObject* bonds = dynamic_object_cast<BondsObject>(path.back());
	const ParticlesObject* particles = dynamic_object_cast<ParticlesObject>(path[path.size()-2]);
	if(!bonds || !particles) return {};
	particles->verifyIntegrity();
	bonds->verifyIntegrity();
	const PropertyObject* bondTopologyProperty = bonds->getProperty(BondsObject::TopologyProperty);
	const PropertyObject* bondPeriodicImageProperty = bonds->getProperty(BondsObject::PeriodicImageProperty);
	const PropertyObject* positionProperty = particles->getProperty(ParticlesObject::PositionProperty);
	const SimulationCellObject* simulationCell = flowState.getObject<SimulationCellObject>();

	// The key type used for caching the computed bounding box:
	using CacheKey = RendererResourceKey<struct BondsVisBoundingBoxCache,
		ConstDataObjectRef,		// Bond topology property
		ConstDataObjectRef,		// Bond PBC vector property
		ConstDataObjectRef,		// Particle position property
		ConstDataObjectRef,		// Simulation cell
		FloatType				// Bond width
	>;

	// Look up the bounding box in the vis cache.
	auto& bbox = dataset()->visCache().get<Box3>(CacheKey(
			bondTopologyProperty,
			bondPeriodicImageProperty,
			positionProperty,
			simulationCell,
			bondWidth()));

	// Check if the cached bounding box information is still up to date.
	if(bbox.isEmpty()) {

		// If not, recompute bounding box from bond data.
		if(bondTopologyProperty && positionProperty) {

			ConstPropertyAccess<ParticleIndexPair> bondTopology(bondTopologyProperty);
			ConstPropertyAccess<Vector3I> bondPeriodicImages(bondPeriodicImageProperty);
			ConstPropertyAccess<Point3> positions(positionProperty);

			size_t particleCount = positions.size();
			const AffineTransformation cell = simulationCell ? simulationCell->cellMatrix() : AffineTransformation::Zero();

			for(size_t bondIndex = 0; bondIndex < bondTopology.size(); bondIndex++) {
				size_t index1 = bondTopology[bondIndex][0];
				size_t index2 = bondTopology[bondIndex][1];
				if(index1 >= particleCount || index2 >= particleCount)
					continue;

				bbox.addPoint(positions[index1]);
				bbox.addPoint(positions[index2]);
				if(bondPeriodicImages && bondPeriodicImages[bondIndex] != Vector3I::Zero()) {
					Vector3 vec = positions[index2] - positions[index1];
					const Vector3I& pbcShift = bondPeriodicImages[bondIndex];
					for(size_t k = 0; k < 3; k++) {
						if(pbcShift[k] != 0) vec += cell.column(k) * (FloatType)pbcShift[k];
					}
					bbox.addPoint(positions[index1] + (vec * FloatType(0.5)));
					bbox.addPoint(positions[index2] - (vec * FloatType(0.5)));
				}
			}

			bbox = bbox.padBox(bondWidth() / 2);
		}
	}
	return bbox;
}

/******************************************************************************
* Lets the visualization element render the data object.
******************************************************************************/
PipelineStatus BondsVis::render(TimePoint time, const ConstDataObjectPath& path, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode)
{
	if(renderer->isBoundingBoxPass()) {
		TimeInterval validityInterval;
		renderer->addToLocalBoundingBox(boundingBox(time, path, contextNode, flowState, validityInterval));
		return {};
	}

	if(path.size() < 2) return {};
	const BondsObject* bonds = dynamic_object_cast<BondsObject>(path.back());
	const ParticlesObject* particles = dynamic_object_cast<ParticlesObject>(path[path.size()-2]);
	if(!bonds || !particles) return {};
	particles->verifyIntegrity();
	bonds->verifyIntegrity();
	const PropertyObject* bondTopologyProperty = bonds->getProperty(BondsObject::TopologyProperty);
	const PropertyObject* bondPeriodicImageProperty = bonds->getProperty(BondsObject::PeriodicImageProperty);
	const PropertyObject* positionProperty = particles->getProperty(ParticlesObject::PositionProperty);
	const SimulationCellObject* simulationCell = flowState.getObject<SimulationCellObject>();
	const PropertyObject* bondTypeProperty = (coloringMode() == ByTypeColoring) ? bonds->getProperty(BondsObject::TypeProperty) : nullptr;
	const PropertyObject* bondColorProperty = bonds->getProperty(BondsObject::ColorProperty);
	const PropertyObject* bondSelectionProperty = renderer->isInteractive() ? bonds->getProperty(BondsObject::SelectionProperty) : nullptr;
	const PropertyObject* transparencyProperty = bonds->getProperty(BondsObject::TransparencyProperty);

	// Obtain particle-related properties and the vis element.
	const ParticlesVis* particleVis = particles->visElement<ParticlesVis>();
	const PropertyObject* particleRadiusProperty = particles->getProperty(ParticlesObject::RadiusProperty);
	const PropertyObject* particleTransparencyProperty = particles->getProperty(ParticlesObject::TransparencyProperty);
	const PropertyObject* particleColorProperty = nullptr;
	const PropertyObject* particleTypeProperty = nullptr;
	if(coloringMode() == ParticleBasedColoring && particleVis) {
		particleColorProperty = particles->getProperty(ParticlesObject::ColorProperty);
		particleTypeProperty = particleVis->getParticleTypeColorProperty(particles);
	}

	// Make sure we don't exceed our internal limits.
	if(bondTopologyProperty && bondTopologyProperty->size() * 2 > (size_t)std::numeric_limits<int>::max()) {
		throw RenderException(tr("This version of OVITO cannot render more than %1 bonds.").arg(std::numeric_limits<int>::max() / 2));
	}

	// The key type used for caching the rendering primitive:
	using CacheKey = RendererResourceKey<struct BondsVisCache,
		ConstDataObjectRef,		// Bond topology property
		ConstDataObjectRef,		// Bond PBC vector property
		ConstDataObjectRef,		// Particle position property
		ConstDataObjectRef,		// Particle color property
		ConstDataObjectRef,		// Particle type property
		ConstDataObjectRef,		// Particle radius property
		ConstDataObjectRef,		// Bond color property
		ConstDataObjectRef,		// Bond type property
		ConstDataObjectRef,		// Bond selection property
		ConstDataObjectRef,		// Bond transparency
		ConstDataObjectRef,		// Simulation cell
		FloatType,				// Bond width
		Color,					// Bond uniform color
		ColoringMode,			// Bond coloring mode
		ShadingMode,			// Bond shading mode
		CylinderPrimitive::RenderingQuality // Bond rendering quality
	>;

	// The data structure stored in the vis cache.
	struct CacheValue {
		CylinderPrimitive cylinders;
		ParticlePrimitive vertices;
	};

	// Lookup the rendering primitive in the vis cache.
	auto& visCache = dataset()->visCache().get<CacheValue>(CacheKey(
			bondTopologyProperty,
			bondPeriodicImageProperty,
			positionProperty,
			particleColorProperty,
			particleTypeProperty,
			particleRadiusProperty,
			bondColorProperty,
			bondTypeProperty,
			bondSelectionProperty,
			transparencyProperty,
			simulationCell,
			bondWidth(),
			bondColor(),
			coloringMode(),
			shadingMode(),
			renderingQuality()));

	// Make sure the primitive for the nodal vertices gets created if particles display is turned off or if particles are semi-transparent.
	bool renderNodalVertices = !transparencyProperty && (!particleVis || particleVis->isEnabled() == false || particleTransparencyProperty != nullptr);
	if(renderNodalVertices && !visCache.vertices.positions())
		visCache.cylinders.setPositions(nullptr, nullptr);

	// Check if we already have a valid rendering primitive that is up to date.
	if(!visCache.cylinders.basePositions()) {

		FloatType bondRadius = bondWidth() / 2;
		if(bondTopologyProperty && positionProperty && bondRadius > 0) {

			// Allocate buffers for the bonds geometry.
			DataBufferAccessAndRef<Point3> bondPositions1 = DataBufferPtr::create(dataset(), bondTopologyProperty->size() * 2, DataBuffer::Float, 3, 0, false);
			DataBufferAccessAndRef<Point3> bondPositions2 = DataBufferPtr::create(dataset(), bondTopologyProperty->size() * 2, DataBuffer::Float, 3, 0, false);
			DataBufferAccessAndRef<Color> bondColors = DataBufferPtr::create(dataset(), bondTopologyProperty->size() * 2, DataBuffer::Float, 3, 0, false);
			DataBufferAccessAndRef<FloatType> bondTransparencies = transparencyProperty ? DataBufferPtr::create(dataset(), bondTopologyProperty->size() * 2, DataBuffer::Float, 1, 0, false) : nullptr;

			// Allocate buffers for the nodal vertices.
			DataBufferAccessAndRef<Color> nodalColors = renderNodalVertices ? DataBufferPtr::create(dataset(), positionProperty->size(), DataBuffer::Float, 3, 0, false) : nullptr;
			DataBufferAccessAndRef<FloatType> nodalTransparencies = (renderNodalVertices && transparencyProperty) ? DataBufferPtr::create(dataset(), positionProperty->size(), DataBuffer::Float, 1, 0, false) : nullptr;
			DataBufferAccessAndRef<int> nodalIndices = renderNodalVertices ? DataBufferPtr::create(dataset(), 0, DataBuffer::Int, 1, 0, false) : nullptr;
			boost::dynamic_bitset<> visitedParticles(renderNodalVertices ? positionProperty->size() : 0);
			OVITO_ASSERT(nodalColors || !nodalTransparencies);

			// Cache some values.
			ConstPropertyAccess<Point3> positions(positionProperty);
			size_t particleCount = positions.size();
			const AffineTransformation cell = simulationCell ? simulationCell->cellMatrix() : AffineTransformation::Zero();

			// Obtain the radii of the particles.
			ConstPropertyAccessAndRef<FloatType> particleRadii;
			if(particleVis)
				particleRadii = particleVis->particleRadii(particles, false);
			// Make sure the particle radius array has the correct length.
			if(particleRadii && particleRadii.size() != particleCount) 
				particleRadii.reset();

			// Determine half-bond colors.
			std::vector<Color> colors = halfBondColors(particles, renderer->isInteractive(), coloringMode(), false);
			OVITO_ASSERT(colors.size() == bondPositions1.size());

			size_t cylinderIndex = 0;
			auto color = colors.cbegin();
			ConstPropertyAccess<ParticleIndexPair> bonds(bondTopologyProperty);
			ConstPropertyAccess<Vector3I> bondPeriodicImages(bondPeriodicImageProperty);
			ConstPropertyAccess<FloatType> bondInputTransparency(transparencyProperty);
			for(size_t bondIndex = 0; bondIndex < bonds.size(); bondIndex++) {
				size_t particleIndex1 = bonds[bondIndex][0];
				size_t particleIndex2 = bonds[bondIndex][1];
				if(particleIndex1 < particleCount && particleIndex2 < particleCount) {
					Vector3 vec = positions[particleIndex2] - positions[particleIndex1];
					bool isSplitBond = false;
					if(bondPeriodicImageProperty) {
						for(size_t k = 0; k < 3; k++) {
							if(int d = bondPeriodicImages[bondIndex][k]) {
								vec += cell.column(k) * (FloatType)d;
								isSplitBond = true;
							}
						}
					}
					FloatType t = 0.5;
					FloatType blen = vec.length() * FloatType(2);
					if(particleRadii && blen != 0) {
						// This calculation determines the point where to split the bond into the two half-bonds
						// such that the border appears halfway between the two particles, which may have two different sizes.
						t = FloatType(0.5) + std::min(FloatType(0.5), particleRadii[particleIndex1]/blen) - std::min(FloatType(0.5), particleRadii[particleIndex2]/blen);
					}
					bondColors[cylinderIndex] = *color++;
					if(nodalColors && !visitedParticles.test(particleIndex1)) {
						nodalColors[particleIndex1] = bondColors[cylinderIndex];
						if(nodalTransparencies)
							nodalTransparencies[particleIndex1] = bondInputTransparency[bondIndex];
						visitedParticles.set(particleIndex1);
						nodalIndices.push_back(particleIndex1);
					}
					if(bondTransparencies) bondTransparencies[cylinderIndex] = bondInputTransparency[bondIndex];
					bondPositions1[cylinderIndex] = positions[particleIndex1];
					bondPositions2[cylinderIndex] = positions[particleIndex1] + vec * t;
					if(isSplitBond)
						swap(bondPositions1[cylinderIndex], bondPositions2[cylinderIndex]);
					cylinderIndex++;

					bondColors[cylinderIndex] = *color++;
					if(nodalColors && !visitedParticles.test(particleIndex2)) {
						nodalColors[particleIndex2] = bondColors[cylinderIndex];
						if(nodalTransparencies)
							nodalTransparencies[particleIndex2] = bondInputTransparency[bondIndex];
						visitedParticles.set(particleIndex2);
						nodalIndices.push_back(particleIndex2);
					}
					if(bondTransparencies) bondTransparencies[cylinderIndex] = bondInputTransparency[bondIndex];
					bondPositions1[cylinderIndex] = positions[particleIndex2];
					bondPositions2[cylinderIndex] = positions[particleIndex2] - vec * (FloatType(1) - t);
					if(isSplitBond)
						swap(bondPositions1[cylinderIndex], bondPositions2[cylinderIndex]);
					cylinderIndex++;
				}
				else {
					bondColors[cylinderIndex] = *color++;
					if(bondTransparencies) bondTransparencies[cylinderIndex] = 0;
					bondPositions1[cylinderIndex] = Point3::Origin();
					bondPositions2[cylinderIndex++] = Point3::Origin();

					bondColors[cylinderIndex] = *color++;
					if(bondTransparencies) bondTransparencies[cylinderIndex] = 0;
					bondPositions1[cylinderIndex] = Point3::Origin();
					bondPositions2[cylinderIndex++] = Point3::Origin();
				}
			}

			visCache.cylinders.setShape(CylinderPrimitive::CylinderShape);
			visCache.cylinders.setShadingMode(static_cast<CylinderPrimitive::ShadingMode>(shadingMode()));
			visCache.cylinders.setRenderingQuality(renderingQuality());
			visCache.cylinders.setRenderSingleCylinderCap(transparencyProperty != nullptr);
			visCache.cylinders.setUniformRadius(bondRadius);
			visCache.cylinders.setPositions(bondPositions1.take(), bondPositions2.take());
			visCache.cylinders.setColors(bondColors.take());
			visCache.cylinders.setTransparencies(bondTransparencies.take());

			if(renderNodalVertices) {
				OVITO_ASSERT(positionProperty);
				visCache.vertices.setParticleShape(ParticlePrimitive::SphericalShape);
				visCache.vertices.setShadingMode((shadingMode() == NormalShading) ? ParticlePrimitive::NormalShading : ParticlePrimitive::FlatShading);
				visCache.vertices.setRenderingQuality(ParticlePrimitive::HighQuality);
				visCache.vertices.setPositions(positionProperty);
				visCache.vertices.setUniformRadius(bondRadius);
				visCache.vertices.setColors(nodalColors.take());
				visCache.vertices.setIndices(nodalIndices.take());
				visCache.vertices.setTransparencies(nodalTransparencies.take());
			}
		}
	}
	if(!visCache.cylinders.basePositions())
		return {};

	if(renderer->isPicking()) {
		OORef<BondPickInfo> pickInfo(new BondPickInfo(particles, simulationCell));
		renderer->beginPickObject(contextNode, pickInfo);
	}
	renderer->renderCylinders(visCache.cylinders);
	if(renderer->isPicking()) {
		renderer->endPickObject();
	}

	if(visCache.vertices.positions() && renderNodalVertices) {
		if(renderer->isPicking())
			renderer->beginPickObject(contextNode);
		renderer->renderParticles(visCache.vertices);
		if(renderer->isPicking())
			renderer->endPickObject();
	}

	return {};
}

/******************************************************************************
* Determines the display colors of half-bonds.
* Returns an array with two colors per full bond, because the two half-bonds
* may have different colors.
******************************************************************************/
std::vector<Color> BondsVis::halfBondColors(const ParticlesObject* particles, bool highlightSelection, ColoringMode coloringMode, bool ignoreBondColorProperty) const
{
	OVITO_ASSERT(particles != nullptr);
	particles->verifyIntegrity();
	const BondsObject* bonds = particles->bonds();
	if(!bonds) return {};
	bonds->verifyIntegrity();

	// Get bond-related properties which determine the bond coloring.
	ConstPropertyAccess<ParticleIndexPair> topologyProperty = bonds->getProperty(BondsObject::TopologyProperty);
	ConstPropertyAccess<Color> bondColorProperty = !ignoreBondColorProperty ? bonds->getProperty(BondsObject::ColorProperty) : nullptr;
	const PropertyObject* bondTypeProperty = (coloringMode == ByTypeColoring) ? bonds->getProperty(BondsObject::TypeProperty) : nullptr;
	ConstPropertyAccess<int> bondSelectionProperty = highlightSelection ? bonds->getProperty(BondsObject::SelectionProperty) : nullptr;

	// Get particle-related properties and the vis element.
	const ParticlesVis* particleVis = particles->visElement<ParticlesVis>();
	ConstPropertyAccess<Color> particleColorProperty;
	const PropertyObject* particleTypeProperty = nullptr;
	if(coloringMode == ParticleBasedColoring && particleVis) {
		particleColorProperty = particles->getProperty(ParticlesObject::ColorProperty);
		particleTypeProperty = particleVis->getParticleTypeColorProperty(particles);
	}

	std::vector<Color> output(bonds->elementCount() * 2);
	Color defaultColor = bondColor();
	if(bondColorProperty && bondColorProperty.size() * 2 == output.size()) {
		// Take bond colors directly from the color property.
		auto bc = output.begin();
		for(const Color& c : bondColorProperty) {
			*bc++ = c;
			*bc++ = c;
		}
	}
	else if(coloringMode == ParticleBasedColoring && particleVis) {
		// Derive bond colors from particle colors.
		size_t particleCount = particles->elementCount();
		ConstPropertyAccessAndRef<Color> particleColors = particleVis->particleColors(particles, false);
		OVITO_ASSERT(particleColors.size() == particleCount);
		auto bc = output.begin();
		for(const auto& bond : topologyProperty) {
			if((size_t)bond[0] < particleCount && (size_t)bond[1] < particleCount) {
				*bc++ = particleColors[bond[0]];
				*bc++ = particleColors[bond[1]];
			}
			else {
				*bc++ = defaultColor;
				*bc++ = defaultColor;
			}
		}
	}
	else {
		if(bondTypeProperty && bondTypeProperty->size() * 2 == output.size()) {
			// Assign colors based on bond types.
			// Generate a lookup map for bond type colors.
			const std::map<int, Color>& colorMap = bondTypeProperty->typeColorMap();
			std::array<Color,16> colorArray;
			// Check if all type IDs are within a small, non-negative range.
			// If yes, we can use an array lookup strategy. Otherwise we have to use a dictionary lookup strategy, which is slower.
			if(boost::algorithm::all_of(colorMap,
					[&colorArray](const std::map<int, Color>::value_type& i) { return i.first >= 0 && i.first < (int)colorArray.size(); })) {
				colorArray.fill(defaultColor);
				for(const auto& entry : colorMap)
					colorArray[entry.first] = entry.second;
				// Fill color array.
				ConstPropertyAccess<int> bondTypeData(bondTypeProperty);
				const int* t = bondTypeData.cbegin();
				for(auto c = output.begin(); c != output.end(); ++t) {
					if(*t >= 0 && *t < (int)colorArray.size()) {
						*c++ = colorArray[*t];
						*c++ = colorArray[*t];
					}
					else {
						*c++ = defaultColor;
						*c++ = defaultColor;
					}
				}
			}
			else {
				// Fill color array.
				ConstPropertyAccess<int> bondTypeData(bondTypeProperty);
				const int* t = bondTypeData.cbegin();
				for(auto c = output.begin(); c != output.end(); ++t) {
					if(auto it = colorMap.find(*t); it != colorMap.end()) {
						*c++ = it->second;
						*c++ = it->second;
					}
					else {
						*c++ = defaultColor;
						*c++ = defaultColor;
					}
				}
			}
		}
		else {
			// Assign a uniform color to all bonds.
			boost::fill(output, defaultColor);
		}
	}

	// Highlight selected bonds.
	if(bondSelectionProperty && bondSelectionProperty.size() * 2 == output.size()) {
		const Color selColor = selectionBondColor();
		const int* t = bondSelectionProperty.cbegin();
		for(auto c = output.begin(); c != output.end(); ++t) {
			if(*t) {
				*c++ = selColor;
				*c++ = selColor;
			}
			else c += 2;
		}
	}

	return output;
}

/******************************************************************************
* Returns a human-readable string describing the picked object,
* which will be displayed in the status bar by OVITO.
******************************************************************************/
QString BondPickInfo::infoString(PipelineSceneNode* objectNode, quint32 subobjectId)
{
	QString str;
	size_t bondIndex = subobjectId / 2;
	if(particles()->bonds()) {
		ConstPropertyAccess<ParticleIndexPair> topologyProperty = particles()->bonds()->getTopology();
		if(topologyProperty && topologyProperty.size() > bondIndex) {
			size_t index1 = topologyProperty[bondIndex][0];
			size_t index2 = topologyProperty[bondIndex][1];
			str = tr("Bond: ");

			// Bond length
			ConstPropertyAccess<Point3> posProperty = particles()->getProperty(ParticlesObject::PositionProperty);
			if(posProperty && posProperty.size() > index1 && posProperty.size() > index2) {
				const Point3& p1 = posProperty[index1];
				const Point3& p2 = posProperty[index2];
				Vector3 delta = p2 - p1;
				if(ConstPropertyAccess<Vector3I> periodicImageProperty = particles()->bonds()->getProperty(BondsObject::PeriodicImageProperty)) {
					if(simulationCell()) {
						delta += simulationCell()->cellMatrix() * periodicImageProperty[bondIndex].toDataType<FloatType>();
					}
				}
				str += QString("<key>Length:</key> <val>%1</val><sep><key>Delta:</key> <val>%2, %3, %4</val>").arg(delta.length()).arg(delta.x()).arg(delta.y()).arg(delta.z());
			}

			// Bond properties
			for(const PropertyObject* property : particles()->bonds()->properties()) {
				if(property->size() <= bondIndex) continue;
				if(property->type() == BondsObject::SelectionProperty) continue;
				if(property->type() == BondsObject::ColorProperty) continue;
				if(!str.isEmpty()) str += QStringLiteral("<sep>");
				str += QStringLiteral("<key>");
				str += property->name().toHtmlEscaped();
				str += QStringLiteral(":</key> <val>");
				if(property->dataType() == PropertyObject::Int) {
					ConstPropertyAccess<int, true> data(property);
					for(size_t component = 0; component < data.componentCount(); component++) {
						if(component != 0) str += QStringLiteral(", ");
						str += QString::number(data.get(bondIndex, component));
						if(property->elementTypes().empty() == false) {
							if(const ElementType* ptype = property->elementType(data.get(bondIndex, component))) {
								if(!ptype->name().isEmpty())
									str += QString(" (%1)").arg(ptype->name().toHtmlEscaped());
							}
						}
					}
				}
				else if(property->dataType() == PropertyObject::Int64) {
					ConstPropertyAccess<qlonglong, true> data(property);
					for(size_t component = 0; component < property->componentCount(); component++) {
						if(component != 0) str += QStringLiteral(", ");
						str += QString::number(data.get(bondIndex, component));
					}
				}
				else if(property->dataType() == PropertyObject::Float) {
					ConstPropertyAccess<FloatType, true> data(property);
					for(size_t component = 0; component < property->componentCount(); component++) {
						if(component != 0) str += QStringLiteral(", ");
						str += QString::number(data.get(bondIndex, component));
					}
				}
				else {
					str += QStringLiteral("<%1>").arg(getQtTypeNameFromId(property->dataType()) ? getQtTypeNameFromId(property->dataType()) : "unknown");
				}
				str += QStringLiteral("</val>");
			}

			// Pair type info.
			const PropertyObject* typeProperty = particles()->getProperty(ParticlesObject::TypeProperty);
			if(typeProperty && typeProperty->size() > index1 && typeProperty->size() > index2) {
				ConstPropertyAccess<int> typeData(typeProperty);
				const ElementType* type1 = typeProperty->elementType(typeData[index1]);
				const ElementType* type2 = typeProperty->elementType(typeData[index2]);
				if(type1 && type2) {
					str += QString("<sep><key>Particles:</key> <val>%1 - %2</val>").arg(type1->nameOrNumericId(), type2->nameOrNumericId());
				}
			}
		}
	}
	return str;
}

/******************************************************************************
* Allows the object to parse the serialized contents of a property field in a custom way.
******************************************************************************/
bool BondsVis::loadPropertyFieldFromStream(ObjectLoadStream& stream, const RefMakerClass::SerializedClassInfo::PropertyFieldInfo& serializedField)
{
	// For backward compatibility with OVITO 3.5.4:
	// Parse the "useParticleColors" field, which has been replaced by the "coloringMode" parameter field in later versions.
	if(serializedField.definingClass == &BondsVis::OOClass() && serializedField.identifier == "useParticleColors") {
		bool useParticleColors;
		stream >> useParticleColors;
		setColoringMode(useParticleColors ? ParticleBasedColoring : ByTypeColoring);
		return true;
	}
	return DataVis::loadPropertyFieldFromStream(stream, serializedField);
}

}	// End of namespace
