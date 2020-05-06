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
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/particles/objects/ParticleType.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/core/app/Application.h>
#include "LAMMPSDataExporter.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(LAMMPSDataExporter);
DEFINE_PROPERTY_FIELD(LAMMPSDataExporter, atomStyle);
DEFINE_PROPERTY_FIELD(LAMMPSDataExporter, omitMassesSection);
SET_PROPERTY_FIELD_LABEL(LAMMPSDataExporter, atomStyle, "Atom style");
SET_PROPERTY_FIELD_LABEL(LAMMPSDataExporter, omitMassesSection, "Omit 'Masses' section");

/******************************************************************************
* Writes the particles of one animation frame to the current output file.
******************************************************************************/
bool LAMMPSDataExporter::exportData(const PipelineFlowState& state, int frameNumber, TimePoint time, const QString& filePath, SynchronousOperation operation)
{
	// Get the particle data to be exported.
	const ParticlesObject* particles = state.expectObject<ParticlesObject>();
	particles->verifyIntegrity();
	ConstPropertyAccess<Point3> posProperty = particles->expectProperty(ParticlesObject::PositionProperty);
	ConstPropertyAccess<Vector3> velocityProperty = particles->getProperty(ParticlesObject::VelocityProperty);
	ConstPropertyAccess<qlonglong> identifierProperty = particles->getProperty(ParticlesObject::IdentifierProperty);
	ConstPropertyAccess<Vector3I> periodicImageProperty = particles->getProperty(ParticlesObject::PeriodicImageProperty);
	const PropertyObject* particleTypeProperty = particles->getProperty(ParticlesObject::TypeProperty);
	ConstPropertyAccess<int> particleTypeArray(particleTypeProperty);
	ConstPropertyAccess<FloatType> chargeProperty = particles->getProperty(ParticlesObject::ChargeProperty);
	ConstPropertyAccess<FloatType> radiusProperty = particles->getProperty(ParticlesObject::RadiusProperty);
	ConstPropertyAccess<FloatType> massProperty = particles->getProperty(ParticlesObject::MassProperty);
	ConstPropertyAccess<qlonglong> moleculeProperty = particles->getProperty(ParticlesObject::MoleculeProperty);
	ConstPropertyAccess<Vector3> dipoleOrientationProperty = particles->getProperty(ParticlesObject::DipoleOrientationProperty);

	// Get the bond data to be exported.
	const BondsObject* bonds = particles->bonds();
	if(bonds) bonds->verifyIntegrity();
	ConstPropertyAccess<ParticleIndexPair> bondTopologyProperty = bonds ? bonds->getTopology() : nullptr;
	const PropertyObject* bondTypeProperty = bonds ? bonds->getProperty(BondsObject::TypeProperty) : nullptr;
	ConstPropertyAccess<int> bondTypeArray(bondTypeProperty);

	// Get the angle data to be exported.
	const AnglesObject* angles = particles->angles();
	if(angles) angles->verifyIntegrity();
	ConstPropertyAccess<ParticleIndexTriplet> angleTopologyProperty = angles ? angles->getTopology() : nullptr;
	const PropertyObject* angleTypeProperty = angles ? angles->getProperty(AnglesObject::TypeProperty) : nullptr;
	ConstPropertyAccess<int> angleTypeArray(angleTypeProperty);

	// Get the dihedral data to be exported.
	const DihedralsObject* dihedrals = particles->dihedrals();
	if(dihedrals) dihedrals->verifyIntegrity();
	ConstPropertyAccess<ParticleIndexQuadruplet> dihedralTopologyProperty = dihedrals ? dihedrals->getTopology() : nullptr;
	const PropertyObject* dihedralTypeProperty = dihedrals ? dihedrals->getProperty(DihedralsObject::TypeProperty) : nullptr;
	ConstPropertyAccess<int> dihedralTypeArray(dihedralTypeProperty);

	// Get the improper data to be exported.
	const ImpropersObject* impropers = particles->impropers();
	if(impropers) impropers->verifyIntegrity();
	ConstPropertyAccess<ParticleIndexQuadruplet> improperTopologyProperty = impropers ? impropers->getTopology() : nullptr;
	const PropertyObject* improperTypeProperty = impropers ? impropers->getProperty(ImpropersObject::TypeProperty) : nullptr;
	ConstPropertyAccess<int> improperTypeArray(improperTypeProperty);

	// Get simulation cell info.
	const SimulationCellObject* simulationCell = state.getObject<SimulationCellObject>();
	if(!simulationCell)
		throwException(tr("No simulation cell defined. Cannot write LAMMPS file."));

	const AffineTransformation& simCell = simulationCell->cellMatrix();

	// Transform triclinic cell to LAMMPS canonical format.
	Vector3 a,b,c;
	AffineTransformation transformation;
	bool transformCoordinates;
	if(simCell.column(0).x() < 0 || simCell.column(0).y() != 0 || simCell.column(0).z() != 0 || 
			simCell.column(1).y() < 0 || simCell.column(1).z() != 0 || simCell.column(2).z() < 0) {
		a.x() = simCell.column(0).length();
		a.y() = a.z() = 0;
		b.x() = simCell.column(1).dot(simCell.column(0)) / a.x();
		b.y() = sqrt(simCell.column(1).squaredLength() - b.x()*b.x());
		b.z() = 0;
		c.x() = simCell.column(2).dot(simCell.column(0)) / a.x();
		c.y() = (simCell.column(1).dot(simCell.column(2)) - b.x()*c.x()) / b.y();
		c.z() = sqrt(simCell.column(2).squaredLength() - c.x()*c.x() - c.y()*c.y());
		transformCoordinates = true;
		transformation = AffineTransformation(a,b,c,simCell.translation()) * simCell.inverse();
	}
	else {
		a = simCell.column(0);
		b = simCell.column(1);
		c = simCell.column(2);
		transformCoordinates = false;
	}

	FloatType xlo = simCell.translation().x();
	FloatType ylo = simCell.translation().y();
	FloatType zlo = simCell.translation().z();
	FloatType xhi = a.x() + xlo;
	FloatType yhi = b.y() + ylo;
	FloatType zhi = c.z() + zlo;
	FloatType xy = b.x();
	FloatType xz = c.x();
	FloatType yz = c.y();

	// Decide if we want to export bonds/angles/dihedrals/impropers.
	bool writeBonds     = bondTopologyProperty && (atomStyle() != LAMMPSDataImporter::AtomStyle_Atomic);
	bool writeAngles    = angleTopologyProperty && (atomStyle() != LAMMPSDataImporter::AtomStyle_Atomic);
	bool writeDihedrals = dihedralTopologyProperty && (atomStyle() != LAMMPSDataImporter::AtomStyle_Atomic);
	bool writeImpropers = improperTopologyProperty && (atomStyle() != LAMMPSDataImporter::AtomStyle_Atomic);

	textStream() << "# LAMMPS data file written by " << Application::applicationName() << " " << Application::applicationVersionString() << "\n";
	textStream() << particles->elementCount() << " atoms\n";
	if(writeBonds)
		textStream() << bonds->elementCount() << " bonds\n";
	if(writeAngles)
		textStream() << angles->elementCount() << " angles\n";
	if(writeDihedrals)
		textStream() << dihedrals->elementCount() << " dihedrals\n";
	if(writeImpropers)
		textStream() << impropers->elementCount() << " impropers\n";

	int numLAMMPSAtomTypes = 1;
	if(particleTypeArray && particleTypeArray.size() > 0) {
		numLAMMPSAtomTypes = std::max(particleTypeProperty->elementTypes().size(), *boost::max_element(particleTypeArray));
		textStream() << numLAMMPSAtomTypes << " atom types\n";
	}
	else textStream() << "1 atom types\n";

	if(writeBonds) {
		if(bondTypeArray && bondTypeArray.size() > 0) {
			int numBondTypes = std::max(bondTypeProperty->elementTypes().size(), *boost::max_element(bondTypeArray));
			textStream() << numBondTypes << " bond types\n";
		}
		else textStream() << "1 bond types\n";
	}

	if(writeAngles) {
		if(angleTypeArray && angleTypeArray.size() > 0) {
			int numAngleTypes = std::max(angleTypeProperty->elementTypes().size(), *boost::max_element(angleTypeArray));
			textStream() << numAngleTypes << " angle types\n";
		}
		else textStream() << "1 angle types\n";
	}

	if(writeDihedrals) {
		if(dihedralTypeArray && dihedralTypeArray.size() > 0) {
			int numDihedralTypes = std::max(dihedralTypeProperty->elementTypes().size(), *boost::max_element(dihedralTypeArray));
			textStream() << numDihedralTypes << " dihedral types\n";
		}
		else textStream() << "1 dihedral types\n";
	}

	if(writeImpropers) {
		if(improperTypeArray && improperTypeArray.size() > 0) {
			int numImproperTypes = std::max(improperTypeProperty->elementTypes().size(), *boost::max_element(improperTypeArray));
			textStream() << numImproperTypes << " improper types\n";
		}
		else textStream() << "1 improper types\n";
	}

	textStream() << xlo << ' ' << xhi << " xlo xhi\n";
	textStream() << ylo << ' ' << yhi << " ylo yhi\n";
	textStream() << zlo << ' ' << zhi << " zlo zhi\n";
	if(xy != 0 || xz != 0 || yz != 0) {
		textStream() << xy << ' ' << xz << ' ' << yz << " xy xz yz\n";
	}
	textStream() << "\n";

	// Write "Masses" section.
	if(!omitMassesSection() && particleTypeProperty && particleTypeProperty->elementTypes().size() > 0 && atomStyle() != LAMMPSDataImporter::AtomStyle_Sphere) {
		// Write the "Masses" section only if there is at least one atom type with a non-zero mass.
		bool hasNonzeroMass = boost::algorithm::any_of(particleTypeProperty->elementTypes(), [](const ElementType* type) {
			if(const ParticleType* ptype = dynamic_object_cast<ParticleType>(type))
				return ptype->mass() != 0;
			return false;
		}); 
		if(hasNonzeroMass) {
			textStream() << "Masses\n\n";
			for(int atomType = 1; atomType <= numLAMMPSAtomTypes; atomType++) {
				if(const ParticleType* ptype = dynamic_object_cast<ParticleType>(particleTypeProperty->elementType(atomType))) {
					textStream() << atomType << " " << ((ptype->mass() > 0.0) ? ptype->mass() : 1.0);
					if(!ptype->name().isEmpty())
						textStream() << "  # " << ptype->name();
				}
				else {
					textStream() << atomType << " " << 1.0;
				}
				textStream() << "\n";
			}
			textStream() << "\n";
		}
	}

	qlonglong totalProgressCount = particles->elementCount();
	if(velocityProperty) totalProgressCount += particles->elementCount();
	if(writeBonds) totalProgressCount += bonds->elementCount();
	if(writeAngles) totalProgressCount += angles->elementCount();
	if(writeDihedrals) totalProgressCount += dihedrals->elementCount();
	if(writeImpropers) totalProgressCount += impropers->elementCount();

	// Write "Atoms" section.
	textStream() << "Atoms";
	switch(atomStyle()) {
		case LAMMPSDataImporter::AtomStyle_Atomic: textStream() << "  # atomic"; break;
		case LAMMPSDataImporter::AtomStyle_Angle: textStream() << "  # angle"; break;
		case LAMMPSDataImporter::AtomStyle_Bond: textStream() << "  # bond"; break;
		case LAMMPSDataImporter::AtomStyle_Molecular: textStream() << "  # molecular"; break;
		case LAMMPSDataImporter::AtomStyle_Full: textStream() << "  # full"; break;
		case LAMMPSDataImporter::AtomStyle_Charge: textStream() << "  # charge"; break;
		case LAMMPSDataImporter::AtomStyle_Dipole: textStream() << "  # dipole"; break;
		case LAMMPSDataImporter::AtomStyle_Sphere: textStream() << "  # sphere"; break;
		default: break; // Do nothing
	}
	textStream() << "\n\n";

	operation.setProgressMaximum(totalProgressCount);
	qlonglong currentProgress = 0;
	for(size_t i = 0; i < posProperty.size(); i++) {
		// atom-ID
		textStream() << (identifierProperty ? identifierProperty[i] : (i+1));
		if(atomStyle() == LAMMPSDataImporter::AtomStyle_Bond || atomStyle() == LAMMPSDataImporter::AtomStyle_Molecular || atomStyle() == LAMMPSDataImporter::AtomStyle_Full || atomStyle() == LAMMPSDataImporter::AtomStyle_Angle) {
			textStream() << ' ';
			// molecule-ID
			textStream() << (moleculeProperty ? moleculeProperty[i] : 1);
		}
		textStream() << ' ';
		// atom-type
		textStream() << (particleTypeArray ? particleTypeArray[i] : 1);
		if(atomStyle() == LAMMPSDataImporter::AtomStyle_Charge || atomStyle() == LAMMPSDataImporter::AtomStyle_Dipole || atomStyle() == LAMMPSDataImporter::AtomStyle_Full) {
			textStream() << ' ';
			// charge
			textStream() << (chargeProperty ? chargeProperty[i] : 0);
		}
		else if(atomStyle() == LAMMPSDataImporter::AtomStyle_Sphere) {
			// diameter
			FloatType radius = (radiusProperty ? radiusProperty[i] : 0);
			textStream() << ' ' << (radius * 2);
			// density
			FloatType density = (massProperty ? massProperty[i] : 0);
			if(radius > 0) density /= pow(radius, 3) * (FLOATTYPE_PI * FloatType(4) / FloatType(3));
			textStream() << ' ' << density;
		}
		// x y z
		const Point3& pos = posProperty[i];
		if(!transformCoordinates) {
			for(size_t k = 0; k < 3; k++)
				textStream() << ' ' << pos[k];
		}
		else {
			for(size_t k = 0; k < 3; k++)
				textStream() << ' ' << transformation.prodrow(pos, k);
		}
		if(atomStyle() == LAMMPSDataImporter::AtomStyle_Dipole) {
			// mux muy muz
			if(dipoleOrientationProperty) {
				textStream() << ' ' << dipoleOrientationProperty[i][0]
							 << ' ' << dipoleOrientationProperty[i][1]
							 << ' ' << dipoleOrientationProperty[i][2];
			}
			else {
				textStream() << " 0 0 0";
			}
		}
		if(periodicImageProperty) {
			// pbc images
			const Vector3I& pbc = periodicImageProperty[i];
			for(size_t k = 0; k < 3; k++) {
				textStream() << ' ' << pbc[k];
			}
		}
		textStream() << '\n';

		if(!operation.setProgressValueIntermittent(currentProgress++))
			return false;
	}

	// Write velocities.
	if(velocityProperty) {
		textStream() << "\nVelocities\n\n";
		const Vector3* v = velocityProperty.cbegin();
		for(size_t i = 0; i < velocityProperty.size(); i++, ++v) {
			textStream() << (identifierProperty ? identifierProperty[i] : (i+1));
			if(!transformCoordinates) {
				for(size_t k = 0; k < 3; k++)
					textStream() << ' ' << (*v)[k];
			}
			else {
				for(size_t k = 0; k < 3; k++)
					textStream() << ' ' << transformation.prodrow(*v, k);
			}
			textStream() << '\n';

			if(!operation.setProgressValueIntermittent(currentProgress++))
				return false;
		}
	}

	// Write bonds.
	if(writeBonds) {
		textStream() << "\nBonds\n\n";

		size_t bondIndex = 1;
		for(size_t i = 0; i < bondTopologyProperty.size(); i++) {
			size_t atomIndex1 = bondTopologyProperty[i][0];
			size_t atomIndex2 = bondTopologyProperty[i][1];
			if(atomIndex1 >= particles->elementCount() || atomIndex2 >= particles->elementCount())
				throwException(tr("Particle indices in the bond topology array are out of range."));
			textStream() << bondIndex++;
			textStream() << ' ';
			textStream() << (bondTypeArray ? bondTypeArray[i] : 1);
			textStream() << ' ';
			textStream() << (identifierProperty ? identifierProperty[atomIndex1] : (atomIndex1+1));
			textStream() << ' ';
			textStream() << (identifierProperty ? identifierProperty[atomIndex2] : (atomIndex2+1));
			textStream() << '\n';

			if(!operation.setProgressValueIntermittent(currentProgress++))
				return false;
		}
		OVITO_ASSERT(bondIndex == bondTopologyProperty.size() + 1);
	}

	// Write angles.
	if(writeAngles) {
		textStream() << "\nAngles\n\n";

		size_t angleIndex = 1;
		for(size_t i = 0; i < angleTopologyProperty.size(); i++) {
			size_t atomIndex1 = angleTopologyProperty[i][0];
			size_t atomIndex2 = angleTopologyProperty[i][1];
			size_t atomIndex3 = angleTopologyProperty[i][2];
			if(atomIndex1 >= particles->elementCount() || atomIndex2 >= particles->elementCount() || atomIndex3 >= particles->elementCount())
				throwException(tr("Particle indices in the angle topology array are out of range."));
			textStream() << angleIndex++;
			textStream() << ' ';
			textStream() << (angleTypeArray ? angleTypeArray[i] : 1);
			textStream() << ' ';
			textStream() << (identifierProperty ? identifierProperty[atomIndex1] : (atomIndex1+1));
			textStream() << ' ';
			textStream() << (identifierProperty ? identifierProperty[atomIndex2] : (atomIndex2+1));
			textStream() << ' ';
			textStream() << (identifierProperty ? identifierProperty[atomIndex3] : (atomIndex3+1));
			textStream() << '\n';

			if(!operation.setProgressValueIntermittent(currentProgress++))
				return false;
		}
		OVITO_ASSERT(angleIndex == angleTopologyProperty.size() + 1);
	}

	// Write dihedrals.
	if(writeDihedrals) {
		textStream() << "\nDihedrals\n\n";

		size_t dihedralIndex = 1;
		for(size_t i = 0; i < dihedralTopologyProperty.size(); i++) {
			size_t atomIndex1 = dihedralTopologyProperty[i][0];
			size_t atomIndex2 = dihedralTopologyProperty[i][1];
			size_t atomIndex3 = dihedralTopologyProperty[i][2];
			size_t atomIndex4 = dihedralTopologyProperty[i][3];
			if(atomIndex1 >= particles->elementCount() || atomIndex2 >= particles->elementCount() || atomIndex3 >= particles->elementCount() || atomIndex4 >= particles->elementCount())
				throwException(tr("Particle indices in the dihedral topology array are out of range."));
			textStream() << dihedralIndex++;
			textStream() << ' ';
			textStream() << (dihedralTypeArray ? dihedralTypeArray[i] : 1);
			textStream() << ' ';
			textStream() << (identifierProperty ? identifierProperty[atomIndex1] : (atomIndex1+1));
			textStream() << ' ';
			textStream() << (identifierProperty ? identifierProperty[atomIndex2] : (atomIndex2+1));
			textStream() << ' ';
			textStream() << (identifierProperty ? identifierProperty[atomIndex3] : (atomIndex3+1));
			textStream() << ' ';
			textStream() << (identifierProperty ? identifierProperty[atomIndex4] : (atomIndex4+1));
			textStream() << '\n';

			if(!operation.setProgressValueIntermittent(currentProgress++))
				return false;
		}
		OVITO_ASSERT(dihedralIndex == dihedralTopologyProperty.size() + 1);
	}

	// Write impropers.
	if(writeImpropers) {
		textStream() << "\nImpropers\n\n";

		size_t improperIndex = 1;
		for(size_t i = 0; i < improperTopologyProperty.size(); i++) {
			size_t atomIndex1 = improperTopologyProperty[i][0];
			size_t atomIndex2 = improperTopologyProperty[i][1];
			size_t atomIndex3 = improperTopologyProperty[i][2];
			size_t atomIndex4 = improperTopologyProperty[i][3];
			if(atomIndex1 >= particles->elementCount() || atomIndex2 >= particles->elementCount() || atomIndex3 >= particles->elementCount() || atomIndex4 >= particles->elementCount())
				throwException(tr("Particle indices in the improper topology array are out of range."));
			textStream() << improperIndex++;
			textStream() << ' ';
			textStream() << (improperTypeArray ? improperTypeArray[i] : 1);
			textStream() << ' ';
			textStream() << (identifierProperty ? identifierProperty[atomIndex1] : (atomIndex1+1));
			textStream() << ' ';
			textStream() << (identifierProperty ? identifierProperty[atomIndex2] : (atomIndex2+1));
			textStream() << ' ';
			textStream() << (identifierProperty ? identifierProperty[atomIndex3] : (atomIndex3+1));
			textStream() << ' ';
			textStream() << (identifierProperty ? identifierProperty[atomIndex4] : (atomIndex4+1));
			textStream() << '\n';

			if(!operation.setProgressValueIntermittent(currentProgress++))
				return false;
		}
		OVITO_ASSERT(improperIndex == improperTopologyProperty.size() + 1);
	}

	return !operation.isCanceled();
}

}	// End of namespace
}	// End of namespace
