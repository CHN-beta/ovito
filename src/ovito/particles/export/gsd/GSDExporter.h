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

#pragma once


#include <ovito/particles/Particles.h>
#include <ovito/particles/export/ParticleExporter.h>

namespace Ovito::Particles {

class GSDFile;	// Defined in GSDFile.h

/**
 * \brief Exporter that writes GSD (General Simulation Data) files as used by the HOOMD simulation code.
 */
class OVITO_PARTICLES_EXPORT GSDExporter : public ParticleExporter
{
	/// Defines a metaclass specialization for this exporter type.
	class OOMetaClass : public ParticleExporter::OOMetaClass
	{
	public:

		/// Inherit standard constructor from base meta class.
		using ParticleExporter::OOMetaClass::OOMetaClass;

		/// Returns the file filter that specifies the extension of files written by this service.
		virtual QString fileFilter() const override { return QStringLiteral("*.gsd"); }

		/// Returns the filter description that is displayed in the drop-down box of the file dialog.
		virtual QString fileFilterDescription() const override { return tr("GSD/HOOMD File"); }
	};

	OVITO_CLASS_META(GSDExporter, OOMetaClass)

public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE GSDExporter(ObjectCreationParams params);

	/// Destructor.
	~GSDExporter();

	/// \brief Indicates whether this file exporter can write more than one animation frame into a single output file.
	virtual bool supportsMultiFrameFiles() const override { return true; }

protected:

	/// \brief This is called once for every output file to be written and before exportFrame() is called.
	virtual bool openOutputFile(const QString& filePath, int numberOfFrames, MainThreadOperation& operation) override;

	/// \brief This is called once for every output file written after exportFrame() has been called.
	virtual void closeOutputFile(bool exportCompleted) override;

	/// \brief Writes the particles of one animation frame to the current output file.
	virtual bool exportData(const PipelineFlowState& state, int frameNumber, TimePoint time, const QString& filePath, MainThreadOperation& operation) override;

private:

	/// The file object.
	std::unique_ptr<GSDFile> _gsdFile;
};

}	// End of namespace
