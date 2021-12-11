////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 OVITO GmbH, Germany
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
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include "XYZExporter.h"

namespace Ovito::Particles {

IMPLEMENT_OVITO_CLASS(XYZExporter);
DEFINE_PROPERTY_FIELD(XYZExporter, subFormat);
SET_PROPERTY_FIELD_LABEL(XYZExporter, subFormat, "XYZ format style");

/******************************************************************************
* Writes the particles of one animation frame to the current output file.
******************************************************************************/
bool XYZExporter::exportData(const PipelineFlowState& state, int frameNumber, TimePoint time, const QString& filePath, SynchronousOperation operation)
{
	// Get particle positions.
	const ParticlesObject* particles = state.expectObject<ParticlesObject>();
	particles->verifyIntegrity();

	size_t atomsCount = particles->elementCount();
	textStream() << atomsCount << '\n';

	const ParticlesOutputColumnMapping& mapping = columnMapping();
	if(mapping.empty())
		throwException(tr("No particle properties have been selected for export to the XYZ file. Cannot write file with zero columns."));
	PropertyOutputWriter columnWriter(mapping, particles, PropertyOutputWriter::WriteNamesUnderscore);

	const SimulationCellObject* simulationCell = state.getObject<SimulationCellObject>();

	if(subFormat() == ParcasFormat) {
		textStream() << QStringLiteral("Frame %1").arg(frameNumber);
		if(simulationCell) {
			const AffineTransformation& simCell = simulationCell->cellMatrix();
			textStream() << " cell_orig " << simCell.translation().x() << " " << simCell.translation().y() << " " << simCell.translation().z();
			textStream() << " cell_vec1 " << simCell.column(0).x() << " " << simCell.column(0).y() << " " << simCell.column(0).z();
			textStream() << " cell_vec2 " << simCell.column(1).x() << " " << simCell.column(1).y() << " " << simCell.column(1).z();
			textStream() << " cell_vec3 " << simCell.column(2).x() << " " << simCell.column(2).y() << " " << simCell.column(2).z();
			textStream() << " pbc " << simulationCell->pbcX() << " " << simulationCell->pbcY() << " " << simulationCell->pbcZ();
		}
	}
	else if(subFormat() == ExtendedFormat) {
		if(simulationCell) {
			const AffineTransformation& simCell = simulationCell->cellMatrix();
			// Save cell information in extended XYZ format:
			// see http://libatoms.github.io/QUIP/io.html#module-ase.io.extxyz for details
			textStream() << QStringLiteral("Lattice=\"");
			textStream() << simCell.column(0).x() << " " << simCell.column(0).y() << " " << simCell.column(0).z() << " ";
			textStream() << simCell.column(1).x() << " " << simCell.column(1).y() << " " << simCell.column(1).z() << " ";
			textStream() << simCell.column(2).x() << " " << simCell.column(2).y() << " " << simCell.column(2).z() << "\" ";

			if(simCell.translation() != Vector3::Zero()) {
				textStream() << QStringLiteral("Origin=\"");
				textStream() << simCell.translation().x() << " " << simCell.translation().y() << " " << simCell.translation().z() << "\" ";
			}
		}
		// Save column information in extended XYZ format:
		// see http://libatoms.github.io/QUIP/io.html#module-ase.io.extxyz for details
		textStream() << QStringLiteral("Properties=");
		QString propertiesStr;
		int i = 0;
		while(i < (int)mapping.size()) {
			const ParticlePropertyReference& pref = mapping[i];

			// Convert from OVITO property type and name to extended XYZ property name
			// Naming conventions followed are those of the QUIP code
			QString columnName;
			switch(pref.type()) {
			case ParticlesObject::TypeProperty: columnName = QStringLiteral("species"); break;
			case ParticlesObject::PositionProperty: columnName = QStringLiteral("pos"); break;
			case ParticlesObject::SelectionProperty: columnName = QStringLiteral("selection"); break;
			case ParticlesObject::ColorProperty: columnName = QStringLiteral("color"); break;
			case ParticlesObject::DisplacementProperty: columnName = QStringLiteral("disp"); break;
			case ParticlesObject::DisplacementMagnitudeProperty: columnName = QStringLiteral("disp_mag"); break;
			case ParticlesObject::PotentialEnergyProperty: columnName = QStringLiteral("local_energy"); break;
			case ParticlesObject::KineticEnergyProperty: columnName = QStringLiteral("kinetic_energy"); break;
			case ParticlesObject::TotalEnergyProperty: columnName = QStringLiteral("total_energy"); break;
			case ParticlesObject::VelocityProperty: columnName = QStringLiteral("velo"); break;
			case ParticlesObject::VelocityMagnitudeProperty: columnName = QStringLiteral("velo_mag"); break;
			case ParticlesObject::RadiusProperty: columnName = QStringLiteral("radius"); break;
			case ParticlesObject::ClusterProperty: columnName = QStringLiteral("cluster"); break;
			case ParticlesObject::CoordinationProperty: columnName = QStringLiteral("n_neighb"); break;
			case ParticlesObject::StructureTypeProperty: columnName = QStringLiteral("structure_type"); break;
			case ParticlesObject::IdentifierProperty: columnName = QStringLiteral("id"); break;
			case ParticlesObject::StressTensorProperty: columnName = QStringLiteral("stress"); break;
			case ParticlesObject::StrainTensorProperty: columnName = QStringLiteral("strain"); break;
			case ParticlesObject::DeformationGradientProperty: columnName = QStringLiteral("deform"); break;
			case ParticlesObject::OrientationProperty: columnName = QStringLiteral("orientation"); break;
			case ParticlesObject::ForceProperty: columnName = QStringLiteral("force"); break;
			case ParticlesObject::MassProperty: columnName = QStringLiteral("mass"); break;
			case ParticlesObject::ChargeProperty: columnName = QStringLiteral("charge"); break;
			case ParticlesObject::PeriodicImageProperty: columnName = QStringLiteral("map_shift"); break;
			case ParticlesObject::TransparencyProperty: columnName = QStringLiteral("transparency"); break;
			case ParticlesObject::DipoleOrientationProperty: columnName = QStringLiteral("dipoles"); break;
			case ParticlesObject::DipoleMagnitudeProperty: columnName = QStringLiteral("dipoles_mag"); break;
			case ParticlesObject::AngularVelocityProperty: columnName = QStringLiteral("omega"); break;
			case ParticlesObject::AngularMomentumProperty: columnName = QStringLiteral("angular_momentum"); break;
			case ParticlesObject::TorqueProperty: columnName = QStringLiteral("torque"); break;
			case ParticlesObject::SpinProperty: columnName = QStringLiteral("spin"); break;
			case ParticlesObject::CentroSymmetryProperty: columnName = QStringLiteral("centro_symmetry"); break;
			case ParticlesObject::AsphericalShapeProperty: columnName = QStringLiteral("aspherical_shape"); break;
			case ParticlesObject::VectorColorProperty: columnName = QStringLiteral("vector_color"); break;
			case ParticlesObject::MoleculeProperty: columnName = QStringLiteral("molecule"); break;
			case ParticlesObject::MoleculeTypeProperty: columnName = QStringLiteral("molecule_type"); break;
			default:
				columnName = pref.name();
				columnName.remove(QRegularExpression(QStringLiteral("[^A-Za-z\\d_]")));
			}

			// Find matching property
			const PropertyObject* property = pref.findInContainer(particles);
			if(property == nullptr && pref.type() != ParticlesObject::IdentifierProperty)
				throwException(tr("Particle property '%1' cannot be exported because it does not exist.").arg(pref.name()));

			// Count the number of consecutive columns with the same property.
			int nCols = 1;
			while(++i < (int)mapping.size() && pref.name() == mapping[i].name() && pref.type() == mapping[i].type())
				nCols++;

			// Convert OVITO property data type to extended XYZ type code: 'I','R','S','L'
			int dataType = property ? property->dataType() : PropertyObject::Int;
			QString dataTypeStr;
			if(dataType == PropertyObject::Float)
				dataTypeStr = QStringLiteral("R");
			else if(dataType == qMetaTypeId<char>() || pref.type() == ParticlesObject::TypeProperty)
				dataTypeStr = QStringLiteral("S");
			else if(dataType == PropertyObject::Int || dataType == PropertyObject::Int64)
				dataTypeStr = QStringLiteral("I");
			else if(dataType == qMetaTypeId<bool>())
				dataTypeStr = QStringLiteral("L");
			else
				throwException(tr("Unexpected data type '%1' for property '%2'.").arg(getQtTypeNameFromId(dataType) ? getQtTypeNameFromId(dataType) : "unknown").arg(pref.name()));

			if(!propertiesStr.isEmpty()) propertiesStr += QStringLiteral(":");
			propertiesStr += QStringLiteral("%1:%2:%3").arg(columnName).arg(dataTypeStr).arg(nCols);
		}
		textStream() << propertiesStr;
	}
	textStream() << '\n';

	operation.setProgressMaximum(atomsCount);
	for(size_t i = 0; i < atomsCount; i++) {
		columnWriter.writeElement(i, textStream());

		if(!operation.setProgressValueIntermittent(i))
			return false;
	}

	return !operation.isCanceled();
}

}	// End of namespace
