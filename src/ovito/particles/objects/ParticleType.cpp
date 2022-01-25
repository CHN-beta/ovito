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
#include <ovito/core/dataset/io/FileSource.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include "ParticleType.h"

namespace Ovito::Particles {

IMPLEMENT_OVITO_CLASS(ParticleType);
DEFINE_PROPERTY_FIELD(ParticleType, radius);
DEFINE_PROPERTY_FIELD(ParticleType, vdwRadius);
DEFINE_PROPERTY_FIELD(ParticleType, shape);
DEFINE_REFERENCE_FIELD(ParticleType, shapeMesh);
DEFINE_PROPERTY_FIELD(ParticleType, highlightShapeEdges);
DEFINE_PROPERTY_FIELD(ParticleType, shapeBackfaceCullingEnabled);
DEFINE_PROPERTY_FIELD(ParticleType, shapeUseMeshColor);
DEFINE_PROPERTY_FIELD(ParticleType, mass);
DEFINE_SHADOW_PROPERTY_FIELD(ParticleType, radius);
DEFINE_SHADOW_PROPERTY_FIELD(ParticleType, vdwRadius);
DEFINE_SHADOW_PROPERTY_FIELD(ParticleType, shape);
DEFINE_SHADOW_PROPERTY_FIELD(ParticleType, highlightShapeEdges);
DEFINE_SHADOW_PROPERTY_FIELD(ParticleType, shapeBackfaceCullingEnabled);
DEFINE_SHADOW_PROPERTY_FIELD(ParticleType, shapeUseMeshColor);
DEFINE_SHADOW_PROPERTY_FIELD(ParticleType, mass);
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
ParticleType::ParticleType(ObjectCreationParams params) : ElementType(params),
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
void ParticleType::initializeType(const PropertyReference& property, bool loadUserDefaults)
{
	ElementType::initializeType(property, loadUserDefaults);

	// Load standard display radius.
	// First load the hardcoded default radius and freeze it, then load the user-defined default radius.
	setRadius(getDefaultParticleRadius(static_cast<ParticlesObject::Type>(property.type()), nameOrNumericId(), numericId(), false, DisplayRadius));
	freezeInitialParameterValues({SHADOW_PROPERTY_FIELD(ParticleType::radius)});
	if(loadUserDefaults)
		setRadius(getDefaultParticleRadius(static_cast<ParticlesObject::Type>(property.type()), nameOrNumericId(), numericId(), true, DisplayRadius));
	
	// Load standard van der Waals radius.
	// First load the hardcoded default radius and freeze it, then load the user-defined default radius.
	setVdwRadius(getDefaultParticleRadius(static_cast<ParticlesObject::Type>(property.type()), nameOrNumericId(), numericId(), false, VanDerWaalsRadius));
	freezeInitialParameterValues({SHADOW_PROPERTY_FIELD(ParticleType::vdwRadius)});
	if(loadUserDefaults)
		setVdwRadius(getDefaultParticleRadius(static_cast<ParticlesObject::Type>(property.type()), nameOrNumericId(), numericId(), true, VanDerWaalsRadius));
	
	// Load standard mass.
	// First load the hardcoded default mass and freeze it, then load the user-defined default mass.
	setMass(getDefaultParticleMass(static_cast<ParticlesObject::Type>(property.type()), nameOrNumericId(), numericId(), false));
	freezeInitialParameterValues({SHADOW_PROPERTY_FIELD(ParticleType::mass)});
	if(loadUserDefaults)
		setMass(getDefaultParticleMass(static_cast<ParticlesObject::Type>(property.type()), nameOrNumericId(), numericId(), true));
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
		if(proxy->radius() != self->radius() || proxy->vdwRadius() != self->vdwRadius() || proxy->mass() != self->mass() || proxy->shape() != self->shape() || proxy->shapeMesh() != self->shapeMesh() || proxy->highlightShapeEdges() != self->highlightShapeEdges() 
				|| proxy->shapeBackfaceCullingEnabled() != self->shapeBackfaceCullingEnabled() || proxy->shapeUseMeshColor() != self->shapeUseMeshColor()) {
			// Make this data object mutable first.
			ParticleType* mutableSelf = static_object_cast<ParticleType>(state.makeMutableInplace(dataPath));
			mutableSelf->setRadius(proxy->radius());
			mutableSelf->setVdwRadius(proxy->vdwRadius());
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
bool ParticleType::loadShapeMesh(const QUrl& sourceUrl, MainThreadOperation& operation, const FileImporterClass* importerType)
{
    operation.setProgressText(tr("Loading mesh geometry file %1").arg(sourceUrl.fileName()));

	// Temporarily disable undo recording while loading the geometry data.
	UndoSuspender noUndo(this);

	OORef<FileSourceImporter> importer;
	if(!importerType) {

		// Inspect input file to detect its format.
		Future<OORef<FileImporter>> importerFuture = FileImporter::autodetectFileFormat(dataset(), sourceUrl);
		if(!operation.waitForFuture(importerFuture))
			return false;

		importer = dynamic_object_cast<FileSourceImporter>(importerFuture.result());
	}
	else {
		importer = dynamic_object_cast<FileSourceImporter>(importerType->createInstance(dataset()));
	}
	if(!importer)
		throwException(tr("Could not detect the format of the geometry file. The format might not be supported."));

	// Create a temporary FileSource for loading the geometry data from the file.
	OORef<FileSource> fileSource = OORef<FileSource>::create(dataset());
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
	DataOORef<TriMeshObject> meshObj = DataOORef<TriMeshObject>::makeCopy(state.expectObject<TriMeshObject>());

	// Throw away any visual elements attached to the mesh object.
	meshObj->setVisElement(nullptr);

	// Show sharp edges of the mesh.
	meshObj->determineEdgeVisibility();

	// Turn on undo recording again. The final shape assignment should be recorded on the undo stack.
	noUndo.reset();
	setShapeMesh(std::move(meshObj));

	// Also switch the particle type's visualization shape to mesh-based.
	setShape(ParticlesVis::Mesh);

	// Determine whether the mesh is a closed manifold. 
	// If not, we should turn off back-face culling.
	if(shapeMesh() && !shapeMesh()->isClosed())
		setShapeBackfaceCullingEnabled(false);

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
// For radii that are not available in either of these publications use r = 2.0.
// The radii for ions (Na, K, Cl, Ca, Mg, and Cs) are based on the CHARMM27 Rmin/2 parameters for (SOD, POT, CLA, CAL, MG, CES).
//
// Colors and covalent radii of elements marked with '//' have been adopted from OpenBabel.
//
const std::array<ParticleType::PredefinedChemicalType, ParticleType::NUMBER_OF_PREDEFINED_PARTICLE_TYPES> ParticleType::_predefinedParticleTypes{{
	ParticleType::PredefinedChemicalType{ QStringLiteral("X"),  Color(255.0f/255.0f, 255.0f/255.0f, 255.0f/255.0f), 0.00f, 0.00f, 0.0 },
	ParticleType::PredefinedChemicalType{ QStringLiteral("H"),  Color(255.0f/255.0f, 255.0f/255.0f, 255.0f/255.0f), 0.46f, 1.20f, 1.00794 },
	ParticleType::PredefinedChemicalType{ QStringLiteral("He"), Color(217.0f/255.0f, 255.0f/255.0f, 255.0f/255.0f), 1.22f, 1.40f, 4.00260 },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Li"), Color(204.0f/255.0f, 128.0f/255.0f, 255.0f/255.0f), 1.57f, 1.82f, 6.941 },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Be"), Color(         0.76,          1.00,          0.00), 1.47f, 2.00f, 9.012182 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("B"),  Color(         1.00,          0.71,          0.71), 2.01f, 2.00f, 10.811 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("C"),  Color(144.0f/255.0f, 144.0f/255.0f, 144.0f/255.0f), 0.77f, 1.70f, 12.0107 },
	ParticleType::PredefinedChemicalType{ QStringLiteral("N"),  Color( 48.0f/255.0f,  80.0f/255.0f, 248.0f/255.0f), 0.74f, 1.55f, 14.0067 },
	ParticleType::PredefinedChemicalType{ QStringLiteral("O"),  Color(255.0f/255.0f,  13.0f/255.0f,  13.0f/255.0f), 0.74f, 1.52f, 15.9994 },
	ParticleType::PredefinedChemicalType{ QStringLiteral("F"),  Color(         0.50,          0.70,          1.00), 0.74f, 1.47f, 18.9984032 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Ne"), Color(         0.70,          0.89,          0.96), 0.74f, 1.54f, 20.1797 }, //

	ParticleType::PredefinedChemicalType{ QStringLiteral("Na"), Color(171.0f/255.0f,  92.0f/255.0f, 242.0f/255.0f), 1.91f, 1.36f, 22.989770 },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Mg"), Color(138.0f/255.0f, 255.0f/255.0f,   0.0f/255.0f), 1.60f, 1.18f, 24.3050 },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Al"), Color(191.0f/255.0f, 166.0f/255.0f, 166.0f/255.0f), 1.43f, 2.00f, 26.981538 },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Si"), Color(240.0f/255.0f, 200.0f/255.0f, 160.0f/255.0f), 1.18f, 2.10f, 28.0855 },
	ParticleType::PredefinedChemicalType{ QStringLiteral("P"),  Color(         1.00,          0.50,          0.00), 1.07f, 1.80f, 30.973761 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("S"),  Color(         0.70,          0.70,          0.00), 1.05f, 1.80f, 32.065 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Cl"), Color(         0.12,          0.94,          0.12), 1.02f, 2.27f, 35.453 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Ar"), Color(         0.50,          0.82,          0.89), 1.06f, 1.88f, 39.948 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("K"),  Color(         0.56,          0.25,          0.83), 2.03f, 1.76f, 39.0983 },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Ca"), Color(         0.24,          1.00,          0.00), 1.97f, 1.37f, 40.078 },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Sc"), Color(         0.90,          0.90,          0.90), 1.70f, 2.00f, 44.955910 }, //

	ParticleType::PredefinedChemicalType{ QStringLiteral("Ti"), Color(191.0f/255.0f, 194.0f/255.0f, 199.0f/255.0f), 1.47f, 2.00f, 47.867 },
	ParticleType::PredefinedChemicalType{ QStringLiteral("V"),  Color(         0.65,          0.65,          0.67), 1.53f, 2.00f, 50.9415 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Cr"), Color(138.0f/255.0f, 153.0f/255.0f, 199.0f/255.0f), 1.29f, 2.00f, 51.9961 },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Mn"), Color(         0.61,          0.48,          0.78), 1.39f, 2.00f, 54.938049 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Fe"), Color(224.0f/255.0f, 102.0f/255.0f,  51.0f/255.0f), 1.26f, 2.00f, 55.845 },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Co"), Color(240.0f/255.0f, 144.0f/255.0f, 160.0f/255.0f), 1.25f, 2.00f, 58.9332 },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Ni"), Color( 80.0f/255.0f, 208.0f/255.0f,  80.0f/255.0f), 1.25f, 1.63f, 58.6934 },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Cu"), Color(200.0f/255.0f, 128.0f/255.0f,  51.0f/255.0f), 1.28f, 1.40f, 63.546 },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Zn"), Color(125.0f/255.0f, 128.0f/255.0f, 176.0f/255.0f), 1.37f, 1.39f, 65.409 },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Ga"), Color(194.0f/255.0f, 143.0f/255.0f, 143.0f/255.0f), 1.53f, 1.07f, 69.723 },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Ge"), Color(102.0f/255.0f, 143.0f/255.0f, 143.0f/255.0f), 1.22f, 2.00f, 72.64 },

	ParticleType::PredefinedChemicalType{ QStringLiteral("As"), Color(         0.74,          0.50,          0.89), 1.19f, 1.85f, 74.92160 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Se"), Color(         1.00,          0.63,          0.00), 1.20f, 1.90f, 78.96 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Br"), Color(         0.65,          0.16,          0.16), 1.20f, 1.85f, 79.904 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Kr"), Color( 92.0f/255.0f, 184.0f/255.0f, 209.0f/255.0f), 1.98f, 2.02f, 83.798 },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Rb"), Color(         0.44,          0.18,          0.69), 2.20f, 2.00f, 85.4678 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Sr"), Color(         0.0f,          1.0f,      0.15259f), 2.15f, 2.00f, 87.62 },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Y"),  Color(     0.40259f,      0.59739f,      0.55813f), 1.82f, 2.00f, 88.90585 },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Zr"), Color(         0.0f,          1.0f,          0.0f), 1.60f, 2.00f, 91.224 },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Nb"), Color(     0.29992f,          0.7f,      0.46459f), 1.47f, 2.00f, 92.90638 },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Mo"), Color(         0.33,          0.71,          0.71), 1.54f, 2.00f, 95.94 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Tc"), Color(         0.23,          0.62,          0.62), 1.47f, 2.00f, 98.0 }, //

	ParticleType::PredefinedChemicalType{ QStringLiteral("Ru"), Color(         0.14,          0.56,          0.56), 1.46f, 2.00f, 101.07 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Rh"), Color(         0.04,          0.49,          0.55), 1.42f, 2.00f, 102.90550 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Pd"), Color(  0.0f/255.0f, 105.0f/255.0f, 133.0f/255.0f), 1.37f, 1.63f, 106.42 },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Ag"), Color(         0.88,          0.88,          1.00), 1.45f, 1.72f, 107.8682 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Cd"), Color(         1.00,          0.85,          0.56), 1.44f, 1.58f, 112.411 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("In"), Color(         0.65,          0.46,          0.45), 1.42f, 1.93f, 114.818 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Sn"), Color(         0.40,          0.50,          0.50), 1.39f, 2.17f, 118.710 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Sb"), Color(         0.62,          0.39,          0.71), 1.39f, 2.00f, 121.760 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Te"), Color(         0.83,          0.48,          0.00), 1.38f, 2.06f, 127.60 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("I"),  Color(         0.58,          0.00,          0.58), 1.39f, 1.98f, 126.90447 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Xe"), Color(         0.26,          0.62,          0.69), 1.40f, 2.16f, 131.293 }, //

	ParticleType::PredefinedChemicalType{ QStringLiteral("Cs"), Color(         0.34,          0.09,          0.56), 2.44f, 2.10f, 132.90545 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Ba"), Color(         0.00,          0.79,          0.00), 2.15f, 2.00f, 137.327 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("La"), Color(         0.44,          0.83,          1.00), 2.07f, 2.00f, 138.9055 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Ce"), Color(         1.00,          1.00,          0.78), 2.04f, 2.00f, 140.116 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Pr"), Color(         0.85,          1.00,          0.78), 2.03f, 2.00f, 140.90765 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Nd"), Color(         0.78,          1.00,          0.78), 2.01f, 2.00f, 144.24 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Pm"), Color(         0.64,          1.00,          0.78), 1.99f, 2.00f, 145.0 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Sm"), Color(         0.56,          1.00,          0.78), 1.98f, 2.00f, 150.36 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Eu"), Color(         0.38,          1.00,          0.78), 1.98f, 2.00f, 151.964 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Gd"), Color(         0.27,          1.00,          0.78), 1.96f, 2.00f, 157.25 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Tb"), Color(         0.19,          1.00,          0.78), 1.94f, 2.00f, 158.92534 }, //

	ParticleType::PredefinedChemicalType{ QStringLiteral("Dy"), Color(         0.12,          1.00,          0.78), 1.92f, 2.00f, 162.500 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Ho"), Color(         0.00,          1.00,          0.61), 1.92f, 2.00f, 164.93032 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Er"), Color(         0.00,          0.90,          0.46), 1.89f, 2.00f, 167.259 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Tm"), Color(         0.00,          0.83,          0.32), 1.90f, 2.00f, 168.93421 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Yb"), Color(         0.00,          0.75,          0.22), 1.87f, 2.00f, 173.04 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Lu"), Color(         0.00,          0.67,          0.14), 1.87f, 2.00f, 174.967 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Hf"), Color(         0.30,          0.76,          1.00), 1.75f, 2.00f, 178.49 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Ta"), Color(         0.30,          0.65,          1.00), 1.70f, 2.00f, 180.9479 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("W"),  Color(         0.13,          0.58,          0.84), 1.62f, 2.00f, 183.84 },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Re"), Color(         0.15,          0.49,          0.67), 1.51f, 2.00f, 186.207 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Os"), Color(         0.15,          0.40,          0.59), 1.44f, 2.00f, 190.23 }, //

	ParticleType::PredefinedChemicalType{ QStringLiteral("Ir"), Color(         0.09,          0.33,          0.53), 1.41f, 2.00f, 192.217 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Pt"), Color(         0.90,          0.85,          0.68), 1.39f, 1.72f, 195.078 },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Au"), Color(255.0f/255.0f, 209.0f/255.0f,  35.0f/255.0f), 1.44f, 1.66f, 196.96655 },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Hg"), Color(         0.71,          0.71,          0.76), 1.32f, 1.55f, 200.59 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Tl"), Color(         0.65,          0.33,          0.30), 1.45f, 1.96f, 204.3833 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Pb"), Color( 87.0f/255.0f,  89.0f/255.0f,  97.0f/255.0f), 1.47f, 2.02f, 207.2 },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Bi"), Color(158.0f/255.0f,  79.0f/255.0f, 181.0f/255.0f), 1.46f, 2.00f, 208.98038 },
	ParticleType::PredefinedChemicalType{ QStringLiteral("Po"), Color(         0.67,          0.36,          0.00), 1.40f, 2.00f, 209.0 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("At"), Color(         0.46,          0.31,          0.27), 1.50f, 2.00f, 210.0 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Rn"), Color(         0.26,          0.51,          0.59), 1.50f, 2.00f, 222.0 }, //
	ParticleType::PredefinedChemicalType{ QStringLiteral("Fr"), Color(         0.26,          0.00,          0.40), 2.60f, 2.00f, 223.0 }, //
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
FloatType ParticleType::getDefaultParticleRadius(ParticlesObject::Type typeClass, const QString& particleTypeName, int numericTypeId, bool loadUserDefaults, RadiusVariant radiusVariant)
{
	// Interactive execution context means that we are supposed to load the user-defined
	// settings from the settings store.
	if(loadUserDefaults && typeClass != ParticlesObject::UserProperty) {

#ifndef OVITO_DISABLE_QSETTINGS
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
#endif
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
			return getDefaultParticleRadius(typeClass, particleTypeName.left(particleTypeName.length() - 1), numericTypeId, loadUserDefaults, radiusVariant);
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

#ifndef OVITO_DISABLE_QSETTINGS
	QSettings settings;
	const QString& settingsKey = ElementType::getElementSettingsKey(ParticlePropertyReference(typeClass), 
		(radiusVariant == DisplayRadius) ? QStringLiteral("radius") : QStringLiteral("vdw_radius"), particleTypeName);
	
	if(std::abs(getDefaultParticleRadius(typeClass, particleTypeName, 0, false, radiusVariant) - radius) > 1e-6)
		settings.setValue(settingsKey, QVariant::fromValue(radius));
	else
		settings.remove(settingsKey);
#endif
}

/******************************************************************************
* Returns the default mass for a particle type.
******************************************************************************/
FloatType ParticleType::getDefaultParticleMass(ParticlesObject::Type typeClass, const QString& particleTypeName, int numericTypeId, bool loadUserDefaults)
{
	if(typeClass == ParticlesObject::TypeProperty) {
		for(const PredefinedChemicalType& predefType : _predefinedParticleTypes) {
			if(predefType.name == particleTypeName) {
				return predefType.mass;
			}
		}

		// Sometimes atom type names have additional letters/numbers appended.
		if(particleTypeName.length() > 1 && particleTypeName.length() <= 5) {
			return getDefaultParticleMass(typeClass, particleTypeName.left(particleTypeName.length() - 1), numericTypeId, loadUserDefaults);
		}
	}

	return 0;
}

}	// End of namespace
