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

#pragma once


#include <ovito/particles/Particles.h>
#include <ovito/particles/export/FileColumnParticleExporter.h>

namespace Ovito { namespace Particles {

/**
 * \brief Exporter that writes the particles to an extended AMBER NetCDF file.
 */
class OVITO_NETCDFPLUGIN_EXPORT AMBERNetCDFExporter : public FileColumnParticleExporter
{
	/// Defines a metaclass specialization for this exporter type.
	class OOMetaClass : public FileColumnParticleExporter::OOMetaClass
	{
	public:

		/// Inherit standard constructor from base meta class.
		using FileColumnParticleExporter::OOMetaClass::OOMetaClass;

		/// Returns the file filter that specifies the extension of files written by this service.
		virtual QString fileFilter() const override { return QStringLiteral("*.nc"); }

		/// Returns the filter description that is displayed in the drop-down box of the file dialog.
		virtual QString fileFilterDescription() const override { return tr("NetCDF/AMBER File"); }
	};

	Q_OBJECT
	OVITO_CLASS_META(AMBERNetCDFExporter, OOMetaClass)

public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE AMBERNetCDFExporter(DataSet* dataset) : FileColumnParticleExporter(dataset) {}

	/// \brief Indicates whether this file exporter can write more than one animation frame into a single output file.
	virtual bool supportsMultiFrameFiles() const override { return true; }

protected:

	/// \brief This is called once for every output file to be written and before exportFrame() is called.
	virtual bool openOutputFile(const QString& filePath, int numberOfFrames, SynchronousOperation operation) override;

	/// \brief This is called once for every output file written after exportFrame() has been called.
	virtual void closeOutputFile(bool exportCompleted) override;

	/// \brief Writes the particles of one animation frame to the current output file.
	virtual bool exportData(const PipelineFlowState& state, int frameNumber, TimePoint time, const QString& filePath, SynchronousOperation operation) override;

private:

	/// The NetCDF file handle.
	int _ncid = -1;

	// NetCDF file dimensions:
	int _frame_dim;
	int _spatial_dim;
	int _Voigt_dim;
	int _atom_dim = -1;
	int _cell_spatial_dim;
	int _cell_angular_dim;
	int _label_dim;

	// NetCDF file variables:
	int _spatial_var;
	int _cell_spatial_var;
	int _cell_angular_var;
	int _time_var;
	int _cell_origin_var;
	int _cell_lengths_var;
	int _cell_angles_var;
	int _coords_var;

	/// NetCDF file variables for global attributes.
	QMap<QString, int> _attributes_vars;

	/// Describes a data array to be written.
	struct NCOutputColumn {
		NCOutputColumn(const ParticlePropertyReference& p, int dt, size_t cc, int ncv) :
			property(p), dataType(dt), componentCount(cc), ncvar(ncv) {}

		ParticlePropertyReference property;
		int dataType;
		size_t componentCount; // Number of values per particle.
		int ncvar;
	};

	std::vector<NCOutputColumn> _columns;

	/// The number of frames written.
	size_t _frameCounter;
};

}	// End of namespace
}	// End of namespace
