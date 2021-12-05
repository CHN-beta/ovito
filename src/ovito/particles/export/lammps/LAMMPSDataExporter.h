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

#pragma once


#include <ovito/particles/Particles.h>
#include <ovito/particles/export/ParticleExporter.h>
#include <ovito/particles/import/lammps/LAMMPSDataImporter.h>

namespace Ovito::Particles {

/**
 * \brief Exporter that writes the particles to a LAMMPS data file.
 */
class OVITO_PARTICLES_EXPORT LAMMPSDataExporter : public ParticleExporter
{
	/// Defines a metaclass specialization for this exporter type.
	class OOMetaClass : public ParticleExporter::OOMetaClass
	{
	public:
		/// Inherit standard constructor from base meta class.
		using ParticleExporter::OOMetaClass::OOMetaClass;

		/// Returns the file filter that specifies the extension of files written by this service.
		virtual QString fileFilter() const override { return QStringLiteral("*"); }

		/// Returns the filter description that is displayed in the drop-down box of the file dialog.
		virtual QString fileFilterDescription() const override { return tr("LAMMPS Data File"); }
	};

	OVITO_CLASS_META(LAMMPSDataExporter, OOMetaClass)

public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE LAMMPSDataExporter(DataSet* dataset) : ParticleExporter(dataset), 
		_atomStyle(LAMMPSDataImporter::AtomStyle_Atomic), 
		_omitMassesSection(false),
		_ignoreParticleIdentifiers(false) {}

protected:

	/// \brief Writes the particles of one animation frame to the current output file.
	virtual bool exportData(const PipelineFlowState& state, int frameNumber, TimePoint time, const QString& filePath, SynchronousOperation operation) override;

private:

	/// Selects the kind of LAMMPS data file to write.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(LAMMPSDataImporter::LAMMPSAtomStyle, atomStyle, setAtomStyle, PROPERTY_FIELD_MEMORIZE);

	/// The list of sub-styles if the hybrid atom style is used.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(std::vector<LAMMPSDataImporter::LAMMPSAtomStyle>, atomSubStyles, setAtomSubStyles);

	/// Flag that allows the user to suppress the "Masses" file section.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, omitMassesSection, setOmitMassesSection);

	/// Flag that allows the user to suppress export of existing particle identifiers.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, ignoreParticleIdentifiers, setIgnoreParticleIdentifiers);
};

}	// End of namespace
