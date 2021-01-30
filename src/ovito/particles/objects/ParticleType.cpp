////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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
#include <ovito/core/dataset/io/FileSource.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include "ParticleType.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(ParticleType);
DEFINE_PROPERTY_FIELD(ParticleType, radius);
DEFINE_PROPERTY_FIELD(ParticleType, vdwRadius);
DEFINE_PROPERTY_FIELD(ParticleType, shape);
DEFINE_REFERENCE_FIELD(ParticleType, shapeMesh);
DEFINE_PROPERTY_FIELD(ParticleType, highlightShapeEdges);
DEFINE_PROPERTY_FIELD(ParticleType, shapeBackfaceCullingEnabled);
DEFINE_PROPERTY_FIELD(ParticleType, shapeUseMeshColor);
DEFINE_PROPERTY_FIELD(ParticleType, mass);
SET_PROPERTY_FIELD_LABEL(ParticleType, radius, "Display radius");
SET_PROPERTY_FIELD_LABEL(ParticleType, vdwRadius, "Van der Waals radius");
SET_PROPERTY_FIELD_LABEL(ParticleType, shape, "Shape");
SET_PROPERTY_FIELD_LABEL(ParticleType, shapeMesh, "Shape Mesh");
SET_PROPERTY_FIELD_LABEL(ParticleType, highlightShapeEdges, "Highlight edges");
SET_PROPERTY_FIELD_LABEL(ParticleType, shapeBackfaceCullingEnabled, "Back-face culling");
SET_PROPERTY_FIELD_LABEL(ParticleType, shapeUseMeshColor, "Use mesh color");
SET_PROPERTY_FIELD_LABEL(ParticleType, mass, "Mass");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ParticleType, radius, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ParticleType, vdwRadius, WorldParameterUnit, 0);

/******************************************************************************
* Constructs a new particle type.
******************************************************************************/
ParticleType::ParticleType(DataSet* dataset) : ElementType(dataset),
	_radius(0),
	_vdwRadius(0),
	_shape(ParticlesVis::ParticleShape::Default),
	_highlightShapeEdges(false),
	_shapeBackfaceCullingEnabled(true),
	_shapeUseMeshColor(false),
	_mass(0)
{
}

/******************************************************************************
* Initializes the particle type's attributes to standard values.
******************************************************************************/
void ParticleType::initializeType(const PropertyReference& property, ExecutionContext executionContext)
{
	ElementType::initializeType(property, executionContext);

	// Load standard display radius.
	setRadius(getDefaultParticleRadius(static_cast<ParticlesObject::Type>(property.type()), nameOrNumericId(), numericId(), executionContext, DisplayRadius));
	// Load standard van der Waals radius.
	setVdwRadius(getDefaultParticleRadius(static_cast<ParticlesObject::Type>(property.type()), nameOrNumericId(), numericId(), executionContext, VanDerWaalsRadius));
}

/******************************************************************************
* Creates an editable proxy object for this DataObject and synchronizes its parameters.
******************************************************************************/
void ParticleType::updateEditableProxies(PipelineFlowState& state, ConstDataObjectPath& dataPath) const
{
	ElementType::updateEditableProxies(state, dataPath);

	// Note: 'this' may no longer exist at this point, because the base method implementationmay
	// have already replaced it with a mutable copy.
	const ParticleType* self = static_object_cast<ParticleType>(dataPath.back());

	if(ParticleType* proxy = static_object_cast<ParticleType>(self->editableProxy())) {

		// This allows the GSD file importer to update the generated shape mesh as long as the user didn't replace the mesh with a custom one.
		if(self->shapeMesh() && self->shapeMesh()->identifier() == QStringLiteral("generated") && proxy->shapeMesh() && proxy->shapeMesh()->identifier() == QStringLiteral("generated")) {
			proxy->setShapeMesh(self->shapeMesh());
		}
	
		// Copy properties changed by the user over to the data object.
		if(proxy->radius() != self->radius() || proxy->mass() != self->mass() || proxy->shape() != self->shape() || proxy->shapeMesh() != self->shapeMesh() || proxy->highlightShapeEdges() != self->highlightShapeEdges() 
				|| proxy->shapeBackfaceCullingEnabled() != self->shapeBackfaceCullingEnabled() || proxy->shapeUseMeshColor() != self->shapeUseMeshColor()) {
			// Make this data object mutable first.
			ParticleType* mutableSelf = static_object_cast<ParticleType>(state.makeMutableInplace(dataPath));
			mutableSelf->setRadius(proxy->radius());
			mutableSelf->setMass(proxy->mass());
			mutableSelf->setShape(proxy->shape());
			mutableSelf->setShapeMesh(proxy->shapeMesh());
			mutableSelf->setHighlightShapeEdges(proxy->highlightShapeEdges());
			mutableSelf->setShapeBackfaceCullingEnabled(proxy->shapeBackfaceCullingEnabled());
			mutableSelf->setShapeUseMeshColor(proxy->shapeUseMeshColor());
		}
	}
}

/******************************************************************************
 * Loads a user-defined display shape from a geometry file and assigns it to this particle type.
 ******************************************************************************/
bool ParticleType::loadShapeMesh(const QUrl& sourceUrl, Promise<>&& operation, ExecutionContext executionContext, const FileImporterClass* importerType)
{
    operation.setProgressText(tr("Loading mesh geometry file %1").arg(sourceUrl.fileName()));

	// Temporarily disable undo recording while loading the geometry data.
	UndoSuspender noUndo(this);

	OORef<FileSourceImporter> importer;
	if(!importerType) {

		// Inspect input file to detect its format.
		Future<OORef<FileImporter>> importerFuture = FileImporter::autodetectFileFormat(dataset(), executionContext, sourceUrl);
		if(!operation.waitForFuture(importerFuture))
			return false;

		importer = dynamic_object_cast<FileSourceImporter>(importerFuture.result());
	}
	else {
		importer = dynamic_object_cast<FileSourceImporter>(importerType->createInstance(dataset(), executionContext));
	}
	if(!importer)
		throwException(tr("Could not detect the format of the geometry file. The format might not be supported."));

	// Create a temporary FileSource for loading the geometry data from the file.
	OORef<FileSource> fileSource = OORef<FileSource>::create(dataset(), executionContext);
	fileSource->setSource({sourceUrl}, importer, false);
	SharedFuture<PipelineFlowState> stateFuture = fileSource->evaluate(PipelineEvaluationRequest(0));
	if(!operation.waitForFuture(stateFuture))
		return false;

	// Check if the FileSource has provided some useful data.
	PipelineFlowState state = stateFuture.result();
	if(state.status().type() == PipelineStatus::Error) {
		operation.cancel();
		return false;
	}
	if(!state)
		throwException(tr("The loaded geometry file does not provide any valid mesh data."));
	TriMeshObject* meshObj = state.expectMutableObject<TriMeshObject>();
	if(!meshObj->mesh())
		throwException(tr("The loaded geometry file does not contain a valid mesh."));

	// Show sharp edges of the mesh.
	meshObj->modifiableMesh()->determineEdgeVisibility();

	// Turn on undo recording again. The final shape assignment should be recorded on the undo stack.
	noUndo.reset();
	setShapeMesh(meshObj);

	// Also switch the particle type's visualization shape to mesh-based.
	setShape(ParticlesVis::Mesh);

    return !operation.isCanceled();
}

/******************************************************************************
* Is called once for this object after it has been completely loaded from a stream.
******************************************************************************/
void ParticleType::loadFromStreamComplete(ObjectLoadStream& stream)
{
	ElementType::loadFromStreamComplete(stream);

	// For backward compatibility with OVITO 3.3.5: 
	// The 'shape' parameter field of the ParticleType class does not exist yet in state files written by older program versions.
	// Automatically switch the type's shape to 'Mesh' if a mesh geometry has been assigned to the type. 
	if(stream.formatVersion() < 30007) {
		if(shape() == ParticlesVis::ParticleShape::Default && shapeMesh())
			setShape(ParticlesVis::ParticleShape::Mesh);
	}
}

// Define default names, colors, and radii for some predefined particle types.
//
// Van der Waals radii have been adopted from the VMD software, which adopted them from A. Bondi, J. Phys. Chem., 68, 441 - 452, 1964,
// except the value for H, which was taken from R.S. Rowland & R. Taylor, J. Phys. Chem., 100, 7384 - 7391, 1996. 
// Radii that are not available in either of these publications use r = 2.0.
// The radii for ions (Na, K, Cl, Ca, Mg, and Cs) are based on the CHARMM27 Rmin/2 parameters for (SOD, POT, CLA, CAL, MG, CES).
const std::array<ParticleType::PredefinedChemicalType, ParticleType::NUMBER_OF_PREDEFINED_PARTICLE_TYPES> ParticleType::_predefinedParticleTypes{{
	ParticleType::PredefinedChemicalType{ QStringLiteral("H"), Color(255.0f/255.0f, 255.0f/255.0f, 255.0f/255.0f), 0.46f, 1.20f, },
	ParticleType::PredefinedChemicalType{ QStringLiteral("He"), Color(217.0f/255.0f, 255.0f/255.0f, 255.0f/255.0f), 1.22f, 1.40f },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Li"), Color(204.0f/255.0f, 128.0f/255.0f, 255.0f/255.0f), 1.57f, 1.82f },
	ParticleType::PredefinedChemicalType{ QStringLiteral("C"), Color(144.0f/255.0f, 144.0f/255.0f, 144.0f/255.0f), 0.77f, 1.70f },
	ParticleType::PredefinedChemicalType{ QStringLiteral("N"), Color(48.0f/255.0f, 80.0f/255.0f, 248.0f/255.0f), 0.74f, 1.55f  },
	ParticleType::PredefinedChemicalType{ QStringLiteral("O"), Color(255.0f/255.0f, 13.0f/255.0f, 13.0f/255.0f), 0.74f, 1.52f },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Na"), Color(171.0f/255.0f, 92.0f/255.0f, 242.0f/255.0f), 1.91f, 1.36f },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Mg"), Color(138.0f/255.0f, 255.0f/255.0f, 0.0f/255.0f), 1.60f, 1.18f },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Al"), Color(191.0f/255.0f, 166.0f/255.0f, 166.0f/255.0f), 1.43f, 2.00f },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Si"), Color(240.0f/255.0f, 200.0f/255.0f, 160.0f/255.0f), 1.18f, 2.10f },
	ParticleType::PredefinedChemicalType{ QStringLiteral("K"), Color(143.0f/255.0f, 64.0f/255.0f, 212.0f/255.0f), 2.35f, 1.76f },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Ca"), Color(61.0f/255.0f, 255.0f/255.0f, 0.0f/255.0f), 1.97f, 1.37f },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Ti"), Color(191.0f/255.0f, 194.0f/255.0f, 199.0f/255.0f), 1.47f, 2.00f },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Cr"), Color(138.0f/255.0f, 153.0f/255.0f, 199.0f/255.0f), 1.29f, 2.00f },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Fe"), Color(224.0f/255.0f, 102.0f/255.0f, 51.0f/255.0f), 1.26f, 2.00f },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Co"), Color(240.0f/255.0f, 144.0f/255.0f, 160.0f/255.0f), 1.25f, 2.00f },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Ni"), Color(80.0f/255.0f, 208.0f/255.0f, 80.0f/255.0f), 1.25f, 1.63f },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Cu"), Color(200.0f/255.0f, 128.0f/255.0f, 51.0f/255.0f), 1.28f, 1.40f },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Zn"), Color(125.0f/255.0f, 128.0f/255.0f, 176.0f/255.0f), 1.37f, 1.39f },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Ga"), Color(194.0f/255.0f, 143.0f/255.0f, 143.0f/255.0f), 1.53f, 1.07f },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Ge"), Color(102.0f/255.0f, 143.0f/255.0f, 143.0f/255.0f), 1.22f, 2.00f },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Kr"), Color(92.0f/255.0f, 184.0f/255.0f, 209.0f/255.0f), 1.98f, 2.02f },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Sr"), Color(0.0f, 1.0f, 0.15259f), 2.15f, 2.00f },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Y"), Color(0.40259f, 0.59739f, 0.55813f), 1.82f, 2.00f },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Zr"), Color(0.0f, 1.0f, 0.0f), 1.60f, 2.00f },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Nb"), Color(0.29992f, 0.7f, 0.46459f), 1.47f, 2.00f },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Pd"), Color(0.0f/255.0f, 105.0f/255.0f, 133.0f/255.0f), 1.37f, 1.63f },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Pt"), Color(0.79997f, 0.77511f, 0.75068f), 1.39f, 1.72f },
	ParticleType::PredefinedChemicalType{ QStringLiteral("W"), Color(0.55616f, 0.54257f, 0.50178f), 1.41f, 2.00f },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Au"), Color(255.0f/255.0f, 209.0f/255.0f, 35.0f/255.0f), 1.44f, 1.66f },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Pb"), Color(87.0f/255.0f, 89.0f/255.0f, 97.0f/255.0f), 1.47f, 2.02f },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Bi"), Color(158.0f/255.0f, 79.0f/255.0f, 181.0f/255.0f), 1.46f, 2.00f }
}};

// Define default names, colors, and radii for predefined structure types.
const std::array<ParticleType::PredefinedStructuralType, ParticleType::NUMBER_OF_PREDEFINED_STRUCTURE_TYPES> ParticleType::_predefinedStructureTypes{{
	ParticleType::PredefinedStructuralType{ QStringLiteral("Other"), Color(0.95f, 0.95f, 0.95f) },
	ParticleType::PredefinedStructuralType{ QStringLiteral("FCC"), Color(0.4f, 1.0f, 0.4f) },
	ParticleType::PredefinedStructuralType{ QStringLiteral("HCP"), Color(1.0f, 0.4f, 0.4f) },
	ParticleType::PredefinedStructuralType{ QStringLiteral("BCC"), Color(0.4f, 0.4f, 1.0f) },
	ParticleType::PredefinedStructuralType{ QStringLiteral("ICO"), Color(0.95f, 0.8f, 0.2f) },
	ParticleType::PredefinedStructuralType{ QStringLiteral("Cubic diamond"), Color(19.0f/255.0f, 160.0f/255.0f, 254.0f/255.0f) },
	ParticleType::PredefinedStructuralType{ QStringLiteral("Cubic diamond (1st neighbor)"), Color(0.0f/255.0f, 254.0f/255.0f, 245.0f/255.0f) },
	ParticleType::PredefinedStructuralType{ QStringLiteral("Cubic diamond (2nd neighbor)"), Color(126.0f/255.0f, 254.0f/255.0f, 181.0f/255.0f) },
	ParticleType::PredefinedStructuralType{ QStringLiteral("Hexagonal diamond"), Color(254.0f/255.0f, 137.0f/255.0f, 0.0f/255.0f) },
	ParticleType::PredefinedStructuralType{ QStringLiteral("Hexagonal diamond (1st neighbor)"), Color(254.0f/255.0f, 220.0f/255.0f, 0.0f/255.0f) },
	ParticleType::PredefinedStructuralType{ QStringLiteral("Hexagonal diamond (2nd neighbor)"), Color(204.0f/255.0f, 229.0f/255.0f, 81.0f/255.0f) },
	ParticleType::PredefinedStructuralType{ QStringLiteral("Simple cubic"), Color(160.0f/255.0f, 20.0f/255.0f, 254.0f/255.0f) },
	ParticleType::PredefinedStructuralType{ QStringLiteral("Graphene"), Color(160.0f/255.0f, 120.0f/255.0f, 254.0f/255.0f) },
	ParticleType::PredefinedStructuralType{ QStringLiteral("Hexagonal ice"), Color(0.0f, 0.9f, 0.9f) },
	ParticleType::PredefinedStructuralType{ QStringLiteral("Cubic ice"), Color(1.0f, 193.0f/255.0f, 5.0f/255.0f) },
	ParticleType::PredefinedStructuralType{ QStringLiteral("Interfacial ice"), Color(0.5f, 0.12f, 0.4f) },
	ParticleType::PredefinedStructuralType{ QStringLiteral("Hydrate"), Color(1.0f, 0.3f, 0.1f) },
	ParticleType::PredefinedStructuralType{ QStringLiteral("Interfacial hydrate"), Color(0.1f, 1.0f, 0.1f) },
}};

/******************************************************************************
* Returns the default radius for a particle type.
******************************************************************************/
FloatType ParticleType::getDefaultParticleRadius(ParticlesObject::Type typeClass, const QString& particleTypeName, int numericTypeId, ExecutionContext executionContext, RadiusVariant radiusVariant)
{
	// Interactive execution context means that we are supposed to load the user-defined
	// settings from the settings store.
	if(executionContext == ExecutionContext::Interactive && typeClass != ParticlesObject::UserProperty) {

		// Use the type's name, property type and container class to look up the 
		// default radius saved by the user.
		const QString& settingsKey = ElementType::getElementSettingsKey(ParticlePropertyReference(typeClass), 
			(radiusVariant == DisplayRadius) ? QStringLiteral("radius") : QStringLiteral("vdw_radius"), particleTypeName);
		QVariant v = QSettings().value(settingsKey);
		if(v.isValid() && v.canConvert<FloatType>())
			return v.value<FloatType>();

		// The following is for backward compatibility with OVITO 3.3.5, which used to store the 
		// default radii in a different branch of the settings registry.
		if(radiusVariant == DisplayRadius) {
			v = QSettings().value(QStringLiteral("particles/defaults/radius/%1/%2").arg(typeClass).arg(particleTypeName));
			if(v.isValid() && v.canConvert<FloatType>())
				return v.value<FloatType>();
		}
	}

	if(typeClass == ParticlesObject::TypeProperty) {
		for(const PredefinedChemicalType& predefType : _predefinedParticleTypes) {
			if(predefType.name == particleTypeName) {
				if(radiusVariant == DisplayRadius)
					return predefType.displayRadius;
				else
					return predefType.vdwRadius;
			}
		}

		// Sometimes atom type names have additional letters/numbers appended.
		if(particleTypeName.length() > 1 && particleTypeName.length() <= 5) {
			return getDefaultParticleRadius(typeClass, particleTypeName.left(particleTypeName.length() - 1), numericTypeId, executionContext, radiusVariant);
		}
	}

	return 0;
}

/******************************************************************************
* Changes the default radius for a particle type.
******************************************************************************/
void ParticleType::setDefaultParticleRadius(ParticlesObject::Type typeClass, const QString& particleTypeName, FloatType radius, RadiusVariant radiusVariant)
{
	if(typeClass == ParticlesObject::UserProperty)
		return;

	QSettings settings;
	const QString& settingsKey = ElementType::getElementSettingsKey(ParticlePropertyReference(typeClass), 
		(radiusVariant == DisplayRadius) ? QStringLiteral("radius") : QStringLiteral("vdw_radius"), particleTypeName);
	
	if(std::abs(getDefaultParticleRadius(typeClass, particleTypeName, 0, ExecutionContext::Scripting, radiusVariant) - radius) > 1e-6)
		settings.setValue(settingsKey, QVariant::fromValue(radius));
	else
		settings.remove(settingsKey);
}

}	// End of namespace
}	// End of namespace
